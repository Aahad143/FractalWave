#include "audioplayback.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// Constructor: initializes audio device and registers callbacks.
AudioPlayback::AudioPlayback()
    : fft(fftOrder),
    fftData(fftSize * 2, 0.0f),  // FFT requires an array of size 2*fftSize.
    fftWindow(fftSize, 0.0f),
    sampleBuffer(fftSize),
    frequencyBands{},
    capturingSource(&transportSource, sampleBuffer, this)  // Initially wrap transportSource.
{
    // Initialise the device manager: no input channels, 2 output channels.
    deviceManager.initialise(0, 2, nullptr, true);

    // Set up the AudioSourcePlayer so that it pulls audio from transportSource.
    deviceManager.addAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(&capturingSource);

    // Register basic audio file formats (WAV, AIFF, MP3, etc.).
    formatManager.registerBasicFormats();

    // Initialize Hann window for FFT.
    for (int i = 0; i < fftSize; ++i)
        fftWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
}

// Destructor: cleans up and disconnects callbacks.
AudioPlayback::~AudioPlayback()
{
    // Disconnect the audio callback.
    audioSourcePlayer.setSource(nullptr);
    deviceManager.removeAudioCallback(&audioSourcePlayer);

    // Stop playback and release the audio source.
    transportSource.stop();
    transportSource.setSource(nullptr);

    capturingSource.setSource(nullptr);
}

// loadFile(): Attempts to load an audio file.
// Returns true if the file is successfully loaded.
bool AudioPlayback::loadFile(const QString filePath)
{
    currentTrackPath = filePath;

    // Convert the QString file path to a JUCE File.
    const juce::File juceFile(filePath.toStdString());

    // Create a reader for the file using the format manager.
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(juceFile));
    if (reader == nullptr)
        return false; // File could not be opened.

    // Create a reader source that will stream the file.
    // The 'true' flag makes the reader source take ownership of the reader.
    readerSource.reset(new juce::AudioFormatReaderSource(reader.release(), true));

    // Set the transport source to use the reader source.
    // The second parameter (bufferSizeInSamples) is 0 (default buffering),
    // and we use the sample rate from the file.
    transportSource.setSource(readerSource.get(), 0, nullptr,
                              readerSource->getAudioFormatReader()->sampleRate);

    // Connect capturing source to the transportSource.
    capturingSource.setSource(&transportSource);

    return true;
}

// unloadFile(): Unloads the currently loaded track.
void AudioPlayback::unloadFile()
{
    // Stop playback if needed.
    transportSource.stop();
    // Unlink the current source.
    transportSource.setSource(nullptr);
    capturingSource.setSource(nullptr);

    // Delete the reader source.
    readerSource.reset();
    // Clear the current track path.
    currentTrackPath.clear();
}

// play(): Starts playback by resetting position and starting the transport.
void AudioPlayback::play()
{
    transportSource.setPosition(0.0);
    transportSource.start();
}

// stop(): Stops audio playback.
void AudioPlayback::stop()
{
    transportSource.stop();
}

// togglePause(): If playing, stop (pause); if paused, start (resume).
void AudioPlayback::togglePause()
{
    if (transportSource.isPlaying())
        transportSource.stop();
    else
        transportSource.start();
}

// replaceTrack(): Unloads the current track, loads a new track, and starts playback.
// Returns true if successful.
bool AudioPlayback::replaceTrack(const QString filePath)
{
    // Unload the current track.
    unloadFile();

    // Attempt to load the new track.
    if (!loadFile(filePath))
        return false;

    // Start playback of the new track.
    play();
    return true;
}

QString& AudioPlayback::getCurrentTrackPath()
{
    return currentTrackPath;
}

void AudioPlayback::seek(double newPosition)
{
    // Set the transport source position.
    transportSource.setPosition(newPosition);
}

bool AudioPlayback::hasAudioLoaded()
{
    return readerSource != nullptr;
}

//============================================================================
// performFFT: Captures fftSize samples from sampleBuffer, applies windowing,
// performs an FFT, and analyzes frequency bands.
void AudioPlayback::performFFT()
{
    std::vector<float> fftInput;
    sampleBuffer.getBuffer(fftInput);

    if (fftInput.size() < static_cast<size_t>(fftSize))
    {
        juce::Logger::writeToLog("Not enough samples in circular buffer: " + juce::String(fftInput.size()));
        return;
    }

    // Ensure we only process exactly fftSize samples.
    fftInput.resize(fftSize);

    // Clear fftData.
    std::fill(fftData.begin(), fftData.end(), 0.0f);

    // Copy fftInput samples into fftData (interleaved real-imaginary).
    for (int i = 0; i < fftSize; ++i)
    {
        fftData[i * 2]     = fftInput[i] * fftWindow[i]; // apply window to real part.
        fftData[i * 2 + 1] = 0.0f;                         // imaginary part is zero.
    }

    // Perform the FFT.
    fft.performRealOnlyForwardTransform(fftData.data(), true);

    // Analyze frequency bands.
    analyzeFrequencyBands();
}

//============================================================================
// analyzeFrequencyBands: Processes fftData to compute average amplitudes for defined bands.
void AudioPlayback::analyzeFrequencyBands()
{
    std::fill(frequencyBands.begin(), frequencyBands.end(), 0.0f);

    // Get sample rate from readerSource if available.
    float sampleRate = 44100.0f;
    if (readerSource != nullptr && readerSource->getAudioFormatReader() != nullptr)
        sampleRate = readerSource->getAudioFormatReader()->sampleRate;

    // Convert frequency ranges (Hz) to FFT bin indices.
    for (int band = 0; band < NumBands; ++band)
    {
        int startBin = juce::jlimit(0, fftSize / 2, static_cast<int>(bandRanges[band].min * fftSize / sampleRate));
        int endBin   = juce::jlimit(0, fftSize / 2, static_cast<int>(bandRanges[band].max   * fftSize / sampleRate));
        float sum = 0.0f;
        for (int i = startBin; i < endBin; ++i)
        {
            // In a real-only FFT, the output is stored in interleaved format.
            float re = fftData[i * 2];
            float im = fftData[i * 2 + 1];
            float magnitude = std::sqrt(re * re + im * im);
            sum += magnitude;
        }
        if (endBin > startBin)
            frequencyBands[band] = sum / (endBin - startBin);
        else
            frequencyBands[band] = 0.0f;
    }
}

//============================================================================
// getFrequencyBandLevel: Returns the computed amplitude for a given frequency band.
float AudioPlayback::getFrequencyBandLevel(FrequencyBand band) const
{
    if (band >= 0 && band < NumBands)
        return frequencyBands[band];
    return 0.0f;
}
