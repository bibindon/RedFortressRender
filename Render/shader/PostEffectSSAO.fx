// SSAO shader.
// 法線から直交基底を作る入口を if で明示している。

float4x4 g_matView;
float4x4 g_matProj;

float g_fNear = 0.1f;
float g_fFar = 50.0f;
float2 g_invSize;
float2 g_aoInvSize;
float g_sampleRadius = 1.0f;
int g_sampleCount = 16;
bool g_enableDepthScaledSampleDistance = false;
float g_shadowStrength = 1.0f;
float g_aoSaturationBoost = 0.30f;
float g_depthCompareThreshold = 0.00f;
float g_depthBiasScale = 1.0f;
float g_normalBiasScale = 1.0f;

texture texZ;
texture texNormal;
texture texThickness;
texture texAO;
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

sampler sampAO = sampler_state
{
    Texture = (texAO);
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

float GetViewDepth(float2 uv);

VS_OUT VS_Fullscreen(float4 p : POSITION, float2 uv : TEXCOORD0)
{
    VS_OUT o;
    o.pos = p;
    o.uv = uv + 0.5f * g_invSize;
    return o;
}

float3 DecodeWorldNormal(float3 enc)
{
    return normalize(enc * 2.0f - 1.0f);
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

float3 GetViewNormal(float2 uv)
{
    float3 worldNormal = DecodeWorldNormal(tex2D(sampNormal, uv).rgb);
    return normalize(mul(float4(worldNormal, 0.0f), g_matView).xyz);
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

float2 ProjectViewPositionToTexCoord(float3 viewPosition)
{
    float4 clip = mul(float4(viewPosition, 1.0f), g_matProj);
    float2 ndc = clip.xy / max(clip.w, 0.0001f);
    return float2(ndc.x * 0.5f + 0.5f,
                  0.5f - ndc.y * 0.5f) + 0.5f * g_invSize;
}

float3 IncreaseSaturation(float3 color, float amount)
{
    float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
    return saturate(lerp(luminance.xxx, color, amount));
}

float2 ComputeOcclusionSample(float2 baseUv,
                              float currentDepth,
                              float3 currentNormal,
                              float3 currentViewPosition,
                              float3 tangent,
                              float3 bitangent,
                              float sampleDistance,
                              float index,
                              float sampleCount)
{
    float distanceScale = 1.0f;
    if (sampleCount > 1.0f)
    {
        float fixedDistanceU = saturate(index / (sampleCount - 1.0f));
        distanceScale = fixedDistanceU * fixedDistanceU * fixedDistanceU;
    }

    float normalizedDepth = saturate((currentDepth - g_fNear) / max(g_fFar - g_fNear, 0.0001f));

    if (g_enableDepthScaledSampleDistance)
    {
        float depthScale = lerp(0.25f, 1.0f, normalizedDepth);
        distanceScale *= depthScale;
    }
    
    distanceScale += 0.1f * normalizedDepth;

    float sampleDistanceScaled = sampleDistance * distanceScale;
    float3 sampleViewPosition = currentViewPosition + currentNormal * sampleDistanceScaled;
    if (sampleViewPosition.z <= 0.0f)
    {
        return float2(0.0f, 0.0f);
    }

    float2 sampleUv = ProjectViewPositionToTexCoord(sampleViewPosition);
    if (sampleUv.x < 0.0f || sampleUv.x > 1.0f || sampleUv.y < 0.0f || sampleUv.y > 1.0f)
    {
        return float2(0.0f, 0.0f);
    }

    float sampleDepth = GetViewDepthLod0(sampleUv);
    if (sampleDepth >= g_fFar)
    {
        return float2(0.0f, 0.0f);
    }

    float expectedDepth = sampleViewPosition.z;
    float normalDepthBiasFactor = saturate(abs(currentNormal.z)) * g_normalBiasScale;
    float sampleDepthBias = (currentDepth - expectedDepth) * normalDepthBiasFactor * g_depthBiasScale;
    float adjustedSampleDepth = max(0.0f, sampleDepth + sampleDepthBias);
    float sampleThickness = tex2Dlod(sampThickness, float4(sampleUv, 0.0f, 0.0f)).r;

    float frontDepthWithMargin = adjustedSampleDepth - g_depthCompareThreshold;
    float backDepthWithMargin = adjustedSampleDepth + sampleThickness + g_depthCompareThreshold;
    if (frontDepthWithMargin <= currentDepth && currentDepth <= backDepthWithMargin)
    {
        return float2(1.0f, 1.0f);
    }

    return float2(0.0f, 1.0f);
}

void AccumulateOcclusionSample(float2 baseUv,
                               float currentDepth,
                               float3 currentNormal,
                               float3 currentViewPosition,
                               float3 tangent,
                               float3 bitangent,
                               float sampleRadius,
                               float sampleIndex,
                               float sampleCount,
                               inout float occlusionCount,
                               inout float validSampleCount)
{
    float2 occlusionSample = ComputeOcclusionSample(baseUv,
                                                    currentDepth,
                                                    currentNormal,
                                                    currentViewPosition,
                                                    tangent,
                                                    bitangent,
                                                    sampleRadius,
                                                    sampleIndex,
                                                    sampleCount);
    occlusionCount += occlusionSample.x;
    validSampleCount += occlusionSample.y;
}

float4 FinalizeAOResult(float occlusionCount, float validSampleCount)
{
    float ao = 1.0f;
    if (validSampleCount > 0.0f)
    {
        float occlusionRate = occlusionCount / validSampleCount;
        ao = saturate(1.0f - occlusionRate);
    }

    return float4(ao, ao, ao, 1.0f);
}

void InitializeAOSampling(VS_OUT i,
                          out float currentDepth,
                          out float3 currentNormal,
                          out float3 currentViewPosition,
                          out float3 tangent,
                          out float3 bitangent)
{
    currentDepth = GetViewDepth(i.uv);
    currentViewPosition = GetViewPosition(i.uv);
    currentNormal = GetViewNormal(i.uv);

    float3 up = float3(0.0f, 1.0f, 0.0f);
    if (abs(currentNormal.z) < 0.999f)
    {
        up = float3(0.0f, 0.0f, 1.0f);
    }
    tangent = normalize(cross(up, currentNormal));
    bitangent = cross(currentNormal, tangent);
}

float4 PS_AO4(VS_OUT i) : COLOR0
{
    float currentDepth = 0.0f;
    float3 currentNormal = float3(0.0f, 0.0f, 0.0f);
    float3 currentViewPosition = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(0.0f, 0.0f, 0.0f);
    float3 bitangent = float3(0.0f, 0.0f, 0.0f);
    InitializeAOSampling(i, currentDepth, currentNormal, currentViewPosition, tangent, bitangent);
    if (currentDepth >= g_fFar)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float occlusionCount = 0.0f;
    float validSampleCount = 0.0f;
    const float sampleCount = 4.0f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < 4; ++sampleIndex)
    {
        AccumulateOcclusionSample(i.uv,
                                  currentDepth,
                                  currentNormal,
                                  currentViewPosition,
                                  tangent,
                                  bitangent,
                                  g_sampleRadius,
                                  (float)sampleIndex,
                                  sampleCount,
                                  occlusionCount,
                                  validSampleCount);
    }

    return FinalizeAOResult(occlusionCount, validSampleCount);
}

float4 PS_AO8(VS_OUT i) : COLOR0
{
    float currentDepth = 0.0f;
    float3 currentNormal = float3(0.0f, 0.0f, 0.0f);
    float3 currentViewPosition = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(0.0f, 0.0f, 0.0f);
    float3 bitangent = float3(0.0f, 0.0f, 0.0f);
    InitializeAOSampling(i, currentDepth, currentNormal, currentViewPosition, tangent, bitangent);
    if (currentDepth >= g_fFar)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float occlusionCount = 0.0f;
    float validSampleCount = 0.0f;
    const float sampleCount = 8.0f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < 8; ++sampleIndex)
    {
        AccumulateOcclusionSample(i.uv,
                                  currentDepth,
                                  currentNormal,
                                  currentViewPosition,
                                  tangent,
                                  bitangent,
                                  g_sampleRadius,
                                  (float)sampleIndex,
                                  sampleCount,
                                  occlusionCount,
                                  validSampleCount);
    }

    return FinalizeAOResult(occlusionCount, validSampleCount);
}

float4 PS_AO16(VS_OUT i) : COLOR0
{
    float currentDepth = 0.0f;
    float3 currentNormal = float3(0.0f, 0.0f, 0.0f);
    float3 currentViewPosition = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(0.0f, 0.0f, 0.0f);
    float3 bitangent = float3(0.0f, 0.0f, 0.0f);
    InitializeAOSampling(i, currentDepth, currentNormal, currentViewPosition, tangent, bitangent);
    if (currentDepth >= g_fFar)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float occlusionCount = 0.0f;
    float validSampleCount = 0.0f;
    const float sampleCount = 16.0f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < 16; ++sampleIndex)
    {
        AccumulateOcclusionSample(i.uv,
                                  currentDepth,
                                  currentNormal,
                                  currentViewPosition,
                                  tangent,
                                  bitangent,
                                  g_sampleRadius,
                                  (float)sampleIndex,
                                  sampleCount,
                                  occlusionCount,
                                  validSampleCount);
    }

    return FinalizeAOResult(occlusionCount, validSampleCount);
}

float4 PS_AO32(VS_OUT i) : COLOR0
{
    float currentDepth = 0.0f;
    float3 currentNormal = float3(0.0f, 0.0f, 0.0f);
    float3 currentViewPosition = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(0.0f, 0.0f, 0.0f);
    float3 bitangent = float3(0.0f, 0.0f, 0.0f);
    InitializeAOSampling(i, currentDepth, currentNormal, currentViewPosition, tangent, bitangent);
    if (currentDepth >= g_fFar)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float occlusionCount = 0.0f;
    float validSampleCount = 0.0f;
    const float sampleCount = 32.0f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < 32; ++sampleIndex)
    {
        AccumulateOcclusionSample(i.uv,
                                  currentDepth,
                                  currentNormal,
                                  currentViewPosition,
                                  tangent,
                                  bitangent,
                                  g_sampleRadius,
                                  (float)sampleIndex,
                                  sampleCount,
                                  occlusionCount,
                                  validSampleCount);
    }

    return FinalizeAOResult(occlusionCount, validSampleCount);
}

float4 PS_AO64(VS_OUT i) : COLOR0
{
    float currentDepth = 0.0f;
    float3 currentNormal = float3(0.0f, 0.0f, 0.0f);
    float3 currentViewPosition = float3(0.0f, 0.0f, 0.0f);
    float3 tangent = float3(0.0f, 0.0f, 0.0f);
    float3 bitangent = float3(0.0f, 0.0f, 0.0f);
    InitializeAOSampling(i, currentDepth, currentNormal, currentViewPosition, tangent, bitangent);
    if (currentDepth >= g_fFar)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float occlusionCount = 0.0f;
    float validSampleCount = 0.0f;
    const float sampleCount = 64.0f;

    [loop]
    for (int sampleIndex = 0; sampleIndex < 64; ++sampleIndex)
    {
        AccumulateOcclusionSample(i.uv,
                                  currentDepth,
                                  currentNormal,
                                  currentViewPosition,
                                  tangent,
                                  bitangent,
                                  g_sampleRadius,
                                  (float)sampleIndex,
                                  sampleCount,
                                  occlusionCount,
                                  validSampleCount);
    }

    return FinalizeAOResult(occlusionCount, validSampleCount);
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

void AccumulateBlurSample(float2 baseUv,
                          float2 texelSize,
                          float centerDepth,
                          float3 centerNormal,
                          int x,
                          int y,
                          inout float blurredValue,
                          inout float weightSum)
{
    float2 sampleUv = baseUv + float2((float)x * texelSize.x,
                                      (float)y * texelSize.y);
    sampleUv = saturate(sampleUv);

    float sampleDepth = GetViewDepth(sampleUv);
    float3 sampleNormal = GetViewNormal(sampleUv);
    float weight = ComputeBlurSampleWeight(1.0f,
                                           centerDepth,
                                           sampleDepth,
                                           centerNormal,
                                           sampleNormal);
    blurredValue += tex2D(sampAO, sampleUv).r * weight;
    weightSum += weight;
}

float4 FinalizeBlurResult(float blurredValue, float weightSum)
{
    float ao = 1.0f;
    if (weightSum > 0.0f)
    {
        ao = blurredValue / weightSum;
    }

    return float4(ao, ao, ao, 1.0f);
}

float4 PS_Blur3x3(VS_OUT i) : COLOR0
{
    float2 texelSize = g_invSize;
    float centerDepth = GetViewDepth(i.uv);
    float3 centerNormal = GetViewNormal(i.uv);
    float blurredValue = 0.0f;
    float weightSum = 0.0f;

    [loop]
    for (int y = -1; y <= 1; ++y)
    {
        [loop]
        for (int x = -1; x <= 1; ++x)
        {
            if (x != 0 || y != 0)
            {
                AccumulateBlurSample(i.uv,
                                     texelSize,
                                     centerDepth,
                                     centerNormal,
                                     x,
                                     y,
                                     blurredValue,
                                     weightSum);
            }
        }
    }

    return FinalizeBlurResult(blurredValue, weightSum);
}

float4 PS_Blur5x5(VS_OUT i) : COLOR0
{
    float2 texelSize = g_invSize;
    float centerDepth = GetViewDepth(i.uv);
    float3 centerNormal = GetViewNormal(i.uv);
    float blurredValue = 0.0f;
    float weightSum = 0.0f;

    [loop]
    for (int y = -2; y <= 2; ++y)
    {
        [loop]
        for (int x = -2; x <= 2; ++x)
        {
            if (x != 0 || y != 0)
            {
                AccumulateBlurSample(i.uv,
                                     texelSize,
                                     centerDepth,
                                     centerNormal,
                                     x,
                                     y,
                                     blurredValue,
                                     weightSum);
            }
        }
    }

    return FinalizeBlurResult(blurredValue, weightSum);
}

float4 PS_Blur11x11(VS_OUT i) : COLOR0
{
    float2 texelSize = g_invSize;
    float centerDepth = GetViewDepth(i.uv);
    float3 centerNormal = GetViewNormal(i.uv);
    float blurredValue = 0.0f;
    float weightSum = 0.0f;

    [loop]
    for (int y = -5; y <= 5; ++y)
    {
        [loop]
        for (int x = -5; x <= 5; ++x)
        {
            if (x != 0 || y != 0)
            {
                AccumulateBlurSample(i.uv,
                                     texelSize,
                                     centerDepth,
                                     centerNormal,
                                     x,
                                     y,
                                     blurredValue,
                                     weightSum);
            }
        }
    }

    return FinalizeBlurResult(blurredValue, weightSum);
}

float4 PS_Blur21x21(VS_OUT i) : COLOR0
{
    float2 texelSize = g_invSize;
    float centerDepth = GetViewDepth(i.uv);
    float3 centerNormal = GetViewNormal(i.uv);
    float blurredValue = 0.0f;
    float weightSum = 0.0f;

    [loop]
    for (int y = -10; y <= 10; ++y)
    {
        [loop]
        for (int x = -10; x <= 10; ++x)
        {
            if (x != 0 || y != 0)
            {
                AccumulateBlurSample(i.uv,
                                     texelSize,
                                     centerDepth,
                                     centerNormal,
                                     x,
                                     y,
                                     blurredValue,
                                     weightSum);
            }
        }
    }

    return FinalizeBlurResult(blurredValue, weightSum);
}

float4 PS_Composite(VS_OUT i) : COLOR0
{
    float3 color = tex2D(sampColor, i.uv).rgb;
    float ao = tex2D(sampAO, i.uv).r;
    float aoAdjusted = saturate(1.0f - (1.0f - saturate(ao)) * g_shadowStrength);
    float shadowPresence = saturate(1.0f - ao);
    float3 shadedColor = color * aoAdjusted;
    float saturationAmount = lerp(1.0f, 1.0f + g_aoSaturationBoost, shadowPresence);
    return float4(IncreaseSaturation(shadedColor, saturationAmount), 1.0f);
}

float4 PS_Composite4TapAverage(VS_OUT i) : COLOR0
{
    float3 color = tex2D(sampColor, i.uv).rgb;
    float aoCenter = tex2D(sampAO, i.uv).r;
    float aoRight = tex2D(sampAO, i.uv + float2(g_aoInvSize.x, 0.0f)).r;
    float aoDown = tex2D(sampAO, i.uv + float2(0.0f, g_aoInvSize.y)).r;
    float aoDownRight = tex2D(sampAO, i.uv + g_aoInvSize).r;
    float ao = (aoCenter + aoRight + aoDown + aoDownRight) * 0.25f;
    float aoAdjusted = saturate(1.0f - (1.0f - saturate(ao)) * g_shadowStrength);
    float shadowPresence = saturate(1.0f - ao);
    float3 shadedColor = color * aoAdjusted;
    float saturationAmount = lerp(1.0f, 1.0f + g_aoSaturationBoost, shadowPresence);
    return float4(IncreaseSaturation(shadedColor, saturationAmount), 1.0f);
}

technique TechniqueAO_Create4
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_AO4();
    }
}

technique TechniqueAO_Create8
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_AO8();
    }
}

technique TechniqueAO_Create16
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_AO16();
    }
}

technique TechniqueAO_Create32
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_AO32();
    }
}

technique TechniqueAO_Create64
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_AO64();
    }
}

technique TechniqueAO_Blur3x3
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur3x3();
    }
}

technique TechniqueAO_Blur5x5
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur5x5();
    }
}

technique TechniqueAO_Blur11x11
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur11x11();
    }
}

technique TechniqueAO_Blur21x21
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur21x21();
    }
}

technique TechniqueAO_Composite
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Composite();
    }
}

technique TechniqueAO_Composite4TapAverage
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Composite4TapAverage();
    }
}
