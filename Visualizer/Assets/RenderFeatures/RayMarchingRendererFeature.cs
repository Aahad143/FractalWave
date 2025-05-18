using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.Rendering.RenderGraphModule;

public class RayMarchingRendererFeature : ScriptableRendererFeature
{
    // [SerializeField] private Shader shader;
    [SerializeField] private Material material;
    RayMarchingPass rayMarchPass;

    public override void Create()
    {
        if (material == null)
        {
            Debug.LogWarning("RayMarchingRendererFeature: Missing ray march shader.");
            return;
        }
        // material = new Material(shader);
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
        renderer.EnqueuePass(rayMarchPass);
    }

    protected override void Dispose(bool disposing)
    {
        if (Application.isPlaying)
        {
            Destroy(material);
        }
        else
        {
            DestroyImmediate(material);
        }
    }
}
