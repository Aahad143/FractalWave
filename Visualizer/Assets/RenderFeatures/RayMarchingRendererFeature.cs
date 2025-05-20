using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.Rendering.RenderGraphModule;

public class RayMarchingRendererFeature : ScriptableRendererFeature
{
    // [SerializeField] private Shader shader;
    [SerializeField] private Material material;
    private FFTReader _fftReader;
    RayMarchingPass rayMarchPass;

    public override void Create()
    {
        if (material == null)
        {
            Debug.LogWarning("RayMarchingRendererFeature: Missing ray march shader.");
            return;
        }
        // material = new Material(shader);
        _fftReader = new FFTReader();
        rayMarchPass = new RayMarchingPass(material);

        rayMarchPass.renderPassEvent = RenderPassEvent.BeforeRenderingPostProcessing;
    }

    // Here you can inject one or multiple render passes in the renderer.
    // This method is called when setting up the renderer once per-camera.
    public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData renderingData)
    {
        if (rayMarchPass == null)
        {
            return;
        }
        float[] bands = _fftReader.ReadBands();
        // set into material
        material.SetVector("_FreqBands", new Vector4(bands[0], bands[1], bands[2], bands[3]));
        material.SetVector("_FreqBands4", new Vector4(bands[4], bands[5], bands[6], bands[7]));
        material.SetVector("_FreqBands8", new Vector4(bands[8], bands[9], bands[10], bands[11]));
        material.SetVector("_FreqBands12", new Vector4(bands[12], bands[13], bands[14], bands[15]));
        renderer.EnqueuePass(rayMarchPass);
    }

    protected override void Dispose(bool disposing)
    {
        if (Application.isPlaying)
        {
            Destroy(material);
            // Dispose the FFT reader to close the native handles
            _fftReader?.Dispose();
            _fftReader = null;
        }
        else
        {
            // DestroyImmediate(material);
        }
    }
}
