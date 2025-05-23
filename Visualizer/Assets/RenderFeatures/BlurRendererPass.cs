using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEngine.Rendering.RenderGraphModule.Util;

class BlurRenderPass : ScriptableRenderPass
{
    private BlurSettings defaultSettings;
    private Material material;
    private RenderTextureDescriptor blurTextureDescriptor;

    private static readonly int horizontalBlurId = Shader.PropertyToID("_HorizontalBlur");
    private static readonly int verticalBlurId = Shader.PropertyToID("_VerticalBlur");
    private const string k_BlurTextureName = "_BlurTexture";
    private const string k_VerticalPassName = "VerticalBlurRenderPass";
    private const string k_HorizontalPassName = "HorizontalBlurRenderPass";
    public BlurRenderPass(Material material, BlurSettings defaultSettings)
    {
        this.material = material;
        this.defaultSettings = defaultSettings;

        blurTextureDescriptor = new RenderTextureDescriptor(Screen.width, Screen.height,
        RenderTextureFormat.Default, 0);
    }

    private void UpdateBlurSettings()
    {
        if (material == null) return;

        // Use the Volume settings or the default settings if no Volume is set.
        // Debug.Log("CustomVolumeComponent is being used");
        var volumeComponent =
            VolumeManager.instance.stack.GetComponent<CustomVolumeComponent>();

        // Debug.Log($"volumeComponent.horizontalBlur.overrideState: {volumeComponent.horizontalBlur.overrideState}");
        // Debug.Log($"volumeComponent.horizontalBlur.value: {volumeComponent.horizontalBlur.value}");
        // Debug.Log($"defaultSettings.horizontalBlur: {defaultSettings.horizontalBlur}");

        float horizontalBlur = volumeComponent.horizontalBlur.overrideState ?
            volumeComponent.horizontalBlur.value : defaultSettings.horizontalBlur;
        float verticalBlur = volumeComponent.verticalBlur.overrideState ?
            volumeComponent.verticalBlur.value : defaultSettings.verticalBlur;

        // Debug.Log($"horizontalBlur in UpdateBlurSettings: {horizontalBlur}");
        // Debug.Log($"verticalBlur in UpdateBlurSettings: {verticalBlur}");
        material.SetFloat(horizontalBlurId, horizontalBlur);
        material.SetFloat(verticalBlurId, verticalBlur);
    }

    // RecordRenderGraph is where the RenderGraph handle can be accessed, through which render passes can be added to the graph.
    // FrameData is a context container through which URP resources can be accessed and managed.
    public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
    {
        // const string passName = "Render Custom Pass";

        // UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();
        UniversalResourceData resourceData = frameData.Get<UniversalResourceData>();

        UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();

        // The following line ensures that the render pass doesn't blit
        // from the back buffer.
        if (resourceData.isActiveTargetBackBuffer)
            return;

        // Set the blur texture size to be the same as the camera target size.
        blurTextureDescriptor.width = cameraData.cameraTargetDescriptor.width;
        blurTextureDescriptor.height = cameraData.cameraTargetDescriptor.height;
        blurTextureDescriptor.depthBufferBits = 0;

        TextureHandle srcCamColor = resourceData.activeColorTexture;
        TextureHandle dst = UniversalRenderer.CreateRenderGraphTexture(renderGraph,
            blurTextureDescriptor, k_BlurTextureName, false);

        // Update the blur settings in the material
        UpdateBlurSettings();

        // This check is to avoid an error from the material preview in the scene
        if (!srcCamColor.IsValid() || !dst.IsValid())
            return;

        // The AddBlitPass method adds a vertical blur render graph pass that blits from the source texture (camera color in this case) to the destination texture using the first shader pass (the shader pass is defined in the last parameter).
        RenderGraphUtils.BlitMaterialParameters paraVertical = new(srcCamColor, dst, material, 0);
        renderGraph.AddBlitPass(paraVertical, k_VerticalPassName);

        // The AddBlitPass method adds a horizontal blur render graph pass that blits from the texture written by the vertical blur pass to the camera color texture. The method uses the second shader pass.
        RenderGraphUtils.BlitMaterialParameters paraHorizontal = new(dst, srcCamColor, material, 1);
        renderGraph.AddBlitPass(paraHorizontal, k_HorizontalPassName);
    }

}