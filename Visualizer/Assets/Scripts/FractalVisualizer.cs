using UnityEngine;
using System;
using System.IO.MemoryMappedFiles;
using UnityEngine.UI;  // only if you still use UI.Text

public class FFTReader : IDisposable
{
    private const string SHM_NAME = "Local\\FractalWaveFFT";
    private const int BAND_COUNT = 16;
    private MemoryMappedFile mmf;
    private MemoryMappedViewAccessor accessor;
    private bool _isValid;

    public bool IsValid => _isValid;

    public FFTReader()
    {
        try
        {
            mmf = MemoryMappedFile.OpenExisting(SHM_NAME, MemoryMappedFileRights.Read);
            accessor = mmf.CreateViewAccessor(0, BAND_COUNT * sizeof(float), MemoryMappedFileAccess.Read);
            _isValid = true;
        }
        catch (Exception e)
        {
            // Could not open the MMF (not found, no rights, etc.)
            Debug.LogWarning($"FFTReader: shared memory '{SHM_NAME}' unavailable: {e.Message}");
            _isValid = false;
        }
    }

    public float[] ReadBands()
    {
        var bands = new float[BAND_COUNT];
        if (!_isValid) return bands;

        try
        {
            for (int i = 0; i < BAND_COUNT; ++i)
                bands[i] = accessor.ReadSingle(i * sizeof(float));
        }
        catch
        {
            // If any read fails, mark invalid so future calls are no‑ops
            _isValid = false;
        }
        return bands;
    }

    public void Dispose()
    {
        accessor?.Dispose();
        mmf?.Dispose();
        accessor = null;
        mmf = null;
        _isValid = false;
    }
}

public class FractalVisualizer : MonoBehaviour
{
    private FFTReader _fftReader;
    public Text[] bandTexts;  // if you still use UI.Text

    void Start()
    {
        _fftReader = new FFTReader();
    }

    void OnDestroy()
    {
        _fftReader?.Dispose();
    }

    void OnGUI()
    {
        float[] bands = _fftReader.ReadBands();
        for (int i = 0; i < bands.Length; ++i)
        {
            string label = _fftReader.IsValid
                ? $"Band {i}: {bands[i]:F2}"
                : $"Band {i}: (no data)";
            GUI.Label(new Rect(10, 10 + i * 20, 200, 20), label);
        }
    }
}
