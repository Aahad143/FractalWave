Shader "Unlit/InfiniteOctahedrons"
{
    Properties
    {
        _MaxDistance     ("Max Ray Distance", Float)  = 150.0
        _MaxIterations   ("Max Iterations", Int)     = 100
        _Precision       ("Distance Precision", Float)= 0.001
        _ObjectColor     ("Object Color", Color)     = (1,1,1,1)
        _OctahedronSize  ("Octahedron Half‑Size", Float)= 0.15
        _CellSize        ("Repetition Cell Size", Vector)= (1,1,0,0)
        _ForwardSpeed    ("Forward Speed (Z)", Float) = 100.0
        
        // Fog properties
        _FogColor        ("Fog Color", Color)        = (0.1,0.2,0.3,1)
        _FogDensity      ("Fog Density", Range(0,0.1)) = 0.01
        _FogStart        ("Fog Start Distance", Float) = 5.0
        _FogColorSpeed   ("Fog Color Change Speed", Range(0.001, 10.0)) = 0.01
        
        // Audio frequency properties
        _FreqBands       ("Frequency Bands", Vector)     = (0,0,0,0)
        _FreqBands4      ("Frequency Bands 4‑7", Vector) = (0,0,0,0)
        _FreqBands8      ("Frequency Bands 8-11", Vector) = (0,0,0,0)
        _FreqBands12     ("Frequency Bands 12-15", Vector) = (0,0,0,0)
        
        // Activity tracking properties
        _ActivityThreshold ("Activity Threshold", Range(0.01, 1.0)) = 0.1
        _ActivityDecay     ("Activity Decay Rate", Range(0.01, 0.5)) = 0.05
        _RandomWeightFactor ("Random Weight Factor", Range(0.1, 10.0)) = 5.0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" "RenderPipeline"="UniversalPipeline" }
        Cull Off ZWrite Off ZTest Always

        Pass
        {
            HLSLPROGRAM
            #pragma vertex Vert
            #pragma fragment Frag
            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"
            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/ShaderVariablesFunctions.hlsl"

            // Vertex → Fragment data
            struct Attributes { float3 pos : POSITION; float2 uv : TEXCOORD0; };
            struct Varyings  { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

            // Camera (set from C#)
            float4x4 _CamToWorld;
            float4x4 _InvProj;
            float3   _CamPos;

            // Ray‑march params
            float  _MaxDistance;
            int    _MaxIterations;
            float  _Precision;

            // Octahedron and tiling
            float4 _ObjectColor;
            float  _OctahedronSize;
            float4 _CellSize;      // only xyz used
            float  _ForwardSpeed;  // controls how fast the scene moves along Z
            
            // Fog parameters
            float4 _FogColor;
            float  _FogDensity;
            float  _FogStart;
            float  _FogColorSpeed;
            
            // Audio frequency bands
            float4 _FreqBands;     // bands[0]=x, [1]=y, [2]=z, [3]=w
            float4 _FreqBands4;    // bands[4]=x, [5]=y, [6]=z, [7]=w
            float4 _FreqBands8;    // bands[8]=x, [9]=y, [10]=z, [11]=w
            float4 _FreqBands12;   // bands[12]=x, [13]=y, [14]=z, [15]=w
            
            // Activity tracking
            float _ActivityThreshold;
            float _ActivityDecay;
            float _RandomWeightFactor;

            // Static variables to store previous values for smoothing
            static float3 prevFogColorShift = float3(0,0,0);
            static float previousFreqValues[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            static float bandActivity[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            static float bandWeights[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; // Starting with equal weights
            
            Varyings Vert(Attributes IN)
            {
                Varyings OUT;
                OUT.pos = float4(IN.pos, 1);
                OUT.uv  = IN.uv;
                return OUT;
            }

            // Sigmoid helper function
            float Sigmoid(float x, float k)
            {
                // Logistic: 1 / (1 + e^(–k * x))
                return 1.0 / (1.0 + exp(-k * x));
            }

            // Octahedron SDF: distance to center‑at‑0, size s
            float sdOctahedron(float3 p, float s)
            {
                p = abs(p);
                // (x+y+z - s) / sqrt(3)
                return (p.x + p.y + p.z - s) * 0.57735027;
            }
            
            // Hash function for pseudo-random numbers
            float hash(float3 p)
            {
                p = frac(p * float3(443.897, 441.423, 437.195));
                p += dot(p, p.yzx + 19.19);
                return frac((p.x + p.y) * p.z);
            }
            
            // Weighted random selection based on activity
            int getWeightedRandomFreqIndex(float3 cellPosition)
            {
                // Get current frequency values
                float freqValues[16] = {
                    _FreqBands.x, _FreqBands.y, _FreqBands.z, _FreqBands.w,
                    _FreqBands4.x, _FreqBands4.y, _FreqBands4.z, _FreqBands4.w,
                    _FreqBands8.x, _FreqBands8.y, _FreqBands8.z, _FreqBands8.w,
                    _FreqBands12.x, _FreqBands12.y, _FreqBands12.z, _FreqBands12.w
                };
                
                // Update activity levels for each frequency band
                float sumActivity = 0;
                for (int i = 0; i < 16; i++) {
                    // Calculate change from previous frame
                    float delta = abs(freqValues[i] - previousFreqValues[i]);
                    
                    // If change exceeds threshold, increase activity
                    if (delta > _ActivityThreshold) {
                        bandActivity[i] = min(bandActivity[i] + delta, 1.0);
                    } else {
                        // Decay activity over time
                        bandActivity[i] = max(bandActivity[i] - _ActivityDecay, 0.0);
                    }
                    
                    // Update previous value for next frame
                    previousFreqValues[i] = freqValues[i];
                    
                    // Calculate weight based on activity (higher activity = higher weight)
                    bandWeights[i] = 1.0 + bandActivity[i] * _RandomWeightFactor;
                    sumActivity += bandWeights[i];
                }
                
                // Use cell position to generate a stable random value for this cell
                float randomVal = hash(cellPosition);
                
                // Normalize weights and select based on random value
                float cumProb = 0;
                for (int i = 0; i < 16; i++) {
                    float normalizedWeight = bandWeights[i] / sumActivity;
                    cumProb += normalizedWeight;
                    if (randomVal <= cumProb) {
                        return i;
                    }
                }
                
                // Fallback
                return int(randomVal * 16) % 16;
            }

            // Scene SDF with infinite repetition
            float sceneSDF(float3 p)
            {
                // Animate forward movement
                p.z += _Time.y * _ForwardSpeed;

                // XY repetition
                float2 cellXY = _CellSize.xy;
                float2 cellPos = floor(p.xy / cellXY + 0.5);
                p.xy = p.xy - cellXY * cellPos;

                // Calculate Z cell index before wrapping
                float cellZ = _CellSize.z;
                float originalZ = p.z;
                float cellIndexZ = cellZ > 0 ? floor(originalZ / cellZ + 0.5) : 0;
                
                // Store original cell position for consistent random selection
                float3 originalCellPos = float3(cellPos.x, cellPos.y, cellIndexZ);
                
                // Z repetition
                if(cellZ > 0)
                    p.z = p.z - cellZ * floor(p.z / cellZ + 0.5);
                
                // Get weighted random frequency index for this cell
                int freqIndex = getWeightedRandomFreqIndex(originalCellPos);
                
                // Get frequency value for this cell
                float freqValues[16] = {
                    _FreqBands.x, _FreqBands.y, _FreqBands.z, _FreqBands.w,
                    _FreqBands4.x, _FreqBands4.y, _FreqBands4.z, _FreqBands4.w,
                    _FreqBands8.x, _FreqBands8.y, _FreqBands8.z, _FreqBands8.w,
                    _FreqBands12.x, _FreqBands12.y, _FreqBands12.z, _FreqBands12.w
                };
                float freq = freqValues[freqIndex];
                
                // Apply sigmoid to get 0-1 range
                float kSteepness = 0.08; // Adjust for sharper transition
                float normalizedFreq = Sigmoid(freq, kSteepness);
                
                // Map to size range
                float dynamicSize = _OctahedronSize * saturate(normalizedFreq);

                return sdOctahedron(p, normalizedFreq - 0.5);
            }

            // Estimate normal via central‑difference
            float3 estimateNormal(float3 p)
            {
                const float eps = 0.001;
                float dx = sceneSDF(p + float3(eps,0,0)) - sceneSDF(p - float3(eps,0,0));
                float dy = sceneSDF(p + float3(0,eps,0)) - sceneSDF(p - float3(0,eps,0));
                float dz = sceneSDF(p + float3(0,0,eps)) - sceneSDF(p - float3(0,0,eps));
                return normalize(float3(dx,dy,dz));
            }

            // Calculate fog factor based on distance
            float calculateFog(float distance)
            {
                // Only apply fog after the start distance
                float fogDistance = max(0, distance - _FogStart);
                
                // Exponential fog: exp(-density * distance)
                return exp(-_FogDensity * fogDistance);
            }
            
            // Perlin-like noise function 
            float perlinNoise(float2 st) 
            {
                float2 i = floor(st);
                float2 f = frac(st);
                
                float a = sin(dot(i, float2(127.1, 311.7)));
                float b = sin(dot(i + float2(1.0, 0.0), float2(127.1, 311.7)));
                float c = sin(dot(i + float2(0.0, 1.0), float2(127.1, 311.7)));
                float d = sin(dot(i + float2(1.0, 1.0), float2(127.1, 311.7)));
                
                // Smooth interpolation
                float2 u = f * f * (3.0 - 2.0 * f);
                
                // Mix 4 corners
                return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y) * 0.5 + 0.5;
            }
            
            // Audio-reactive fog color with time-based smoothing
            float3 getAudioReactiveFogColor() 
            {
                // Base fog color
                // float3 fogCol = _FogColor.rgb;
                float3 fogCol = float3(0,0,0);
                
                // Calculate slow-moving time values 
                float slowTime = _Time * _FogColorSpeed;
                
                // Generate smooth flowing noise patterns at different frequencies
                float noiseR = perlinNoise(float2(slowTime * 0.5 + 200.0, slowTime * 0.4));
                float noiseG = perlinNoise(float2(slowTime * 0.3 + 100.0, slowTime * 0.5));
                float noiseB = perlinNoise(float2(slowTime * 0.4, slowTime * 0.3));
                
                // Get frequency bands but apply heavy damping for stability
                float highValue = (_FreqBands8.x + _FreqBands8.y + _FreqBands8.z + _FreqBands8.w) * 0.1;
                float midValue = (_FreqBands4.z + _FreqBands4.w + _FreqBands4.z + _FreqBands4.w) * 0.1;
                float bassValue = (_FreqBands.x + _FreqBands.y + _FreqBands.z + _FreqBands.w) * 0.1;


                // float bandValue1 = (_FreqBands.x + _FreqBands.y) * 0.9;
                // float bandValue2 = (_FreqBands.z + _FreqBands.w) * 0.9;
                // float bandValue3 = (_FreqBands4.x + _FreqBands4.y) * 0.9;
                // float bandValue4 = (_FreqBands4.z + _FreqBands4.w) * 0.9;
                // float bandValue5 = (_FreqBands8.x + _FreqBands8.y) * 0.9;
                // float bandValue6 = (_FreqBands8.z + _FreqBands8.w) * 0.9;
                // float bandValue7 = (_FreqBands12.x + _FreqBands12.y) * 0.9;
                
                // Modulate the noise patterns with audio frequencies
                float blueShift = noiseB * 0.1 + bassValue * 0.03;
                float greenShift = noiseG * 0.1 + midValue * 0.03;
                float redShift = noiseR * 0.1 + highValue * 0.03;
                
                // Combine into color shift
                float3 targetColorShift = float3(redShift, greenShift, blueShift);
                
                // CRITICAL: Extremely slow interpolation between current and previous color shifts
                // This creates smooth, gradual transitions (approximately 10% change per frame at 60fps)
                float3 smoothedColorShift = lerp(prevFogColorShift, targetColorShift, 0.005);
                
                // Store for next frame
                prevFogColorShift = smoothedColorShift;
                
                return saturate(fogCol + smoothedColorShift);
            }

            // Audio-reactive fog color with time-based smoothing
            // float3 getAudioReactiveFogColor() 
            // {
            //     // Base fog color
            //     // float3 fogCol = _FogColor.rgb;
            //     float3 fogCol = float3(1,1,1);
                
            //     // Calculate slow-moving time values 
            //     float slowTime = _Time * _FogColorSpeed;
                
            //     // Generate smooth flowing noise patterns at different frequencies
            //     float noiseR = perlinNoise(float2(slowTime * 0.5 + 200.0, slowTime * 0.4));
            //     float noiseG = perlinNoise(float2(slowTime * 0.3 + 100.0, slowTime * 0.5));
            //     float noiseB = perlinNoise(float2(slowTime * 0.4, slowTime * 0.3));
                
            //     // Get frequency bands but apply heavy damping for stability
            //     float highValue = (_FreqBands8.x + _FreqBands8.y + _FreqBands8.z + _FreqBands8.w) * 0.4;
            //     float midValue = (_FreqBands4.z + _FreqBands4.w + _FreqBands4.z + _FreqBands4.w) * 0.4;
            //     float bassValue = (_FreqBands.x + _FreqBands.y + _FreqBands.z + _FreqBands.w) * 0.4;


            //     // float bandValue1 = (_FreqBands.x + _FreqBands.y) * 0.9;
            //     // float bandValue2 = (_FreqBands.z + _FreqBands.w) * 0.9;
            //     // float bandValue3 = (_FreqBands4.x + _FreqBands4.y) * 0.9;
            //     // float bandValue4 = (_FreqBands4.z + _FreqBands4.w) * 0.9;
            //     // float bandValue5 = (_FreqBands8.x + _FreqBands8.y) * 0.9;
            //     // float bandValue6 = (_FreqBands8.z + _FreqBands8.w) * 0.9;
            //     // float bandValue7 = (_FreqBands12.x + _FreqBands12.y) * 0.9;
                
            //     // Modulate the noise patterns with audio frequencies
            //     float greenShift = noiseG * 0.1 + bassValue * 0.03;
            //     float blueShift = noiseB * 0.1 + midValue * 0.03;
            //     float redShift = noiseR * 0.1 + highValue * 0.03;
                
            //     // Combine into color shift
            //     float3 targetColorShift = float3(fogCol.x-redShift, fogCol.y-greenShift, fogCol.z-blueShift);
                
            //     // CRITICAL: Extremely slow interpolation between current and previous color shifts
            //     // This creates smooth, gradual transitions (approximately 10% change per frame at 60fps)
            //     float3 smoothedColorShift = lerp(prevFogColorShift, targetColorShift, 0.01);
                
            //     // Store for next frame
            //     prevFogColorShift = smoothedColorShift;
                
            //     return saturate(fogCol + smoothedColorShift);
            // }

            // Ray‑march loop
            float2 rayMarch(float3 ro, float3 rd)
            {
                float dist = 0;
                for (int i = 0; i < _MaxIterations; i++)
                {
                    float3 p = ro + rd * dist;
                    float d  = sceneSDF(p);
                    if (d < _Precision)    return float2(dist,1);
                    if (dist > _MaxDistance) break;
                    dist += d;
                }
                return float2(dist,0);
            }

            half4 Frag(Varyings IN) : SV_Target
            {
                // 1) Reconstruct ray
                float2 uv   = IN.uv * 2 - 1;                       
                float4 clip = float4(uv, 1, 1);
                float4 view = mul(_InvProj, clip);
                view.xyz   /= view.w;
                float3 rd   = normalize(mul((float3x3)_CamToWorld, view.xyz));
                float3 ro   = _CamPos;

                // 2) March
                float2 mr = rayMarch(ro, rd);
                float distance = mr.x;
                
                // 3) Shade with fog
                float3 col = 0;
                
                // Get fog color
                float3 fogCol = getAudioReactiveFogColor();
                
                if (mr.y > 0.5) // Hit something
                {
                    // Basic lighting
                    float3 objectCol = _ObjectColor.rgb;
                    
                    // Calculate fog factor (1=no fog, 0=full fog)
                    float fogFactor = calculateFog(distance);
                    
                    // Mix object color with fog color based on fog factor
                    col = lerp(fogCol, objectCol, fogFactor);
                }
                else // Nothing hit, just fog
                {
                    // For background, use fog that gets denser with distance
                    float skyFogFactor = calculateFog(distance * 0.5); // Adjust fog for sky
                    col = lerp(fogCol, float3(0,0,0), skyFogFactor); // Blend to black or fogCol
                }
                
                return float4(col, 1);
            }
            ENDHLSL
        }
    }
}