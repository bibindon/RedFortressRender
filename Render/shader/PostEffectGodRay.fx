// God ray post effect.
//
// Pass1: build a screen-space occlusion mask from the GBuffer depth.
// Pass2: blur the occlusion mask horizontally and vertically.
// Pass3: ray-march the blurred mask and composite the result with the scene.

texture g_SceneTex;
texture g_OcclusionTex;
texture g_BlurSourceTex;

float2 g_LightScreenPos = float2(0.5f, 0.3f);
float3 g_LightColor = float3(1.0f, 0.9f, 0.8f);
float  g_RayLength = 1.0f;
float  g_RayIntensity = 0.6f;
float  g_OcclusionFalloff = 5.0f;

static const int SAMPLE_COUNT = 128;

texture g_ZTex;
float   g_LightViewZ;
float2  g_TexelSize = float2(1.0f / 1280.0f, 1.0f / 720.0f);
float2  g_BlurDirection = float2(1.0f, 0.0f);

sampler g_SceneSampler = sampler_state
{
    Texture = (g_SceneTex);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler g_OcclusionSampler = sampler_state
{
    Texture = (g_OcclusionTex);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler g_BlurSourceSampler = sampler_state
{
    Texture = (g_BlurSourceTex);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler g_ZSampler = sampler_state
{
    Texture = (g_ZTex);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VS_IN
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VS_OUT
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VS_OUT VS(VS_IN i)
{
    VS_OUT o;
    o.pos = i.pos;
    o.uv = i.uv;
    return o;
}

float4 PS_OcclusionMask(VS_OUT i) : COLOR
{
    float pixelZ = tex2D(g_ZSampler, i.uv).a;
    float mask = (pixelZ > 0.0f && pixelZ < g_LightViewZ) ? 0.0f : 1.0f;
    return float4(mask, mask, mask, 1.0f);
}

float4 PS_Blur(VS_OUT i) : COLOR
{
    const float weights[5] =
    {
        0.2270270270f,
        0.1945945946f,
        0.1216216216f,
        0.0540540541f,
        0.0162162162f
    };

    float3 color = tex2D(g_BlurSourceSampler, i.uv).rgb * weights[0];

    [unroll]
    for (int tap = 1; tap < 5; ++tap)
    {
        float2 offset = g_BlurDirection * g_TexelSize * float(tap);
        color += tex2D(g_BlurSourceSampler, i.uv + offset).rgb * weights[tap];
        color += tex2D(g_BlurSourceSampler, i.uv - offset).rgb * weights[tap];
    }

    return float4(color, 1.0f);
}

float4 PS_GodRay(VS_OUT i) : COLOR
{
    float2 dir = g_LightScreenPos - i.uv;

    float visibilitySum = 0.0f;
    float validSampleCount = 0.0f;

    [loop]
    for (int s = 0; s < SAMPLE_COUNT; ++s)
    {
        float t = g_RayLength * (float(s) / float(SAMPLE_COUNT - 1));
        float2 sampleUv = i.uv + dir * t;

        if (sampleUv.x >= 0.0f && sampleUv.x <= 1.0f &&
            sampleUv.y >= 0.0f && sampleUv.y <= 1.0f)
        {
            visibilitySum += tex2D(g_OcclusionSampler, sampleUv).r;
            validSampleCount += 1.0f;
        }
    }

    float lightRays = 0.0f;
    if (validSampleCount > 0.0f)
    {
        lightRays = visibilitySum / validSampleCount;
    }

    float occlusion = 1.0f - lightRays;
    lightRays = exp(-g_OcclusionFalloff * occlusion);

    float3 sceneColor = tex2D(g_SceneSampler, i.uv).rgb;
    float3 rayColor = lightRays * g_RayIntensity * g_LightColor;
    return float4(sceneColor + rayColor, 1.0f);
}

technique OcclusionMask
{
    pass P0
    {
        CullMode = NONE;
        ZEnable = FALSE;
        ZWriteEnable = FALSE;
        AlphaBlendEnable = FALSE;
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS_OcclusionMask();
    }
}

technique Blur
{
    pass P0
    {
        CullMode = NONE;
        ZEnable = FALSE;
        ZWriteEnable = FALSE;
        AlphaBlendEnable = FALSE;
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS_Blur();
    }
}

technique GodRay
{
    pass P0
    {
        CullMode = NONE;
        ZEnable = FALSE;
        ZWriteEnable = FALSE;
        AlphaBlendEnable = FALSE;
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS_GodRay();
    }
}
