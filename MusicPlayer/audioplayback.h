// AUDIOPLAYBACK.H - Beginner-friendly, organized and commented
#ifndef AUDIOPLAYBACK_H
#define AUDIOPLAYBACK_H

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

// JUCE framework modules for audio processing and utilities
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

// Standard library utilities
#include <memory>
#include <vector>
#include <array>

// Qt for string handling and debugging
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QTimer>

// Windows API for shared memory
#include <windows.h>

// Project headers
#include "CircularBuffer.h"
#include "unitypage.h"

// -----------------------------------------------------------------------------
// AudioPlayback Class: Manages audio loading, playback, FFT analysis, and
//                       shared-memory communication for Unity embedding.
// -----------------------------------------------------------------------------
class AudioPlayback
{
public:
    // Constructor and Destructor
    AudioPlayback();
    ~AudioPlayback();

    // -------------------------------------------------------------------------
    // Basic Playback Controls
    // -------------------------------------------------------------------------

    // Load an audio file from disk, returns true if successful
    bool loadFile(const QString filePath);

    // Unload current file and reset transport
    void unloadFile();

    // Start playback
    void play();

    // Stop playback
    void stop();

    // Toggle between pause and resume
    void togglePause();

    // Replace the current track with a new one
    bool replaceTrack(const QString filePath);

    // Get the file path of the currently loaded track
    QString& getCurrentTrackPath();

    // Check if audio is playing
    bool isPlaying() const { return transportSource.isPlaying(); }

    // Get playback position (seconds)
    double getCurrentPosition() const { return transportSource.getCurrentPosition(); }

    // Get total track length (seconds)
    double getTrackLength() const { return transportSource.getLengthInSeconds(); }

    // Seek to a new position (seconds)
    void seek(double newPosition);

    // Check if any audio file is loaded
    bool hasAudioLoaded();

    // -------------------------------------------------------------------------
    // FFT & Frequency Analysis
    // -------------------------------------------------------------------------

    // Perform FFT on captured audio samples
    void performFFT();

    // Different frequency bands for visualization
    // enum FrequencyBand {
    //     SubBass = 0,
    //     Bass,
    //     LowMid,
    //     Mid,
    //     UpperMid,
    //     Presence,
    //     Brilliance,
    //     NumBands
    // };

    // // Defines the frequency range for each band
    // struct BandRange { float min; float max; };

    // // Static ranges for each band
    // static constexpr std::array<BandRange, NumBands> bandRanges = {{
    //     {20.0f, 60.0f},    // SubBass
    //     {60.0f, 250.0f},   // Bass
    //     {250.0f, 500.0f},  // LowMid
    //     {500.0f, 2000.0f}, // Mid
    //     {2000.0f, 4000.0f},// UpperMid
    //     {4000.0f, 6000.0f},// Presence
    //     {6000.0f, 20000.0f}// Brilliance
    // }};

    enum FrequencyBand {
        Band0 = 0,
        Band1,
        Band2,
        Band3,
        Band4,
        Band5,
        Band6,
        Band7,
        Band8,
        Band9,
        Band10,
        Band11,
        Band12,
        Band13,
        Band14,
        Band15,
        NumBands   // = 16
    };

    // Closed‑form, precomputed min/max for each band
    struct BandRange { float min; float max; };

    // static constexpr std::array<BandRange, NumBands> bandRanges = {{
    //     {   20.00f,   20.00f * 1.5422108f    }, // Band0:  20.00 –  30.84 Hz
    //     {   30.84f,   30.84f * 1.5422108f    }, // Band1:  30.84 –  47.52 Hz
    //     {   47.52f,   47.52f * 1.5422108f    }, // Band2:  47.52 –  73.23 Hz
    //     {   73.23f,   73.23f * 1.5422108f    }, // Band3:  73.23 – 112.92 Hz
    //     {  112.92f,  112.92f * 1.5422108f    }, // Band4: 112.92 – 174.06 Hz
    //     {  174.06f,  174.06f * 1.5422108f    }, // Band5: 174.06 – 268.19 Hz
    //     {  268.19f,  268.19f * 1.5422108f    }, // Band6: 268.19 – 413.23 Hz
    //     {  413.23f,  413.23f * 1.5422108f    }, // Band7: 413.23 – 636.76 Hz
    //     {  636.76f,  636.76f * 1.5422108f    }, // Band8: 636.76 – 981.23 Hz
    //     {  981.23f,  981.23f * 1.5422108f    }, // Band9: 981.23 –1513.70 Hz
    //     { 1513.70f, 1513.70f * 1.5422108f    }, // Band10:1513.70 –2337.90 Hz
    //     { 2337.90f, 2337.90f * 1.5422108f    }, // Band11:2337.90 –3617.10 Hz
    //     { 3617.10f, 3617.10f * 1.5422108f    }, // Band12:3617.10 –5597.60 Hz
    //     { 5597.60f, 5597.60f * 1.5422108f    }, // Band13:5597.60 –8660.20 Hz
    //     { 8660.20f, 8660.20f * 1.5422108f    }, // Band14:8660.20 –13405.20 Hz
    //     {13405.20f,             20000.0f      }  // Band15:13405.20 –20000.00 Hz
    // }};

    static constexpr std::array<BandRange, NumBands> bandRanges = {{
        // 14 bands from 20 → 500 Hz (ratio ≈ (500/20)^(1/14) ≈ 1.2585)
        {  20.00f,   25.17f },  // Band0
        {  25.17f,   31.68f },  // Band1
        {  31.68f,   39.86f },  // Band2
        {  39.86f,   50.17f },  // Band3
        {  50.17f,   63.14f },  // Band4
        {  63.14f,   79.46f },  // Band5
        {  79.46f,  100.00f },  // Band6
        { 100.00f,  125.85f },  // Band7
        { 125.85f,  158.38f },  // Band8
        { 158.38f,  199.32f },  // Band9
        { 199.32f,  250.85f },  // Band10
        { 250.85f,  315.69f },  // Band11
        { 315.69f,  397.30f },  // Band12
        { 397.30f,  500.00f },  // Band13

        // 2 bands from 500 → 20 kHz (ratio ≈ √(20000/500) ≈ 6.3249)
        { 500.00f, 3162.28f },  // Band14
        {3162.28f,20000.00f }   // Band15
    }};

    // Get level of a specific frequency band
    float getFrequencyBandLevel(FrequencyBand band) const;

    // Access to raw audio samples (for custom processing)
    float* getCurrentAudioSamples();

    // Access computed frequency band levels
    std::array<float, NumBands>& getFreqBands() { return frequencyBands; }

private:
    // -------------------------------------------------------------------------
    // Internal Helpers for FFT
    // -------------------------------------------------------------------------
    void analyzeFrequencyBands();

    static constexpr int fftOrder = 13;            // 2^13 = 8192 samples
    static constexpr int fftSize  = 1 << fftOrder; // FFT size

    juce::dsp::FFT fft;                            // FFT engine
    std::vector<float> fftData;                    // Buffer for FFT
    std::vector<float> fftWindow;                  // Window function (Hann)
    std::array<float, NumBands> frequencyBands;    // Average magnitudes per band

    // -------------------------------------------------------------------------
    // Audio Playback Internals
    // -------------------------------------------------------------------------
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    juce::AudioTransportSource transportSource;
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    QString currentTrackPath;

    // Buffer to store recent audio samples
    CircularBuffer sampleBuffer{fftSize};

    // -------------------------------------------------------------------------
    // CapturingAudioSource: Wraps another AudioSource to capture samples
    // -------------------------------------------------------------------------
    // Custom audio source that captures samples as they're played.
    class CapturingAudioSource : public juce::AudioSource
    {
    public:
        CapturingAudioSource(juce::AudioSource* sourceToWrap, CircularBuffer& bufferToFill, AudioPlayback* externalAudioPlayback)
            : wrappedSource(sourceToWrap),
            circularBuffer(bufferToFill),
            audioPlayback(externalAudioPlayback) { }
        void setSource(juce::AudioSource* newSource) { wrappedSource = newSource; }
        void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
        {
            if (wrappedSource != nullptr)
                wrappedSource->prepareToPlay(samplesPerBlockExpected, sampleRate);
        }
        void releaseResources() override
        {
            if (wrappedSource != nullptr)
                wrappedSource->releaseResources();
        }
        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
        {
            if (wrappedSource != nullptr)
                wrappedSource->getNextAudioBlock(bufferToFill);
            else
                bufferToFill.clearActiveBufferRegion();

            // Remove the processed samples from sampleBuffer.
            if (UnityPage::isUnityEmbedded()) {
                if (bufferToFill.buffer != nullptr && bufferToFill.buffer->getNumChannels() > 0)
                {
                    auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
                    int numSamples = bufferToFill.numSamples;
                    // Append samples to sampleBuffer
                    circularBuffer.pushSamples(channelData, static_cast<size_t>(numSamples));
                    // qDebug() << "Captured" << numSamples << "samples, total in buffer:" << circularBuffer.size();
                }

                performFFT();
            }
        }

        void performFFT(){
            // Remove the processed samples from sampleBuffer.
            if (circularBuffer.size() >= AudioPlayback::fftSize) {
                // Perform FFT analysis on the current audio samples.
                audioPlayback->performFFT();

                transferFreqData();
                writeFrequencyBands(audioPlayback->getFreqBands());
            }
        }

    private:
        // Initiating constants for shared memory
        static constexpr const wchar_t* SHM_NAME = L"Local\\FractalWaveFFT";
        static constexpr size_t BAND_COUNT = static_cast<size_t>(FrequencyBand::NumBands);
        static constexpr size_t SHM_SIZE   = BAND_COUNT * sizeof(float);

        // Call once at startup:
        HANDLE createOrOpenShm()
        {
            // INVALID_HANDLE_VALUE = use the system paging file
            HANDLE hMap = CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                (DWORD)SHM_SIZE,
                SHM_NAME
                );
            if (!hMap) {
                qDebug() << "CreateFileMapping failed:" << GetLastError();
            }
            return hMap;
        }

        // Call every frame
        void writeFrequencyBands(const std::array<float, BAND_COUNT>& frequencyBands)
        {
            static HANDLE hMap = createOrOpenShm();
            if (!hMap) return;

            // Map the memory into our address space
            LPVOID pBuf = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, SHM_SIZE);
            if (!pBuf) {
                qDebug() << "MapViewOfFile failed:" << GetLastError();
                return;
            }

            // Copy the raw floats into shared memory
            std::memcpy(pBuf, frequencyBands.data(), SHM_SIZE);

            // Optionally flush so Unity sees them immediately
            FlushViewOfFile(pBuf, SHM_SIZE);

            UnmapViewOfFile(pBuf);
        }

        void transferFreqData() {
            // Retrieve frequency band levels.
            float subBassLevel = audioPlayback->getFrequencyBandLevel(AudioPlayback::Band0);
            float bassLevel = audioPlayback->getFrequencyBandLevel(AudioPlayback::Band1);
            float lowMidLevel = audioPlayback->getFrequencyBandLevel(AudioPlayback::Band2);
            float midLevel = audioPlayback->getFrequencyBandLevel(AudioPlayback::Band3);
            float upperMidLevel = audioPlayback->getFrequencyBandLevel(AudioPlayback::Band4);
            float presenceLevel = audioPlayback->getFrequencyBandLevel(AudioPlayback::Band5);
            float brillianceLevel = audioPlayback->getFrequencyBandLevel(AudioPlayback::Band6);

            QString subBassLevelStr = QString::number(subBassLevel, 'f', 2);
            QString bassLevelStr = QString::number(bassLevel, 'f', 2);
            QString lowMidLevelStr = QString::number(lowMidLevel, 'f', 2);
            QString midLevelStr = QString::number(midLevel, 'f', 2);
            QString upperMidLevelStr = QString::number(upperMidLevel, 'f', 2);
            QString presenceLevelStr = QString::number(presenceLevel, 'f', 2);
            QString brillianceLevelStr = QString::number(brillianceLevel , 'f', 2);

            // Log the frequency band levels.
            qDebug() << "SubBass:" << subBassLevelStr
                     << "Bass:" << bassLevelStr
                     << "LowMid:" << lowMidLevelStr
                     << "Mid:" << midLevelStr
                     << "HighMid:" << upperMidLevelStr
                     << "Presence:" << presenceLevelStr
                     << "Treble:" << brillianceLevelStr;
        }

        juce::AudioSource* wrappedSource;
        CircularBuffer& circularBuffer;
        AudioPlayback* audioPlayback;
    } capturingSource; // Instance of our custom audio source
};

#endif // AUDIOPLAYBACK_H
