using UnityEngine;
using System;
using System.IO.MemoryMappedFiles;
using UnityEngine.UI;  // for UI Text

public class FFTReader
{
    private const string SHM_NAME = "Local\\FractalWaveFFT";
    private const int BAND_COUNT = 7; // same as NumBands in C++
    private MemoryMappedFile mmf;
    private MemoryMappedViewAccessor accessor;

    public FFTReader()
    {
        // Open the existing mapping (must match C++ name)
        mmf = MemoryMappedFile.OpenExisting(SHM_NAME, MemoryMappedFileRights.Read);
        accessor = mmf.CreateViewAccessor(0, BAND_COUNT * sizeof(float), MemoryMappedFileAccess.Read);
    }

    public float[] ReadBands()
    {
        var bands = new float[BAND_COUNT];
        for (int i = 0; i < BAND_COUNT; ++i)
        {
            bands[i] = accessor.ReadSingle(i * sizeof(float));
        }
        return bands;
    }

    public void Dispose()
    {
        accessor?.Dispose();
        mmf?.Dispose();
    }
}

public class FractalVisualizer : MonoBehaviour
{
    private FFTReader _fftReader;
    // assign these in the Inspector: 7 Text elements for your 7 bands
    public Text[] bandTexts;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        _fftReader = new FFTReader();
    }

    // Update is called once per frame
    void Update()
    {
        // float[] bands = _fftReader.ReadBands();

        // // safety check
        // int count = Mathf.Min(bands.Length, bandTexts.Length);
        // for (int i = 0; i < count; ++i)
        // {
        //     // Format to 2 decimal places
        //     bandTexts[i].text = $"{bands[i]:F2}";
        // }
    }

    void OnGUI()
    {
        float[] bands = _fftReader.ReadBands();
        for (int i = 0; i < bands.Length; ++i)
        {
            // Draw at (10,10), (10,30), (10,50), etc.
            GUI.Label(new Rect(10, 10 + i * 20, 200, 20),
                    $"Band {i}: {bands[i]:F2}");
        }
    }
}
