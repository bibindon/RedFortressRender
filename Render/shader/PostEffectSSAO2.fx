float4x4 g_matView;
float4x4 g_matProj;

float g_fNear = 0.1f;
float g_fFar = 15.0f;
float2 g_invSize;
float g_posRange = 15.0f;
float g_sampleRadius = 1.0f;
float g_aoBrightness = 1.0f;
float g_aoSaturationBoost = 0.30f;
float g_depthCompareThreshold = 0.00f;
float g_depthBiasScale = 1.0f;
float g_normalBiasScale = 1.0f;

texture texZ;
texture texPos;
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

sampler sampPos = sampler_state
{
    Texture = (texPos);
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

VS_OUT VS_Fullscreen(float4 p : POSITION, float2 uv : TEXCOORD0)
{
    VS_OUT o;
    o.pos = p;
    o.uv = uv + 0.5f * g_invSize;
    return o;
}

float3 DecodeWorldPos(float3 enc)
{
    return (enc * 2.0f - 1.0f) * g_posRange;
}

float3 DecodeWorldNormal(float3 enc)
{
    return normalize(enc * 2.0f - 1.0f);
}

float3 GetViewPosition(float2 uv)
{
    float3 worldPos = DecodeWorldPos(tex2D(sampPos, uv).rgb);
    return mul(float4(worldPos, 1.0f), g_matView).xyz;
}

float3 GetViewNormal(float2 uv)
{
    float3 worldNormal = DecodeWorldNormal(tex2D(sampNormal, uv).rgb);
    return normalize(mul(float4(worldNormal, 0.0f), g_matView).xyz);
}

float GetViewDepth(float2 uv)
{
    float linearZ = tex2D(sampZ, uv).a;
    return linearZ * (g_fFar - g_fNear) + g_fNear;
}

float Random01(float2 seed)
{
    return frac(sin(dot(seed, float2(12.9898f, 78.233f))) * 43758.5453f);
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

float ComputeOcclusionSample(float2 baseUv,
                             float currentDepth,
                             float3 currentNormal,
                             float3 currentViewPosition,
                             float3 tangent,
                             float3 bitangent,
                             float sampleDistance,
                             float index)
{
    float randomAngle = Random01(baseUv * 32768.0f + index.xx) * 6.2831853f;
    float radial = (index + 0.5f) / 16.0f;
    float radiusScale = radial * radial;
    float2 disk = float2(cos(randomAngle), sin(randomAngle)) * radiusScale;
    float hemisphereLift = sqrt(saturate(1.0f - dot(disk, disk)));
    float3 sampleDirection = normalize(tangent * disk.x + bitangent * disk.y + currentNormal * hemisphereLift);

    float3 sampleViewPosition = currentViewPosition + currentNormal * (sampleDistance * 0.05f) + sampleDirection * sampleDistance;
    if (sampleViewPosition.z <= 0.0f)
    {
        return 0.0f;
    }

    float2 sampleUv = ProjectViewPositionToTexCoord(sampleViewPosition);
    if (sampleUv.x < 0.0f || sampleUv.x > 1.0f || sampleUv.y < 0.0f || sampleUv.y > 1.0f)
    {
        return 0.0f;
    }

    float sampleDepth = GetViewDepth(sampleUv);
    if (sampleDepth >= g_fFar)
    {
        return 0.0f;
    }

    float expectedDepth = sampleViewPosition.z;
    float normalDepthBiasFactor = saturate(abs(currentNormal.z)) * g_normalBiasScale;
    float sampleDepthBias = (currentDepth - expectedDepth) * normalDepthBiasFactor * g_depthBiasScale;
    float adjustedSampleDepth = max(0.0f, sampleDepth + sampleDepthBias);
    float sampleThickness = tex2D(sampThickness, sampleUv).r;

    float frontDepthWithMargin = adjustedSampleDepth - g_depthCompareThreshold;
    float backDepthWithMargin = adjustedSampleDepth + sampleThickness + g_depthCompareThreshold;
    if (frontDepthWithMargin <= currentDepth && currentDepth <= backDepthWithMargin)
    {
        float distanceAttenuation = 1.0f - saturate(abs(sampleDepth - currentDepth) / max(sampleDistance, 0.001f));
        return distanceAttenuation * distanceAttenuation;
    }

    return 0.0f;
}

float4 PS_AO2(VS_OUT i) : COLOR0
{
    float currentDepth = GetViewDepth(i.uv);
    if (currentDepth >= g_fFar)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float3 currentViewPosition = GetViewPosition(i.uv);
    float3 currentNormal = GetViewNormal(i.uv);

    float3 up = (abs(currentNormal.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    float3 tangent = normalize(cross(up, currentNormal));
    float3 bitangent = cross(currentNormal, tangent);

    const int kSampleCount = 16;
    float occlusion = 0.0f;

    [unroll]
    for (int sampleIndex = 0; sampleIndex < kSampleCount; ++sampleIndex)
    {
        occlusion += ComputeOcclusionSample(i.uv,
                                            currentDepth,
                                            currentNormal,
                                            currentViewPosition,
                                            tangent,
                                            bitangent,
                                            g_sampleRadius,
                                            (float)sampleIndex);
    }

    float occlusionRate = occlusion / (float)kSampleCount;
    float ao = saturate(1.0f - occlusionRate);
    ao = max(0.2f, ao);
    return float4(ao, ao, ao, 1.0f);
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
            if (x == 0 && y == 0)
            {
                continue;
            }

            float2 sampleUv = i.uv + float2((float)x * texelSize.x,
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
    }

    float ao = 1.0f;
    if (weightSum > 0.0f)
    {
        ao = blurredValue / weightSum;
    }
    return float4(ao, ao, ao, 1.0f);
}

float4 PS_Composite(VS_OUT i) : COLOR0
{
    float3 color = tex2D(sampColor, i.uv).rgb;
    float ao = tex2D(sampAO, i.uv).r;
    float aoAdjusted = pow(saturate(ao), 1.0f / max(g_aoBrightness, 0.0001f));
    float shadowPresence = saturate(1.0f - ao);
    float3 shadedColor = color * aoAdjusted;
    float saturationAmount = lerp(1.0f, 1.0f + g_aoSaturationBoost, shadowPresence);
    return float4(IncreaseSaturation(shadedColor, saturationAmount), 1.0f);
}

technique TechniqueAO2_Create
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_AO2();
    }
}

technique TechniqueAO2_Blur21x21
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Blur21x21();
    }
}

technique TechniqueAO2_Composite
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS_Fullscreen();
        PixelShader = compile ps_3_0 PS_Composite();
    }
}
