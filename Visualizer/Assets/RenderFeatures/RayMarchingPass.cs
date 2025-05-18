using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.Rendering.RenderGraphModule;

class RayMarchingPass : ScriptableRenderPass
{
    private Material material;
    private readonly Mesh fullScreenMesh;
    RasterGraphContext context;

    private class PassData
    {
        public Material material;
        public Mesh fullScreenMesh;
    }

    // This static method is passed as the RenderFunc delegate to the RenderGraph render pass.
    // It is used to execute draw commands.
    static void ExecutePass(PassData data, RasterGraphContext context)
    {
        context.cmd.DrawMesh(data.fullScreenMesh, Matrix4x4.identity, data.material);
    }

    public RayMarchingPass(Material material)
    {
        this.material = material;

        // Create a full-screen quad or triangle mesh (identity UVs).
        // fullScreenMesh = CoreUtils.CreateFullScreenMesh();

        // In your RayMarchingPass constructor:
        fullScreenMesh = new Mesh { name = "FullScreenQuad" };

        // Four corners in clip‑space
        fullScreenMesh.vertices = new Vector3[] {
            new Vector3(-1f, -1f, 0f),  // bottom‑left
            new Vector3(-1f,  1f, 0f),  // top‑left
            new Vector3( 1f,  1f, 0f),  // top‑right
            new Vector3( 1f, -1f, 0f)   // bottom‑right
        };

        // Standard UVs
        fullScreenMesh.uv = new Vector2[] {
            new Vector2(0f, 0f),
            new Vector2(0f, 1f),
            new Vector2(1f, 1f),
            new Vector2(1f, 0f)
        };

        // Two triangles: (0,1,2) and (0,2,3)
        fullScreenMesh.triangles = new int[] {
            0, 1, 2,
            0, 2, 3
        };

        // Optional: recalc normals (not really needed for unlit full‑screen)
        fullScreenMesh.RecalculateNormals();

        // Upload and mark as immutable
        fullScreenMesh.UploadMeshData(true);

    }

    // RecordRenderGraph is where the RenderGraph handle can be accessed, through which render passes can be added to the graph.
    // FrameData is a context container through which URP resources can be accessed and managed.
    public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
    {
        const string passName = "RayMarchPass";

        // Declare a raster pass named "RayMarchPass"
        using (var builder = renderGraph.AddRasterRenderPass<PassData>(
            passName, out var passData))
        {
            // Get the camera's color target
            var urpResources = frameData.Get<UniversalResourceData>();
            builder.SetRenderAttachment(urpResources.activeColorTexture, 0);

            // Assign data to pass
            passData.material = material;
            passData.fullScreenMesh = fullScreenMesh;

            // Grab the current camera
            var camData = frameData.Get<UniversalCameraData>();
            var cam = camData.camera;

            // Compute and bind the matrices & position
            Matrix4x4 camToWorld = cam.cameraToWorldMatrix;
            Matrix4x4 invProj = cam.projectionMatrix.inverse;
            Vector3 camPos = cam.transform.position;

            Debug.Log($"camToWorld in RecordRenderGraph: {camToWorld}");
            Debug.Log($"invProj in RecordRenderGraph: {invProj}");
            Debug.Log($"camPos in RecordRenderGraph: {camPos}");

            // Execution: draw full-screen triangle
            builder.SetRenderFunc((PassData data, RasterGraphContext ctx) =>
            {
                data.material.SetMatrix("_CamToWorld", camToWorld);
                data.material.SetMatrix("_InvProj", invProj);
                data.material.SetVector("_CamPos", camPos);

                ExecutePass(data, ctx);
            });
        }
    }

}
