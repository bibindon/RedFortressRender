float4x4 g_matView;
float4x4 g_matProj;

float g_fNear = 0.1f;
float g_fFar = 50.0f;
float2 g_invSize;
float g_sampleRadius = 1.0f;
int g_sampleCount = 16;
bool g_enableDepthScaledSampleDistance = false;
bool g_useThickness = true;
float g_indirectLightStrength = 1.0f;
float g_indirectLightMaxContribution = 1.0f;
float g_thicknessScale = 1.0f;
float g_depthCompareThreshold = 0.0f;
float g_sampleDepthBiasDistance = 0.1f;
float g_targetNormalBiasScale = 1.0f;
float g_targetDepthBiasScale = 1.0f;
float g_minThickness = 0.10f;

texture texZ;
texture texNormal;
texture texThickness;
texture texGI;
texture texColor;

sampler sampZ = sampler_state
{
    Texture = (texZ);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler sampNormal = sampler_state
{
    Texture = (texNormal);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler sampThickness = sampler_state
{
    Texture = (texThickness);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler sampGI = sampler_state
{
    Texture = (texGI);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler sampColor = sampler_state
{
    Texture = (texColor);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VS_OUT
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUT VS_Fullscreen(float4 p : POSITION, float2 uv : TEXCOORD0)
{
    VS_OUT outputData;
    outputData.pos = p;
    outputData.uv = uv + 0.5f * g_invSize;
    return outputData;
}

float Random01(float2 seed)
{
    return frac(sin(dot(seed, float2(12.9898f, 78.233f))) * 43758.5453f);
}

float GetViewDepth(float2 uv)
{
    float linearZ = tex2D(sampZ, uv).r;
    return linearZ * (g_fFar - g_fNear) + g_fNear;
}

float GetViewDepthLod0(float2 uv)
{
    float linearZ = tex2Dlod(sampZ, float4(uv, 0.0f, 0.0f)).r;
    return linearZ * (g_fFar - g_fNear) + g_fNear;
}

float3 DecodeWorldNormal(float3 encodedNormal)
{
    return normalize(encodedNormal * 2.0f - 1.0f);
}

float3 GetViewNormal(float2 uv)
{
    float3 worldNormal = DecodeWorldNormal(tex2D(sampNormal, uv).rgb);
    return normalize(mul(float4(worldNormal, 0.0f), g_matView).xyz);
}

float3 GetViewPosition(float2 uv)
{
    float viewDepth = GetViewDepth(uv);
    float2 ndc = float2(uv.x * 2.0f - 1.0f,
                        1.0f - uv.y * 2.0f);
    return float3(ndc.x * viewDepth / g_matProj._11,
                  ndc.y * viewDepth / g_matProj._22,
                  viewDepth);
}

float2 ProjectViewPositionToTexCoord(float3 viewPosition)
{
    float4 clip = mul(float4(viewPosition, 1.0f), g_matProj);
    float clipW = max(clip.w, 0.0001f);
    float2 ndc = clip.xy / clipW;
    return float2(ndc.x * 0.5f + 0.5f,
                  0.5f - ndc.y * 0.5f) + 0.5f * g_invSize;
}

void BuildViewSpaceBasis(float3 normal, out float3 tangent, out float3 bitangent)
{
    float3 helperAxis = float3(0.0f, 0.0f, 1.0f);
    if (abs(normal.z) > 0.999f)
    {
        helperAxis = float3(1.0f, 0.0f, 0.0f);
    }

    tangent = normalize(cross(helperAxis, normal));
    bitangent = normalize(cross(normal, tangent));
}

float3 SampleRandomHemisphereDirection(float3 normal, float2 randomPair)
{
    const float kTwoPi = 6.2831853f;
    float azimuth = kTwoPi * randomPair.x;
    float cosTheta = randomPair.y;
    float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));

    float3 localDirection = float3(cos(azimuth) * sinTheta,
                                   sin(azimuth) * sinTheta,
                                   cosTheta);

    float3 tangent = float3(0.0f, 0.0f, 0.0f);
    float3 bitangent = float3(0.0f, 0.0f, 0.0f);
    BuildViewSpaceBasis(normal, tangent, bitangent);

    float3 sampleDirection = tangent * localDirection.x +
                             bitangent * localDirection.y +
                             normal * localDirection.z;
    return normalize(sampleDirection);
}

float ComputeDepthAwareBlurWeight(float centerDepth, float sampleDepth)
{
    if (centerDepth >= g_fFar || sampleDepth >= g_fFar)
    {
        return 0.0f;
    }

    float depthDifferenceMeters = abs(sampleDepth - centerDepth);
    float depthToleranceMeters = max(0.01f, centerDepth * 0.08f);
    float normalizedDifference = depthDifferenceMeters / depthToleranceMeters;
    if (normalizedDifference >= 1.0f)
    {
        return 0.0f;
    }

    float closeness = 1.0f - normalizedDifference;
    return closeness * closeness;
}

float ComputeNormalAwareBlurWeight(float3 centerNormal, float3 sampleNormal)
{
    float normalAlignment = dot(centerNormal, sampleNormal);
    if (normalAlignment <= 0.55f)
    {
        return 0.0f;
    }

    float normalizedDifference = saturate((1.0f - normalAlignment) / 0.45f);
    float closeness = 1.0f - normalizedDifference;
    return closeness * closeness;
}

float ComputeBlurSampleWeight(float baseWeight,
                              float centerDepth,
                              float sampleDepth,
                              float3 centerNormal,
                              float3 sampleNormal)
{
    float depthWeight = ComputeDepthAwareBlurWeight(centerDepth, sampleDepth);
    float normalWeight = ComputeNormalAwareBlurWeight(centerNormal, sampleNormal);
    return baseWeight * depthWeight * normalWeight;
}

void ComputeGiSample(float2 baseUv,
                     float currentDepth,
                     float3 currentNormal,
                     float3 currentViewPosition,
                     float sampleIndex,
                     float sampleCount,
                     out float occluded,
                     out float valid,
                     out float3 hitColor)
{
    occluded = 0.0f;
    valid = 0.0f;
    hitColor = float3(0.0f, 0.0f, 0.0f);

    float2 sampleSeed = baseUv / g_invSize + float2(sampleIndex * 13.37f,
                                                     sampleIndex * 7.91f);
    float2 randomPair = float2(Random01(sampleSeed + float2(17.0f, 59.4f)),
                               Random01(sampleSeed + float2(83.1f, 11.7f)));
    float3 sampleDirection = SampleRandomHemisphereDirection(currentNormal, randomPair);

    float distanceScale = 1.0f;
    if (sampleCount > 1.0f)
    {
        float randomDistance = Random01(sampleSeed + float2(31.0f, 97.0f));
        distanceScale = randomDistance * randomDistance * randomDistance;
    }

    if (g_enableDepthScaledSampleDistance)
    {
        float normalizedDepth = saturate((currentDepth - g_fNear) / max(g_fFar - g_fNear, 0.0001f));
        float depthScale = 1.0f + saturate(1.0f - normalizedDepth);
        distanceScale *= depthScale;
    }

    float sampleDistance = g_sampleRadius * distanceScale;
    float3 sampleViewPosition = currentViewPosition + sampleDirection * sampleDistance;
    if (sampleViewPosition.z <= 0.0f)
    {
        return;
    }

    float2 sampleUv = ProjectViewPositionToTexCoord(sampleViewPosition);
    if (sampleUv.x < 0.0f || sampleUv.x > 1.0f || sampleUv.y < 0.0f || sampleUv.y > 1.0f)
    {
        return;
    }

    float sampleDepth = GetViewDepthLod0(sampleUv);
    if (sampleDepth >= g_fFar)
    {
        return;
    }

    float expectedDepth = sampleViewPosition.z;
    float targetNormalDepthBiasFactor = saturate(abs(currentNormal.z)) * g_targetNormalBiasScale;
    float depthDelta = currentDepth - expectedDepth;
    float sampleDepthBias = depthDelta * targetNormalDepthBiasFactor * g_targetDepthBiasScale;
    float adjustedSampleDepth = max(g_fNear, sampleDepth + sampleDepthBias);
    float thickness = tex2Dlod(sampThickness, float4(sampleUv, 0.0f, 0.0f)).r;
    thickness = max(thickness, g_minThickness);
    float frontDepthWithMargin = adjustedSampleDepth - g_depthCompareThreshold;
    float backDepthWithMargin = adjustedSampleDepth + g_depthCompareThreshold;
    if (g_useThickness)
    {
        backDepthWithMargin += thickness * g_thicknessScale;
    }
    else
    {
        backDepthWithMargin += max(g_sampleDepthBiasDistance, currentDepth * 0.02f) * g_thicknessScale;
    }

    if (frontDepthWithMargin <= currentDepth && currentDepth <= backDepthWithMargin)
    {
        occluded = 1.0f;
        hitColor = tex2Dlod(sampColor, float4(sampleUv, 0.0f, 0.0f)).rgb;
    }

    valid = 1.0f;
}

float4 ComputeGiResult(float2 uv, float sampleCount)
{
    float currentDepth = GetViewDepth(uv);
    if (currentDepth >= g_fFar)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float3 currentNormal = GetViewNormal(uv);
    float3 currentViewPosition = GetViewPosition(uv);
    float occlusionCount = 0.0f;
    float validSampleCount = 0.0f;
    float3 occlusionColorSum = float3(0.0f, 0.0f, 0.0f);

    for (int sampleIndex = 0; sampleIndex < 64; ++sampleIndex)
    {
        if ((float)sampleIndex >= sampleCount)
        {
            break;
        }

        float occluded = 0.0f;
        float valid = 0.0f;
        float3 hitColor = float3(0.0f, 0.0f, 0.0f);
        ComputeGiSample(uv,
                        currentDepth,
                        currentNormal,
                        currentViewPosition,
                        (float)sampleIndex,
                        sampleCount,
                        occluded,
                        valid,
                        hitColor);
        occlusionCount += occluded;
        validSampleCount += valid;
        occlusionColorSum += hitColor * occluded;
    }

    float indirectFactor = 0.0f;
    if (validSampleCount > 0.0f)
    {
        indirectFactor = saturate(occlusionCount / validSampleCount);
    }

    float3 indirectColor = float3(0.0f, 0.0f, 0.0f);
    if (occlusionCount > 0.0f)
    {
        indirectColor = occlusionColorSum / occlusionCount;
    }

    return float4(indirectColor, indirectFactor);
}

float4 PS_GI4(VS_OUT inputData) : COLOR0
{
    return ComputeGiResult(inputData.uv, 4.0f);
}

float4 PS_GI8(VS_OUT inputData) : COLOR0
{
    return ComputeGiResult(inputData.uv, 8.0f);
}

float4 PS_GI16(VS_OUT inputData) : COLOR0
{
    return ComputeGiResult(inputData.uv, 16.0f);
}

float4 PS_GI32(VS_OUT inputData) : COLOR0
{
    return ComputeGiResult(inputData.uv, 32.0f);
}

float4 PS_GI64(VS_OUT inputData) : COLOR0
{
    return ComputeGiResult(inputData.uv, 64.0f);
}

float4 BlurGI(VS_OUT inputData, int radius) : COLOR0
{
    float centerDepth = GetViewDepth(inputData.uv);
    float3 centerNormal = GetViewNormal(inputData.uv);
    float4 centerGi = tex2D(sampGI, inputData.uv);
    float4 blurredValue = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;

    [loop]
    for (int y = -10; y <= 10; ++y)
    {
        if (abs(y) > radius)
        {
            continue;
        }

        [loop]
        for (int x = -10; x <= 10; ++x)
        {
            if (abs(x) > radius)
            {
                continue;
            }

            float2 sampleUv = inputData.uv + float2((float)x * g_invSize.x,
                                                    (float)y * g_invSize.y);
            sampleUv = saturate(sampleUv);
            float sampleDepth = GetViewDepth(sampleUv);
            float3 sampleNormal = GetViewNormal(sampleUv);
            float weight = ComputeBlurSampleWeight(1.0f,
                                                   centerDepth,
                                                   sampleDepth,
                                                   centerNormal,
                                                   sampleNormal);
            blurredValue += tex2D(sampGI, sampleUv) * weight;
            weightSum += weight;
        }
    }

    if (weightSum > 0.0f)
    {
        return blurredValue / weightSum;
    }

    return centerGi;
}

float4 PS_Blur5x5(VS_OUT inputData) : COLOR0
{
    return BlurGI(inputData, 2);
}

float4 PS_Blur11x11(VS_OUT inputData) : COLOR0
{
    return BlurGI(inputData, 5);
}

float4 PS_Blur21x21(VS_OUT inputData) : COLOR0
{
    return BlurGI(inputData, 10);
}

float4 PS_Composite(VS_OUT inputData) : COLOR0
{
    float4 sourceColor = tex2D(sampColor, inputData.uv);
    float4 giData = tex2D(sampGI, inputData.uv);
    float indirectAmount = saturate(g_indirectLightStrength * giData.a);
    if (indirectAmount > g_indirectLightMaxContribution)
    {
        indirectAmount = g_indirectLightMaxContribution;
    }

    float3 resultColor = sourceColor.rgb + (giData.rgb - sourceColor.rgb) * indirectAmount;
    return float4(saturate(resultColor), sourceColor.a);
}

technique TechniqueGI_Create4
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_GI4();
    }
}

technique TechniqueGI_Create8
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_GI8();
    }
}

technique TechniqueGI_Create16
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_GI16();
    }
}

technique TechniqueGI_Create32
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_GI32();
    }
}

technique TechniqueGI_Create64
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_GI64();
    }
}

technique TechniqueGI_Blur5x5
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur5x5();
    }
}

technique TechniqueGI_Blur11x11
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur11x11();
    }
}

technique TechniqueGI_Blur21x21
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur21x21();
    }
}

technique TechniqueGI_Composite
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Composite();
    }
}
