Shader "Unlit/RayMarching"
{
    Properties
    {
        _MaxDistance    ("Max Ray Distance", Float) = 100.0
        _MaxIterations  ("Max Iterations", Int)    = 100
        _Precision      ("Distance Precision", Float) = 0.001
        _ObjectColor    ("Object Color", Color)    = (1,1,1,1)
        _LightDirection("Light Direction", Vector) = (0,1,0,0)
        _SpherePosition ("Sphere Position", Vector) = (0,0,3,0)
        _SphereRadius   ("Sphere Radius", Float)    = 1.0
        _CellSize ("Repetition Cell Size", Vector) = (4,4,4,0)
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" "RenderPipeline"="UniversalPipeline" }
        Cull Off ZWrite Off ZTest Always LOD 100

        Pass
        {
            HLSLPROGRAM
            #pragma vertex Vert
            #pragma fragment Frag
            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"

            struct Attributes
            {
                float3 position : POSITION;
                float2 uv       : TEXCOORD0;
            };

            struct Varyings
            {
                float4 position : SV_POSITION;
                float2 uv       : TEXCOORD0;
            };

            // Camera data (set from C#)
            float4x4 _CamToWorld;
            float4x4 _InvProj;
            float3   _CamPos;

            // Ray march settings
            float   _MaxDistance;
            int     _MaxIterations;
            float   _Precision;

            // Scene and lighting
            float4 _ObjectColor;
            float4 _LightDirection;
            float4 _SpherePosition;
            float  _SphereRadius;
            float4 _CellSize;

            // Vertex: passthrough
            Varyings Vert(Attributes IN)
            {
                Varyings OUT;
                OUT.position = float4(IN.position, 1.0);
                OUT.uv       = IN.uv;
                return OUT;
            }

            // Signed distance for sphere
            float sdSphere(float3 p, float3 center, float radius)
            {
                return length(p - center) - radius;
            }

            // Complete scene SDF
            float sceneSDF(float3 p)
            {
                float3 cell = _CellSize.xyz;

                // fold into repeating cell with fmod
                float3 q = fmod(p + 0.5 * cell, cell) - 0.5 * cell;

                // now evaluate your sphere at the origin of each cell
                return sdSphere(q - _SpherePosition.xyz, float3(0,0,0), _SphereRadius);
            }

            // Estimate normal via central differences
            float3 estimateNormal(float3 p)
            {
                const float eps = 0.001;
                float dx = sceneSDF(p + float3(eps,0,0)) - sceneSDF(p - float3(eps,0,0));
                float dy = sceneSDF(p + float3(0,eps,0)) - sceneSDF(p - float3(0,eps,0));
                float dz = sceneSDF(p + float3(0,0,eps)) - sceneSDF(p - float3(0,0,eps));
                return normalize(float3(dx,dy,dz));
            }

            // Ray marching returns (distance, hitFlag)
            float2 rayMarch(float3 ro, float3 rd)
            {
                float dist = 0.0;
                for (int i = 0; i < _MaxIterations; i++)
                {
                    float3 p = ro + rd * dist;
                    float d = sceneSDF(p);
                    if (d < _Precision)
                        return float2(dist, 1.0);
                    if (dist > _MaxDistance)
                        break;
                    dist += d;
                }
                return float2(dist, 0.0);
            }

            // Fragment: compute ray, march, shade
            half4 Frag(Varyings IN) : SV_Target
            {
                // Compute NDC UV -> view space
                float2 uv = IN.uv * 2.0 - 1.0;                    // [0,1]→[-1,1]
                float4 clip = float4(uv, 1.0, 1.0);
                float4 viewPos = mul(_InvProj, clip);
                viewPos.xyz /= viewPos.w;                         // perspective divide
                float3 rd = normalize(mul((float3x3)_CamToWorld, viewPos.xyz));

                float3 ro = _CamPos;

                // Background color
                float3 col = 0;

                // Ray march
                float2 rm = rayMarch(ro, rd);
                if (rm.y > 0.5)
                {
                    float3 pos = ro + rd * rm.x;
                    float3 N   = estimateNormal(pos);

                    float3 L   = normalize(_LightDirection.xyz);
                    float diff = saturate(dot(N, L));
                    float amb  = 0.1;
                    col = _ObjectColor.rgb * (amb + diff);
                }

                return float4(col, 1.0);
            }
            ENDHLSL
        }
    }
}