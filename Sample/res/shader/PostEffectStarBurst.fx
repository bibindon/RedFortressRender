texture g_SceneTex;
sampler SceneSampler = sampler_state
{
    Texture = <g_SceneTex>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_SrcTex;
sampler SrcSampler = sampler_state
{
    Texture = <g_SrcTex>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex0;
sampler BlurSampler0 = sampler_state
{
    Texture = <g_BlurTex0>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex1;
sampler BlurSampler1 = sampler_state
{
    Texture = <g_BlurTex1>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex2;
sampler BlurSampler2 = sampler_state
{
    Texture = <g_BlurTex2>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex3;
sampler BlurSampler3 = sampler_state
{
    Texture = <g_BlurTex3>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex4;
sampler BlurSampler4 = sampler_state
{
    Texture = <g_BlurTex4>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex5;
sampler BlurSampler5 = sampler_state
{
    Texture = <g_BlurTex5>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex6;
sampler BlurSampler6 = sampler_state
{
    Texture = <g_BlurTex6>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

float g_Threshold = 2.8f;
float2 g_TexelSize;
float4 g_BurstWeightsA = float4(0.32f, 0.23f, 0.16f, 0.11f);
float4 g_BurstWeightsB = float4(0.08f, 0.06f, 0.04f, 0.0f);

float4 BrightPassPS(float2 uv : TEXCOORD0) : COLOR
{
    const float4 c = tex2D(SrcSampler, uv);
    const float lum = dot(c.rgb, float3(0.299f, 0.587f, 0.114f));
    if (lum > g_Threshold)
    {
        return c;
    }
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

float4 DownsamplePS(float2 uv : TEXCOORD0) : COLOR
{
    return tex2D(SrcSampler, uv);
}

float4 DiagonalBlur3x3PS(float2 uv : TEXCOORD0) : COLOR
{
    const float2 texel = g_TexelSize;
    const float2 diagonalStep = texel * float2(1.0f, 1.0f);

    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;
    [unroll]
    for (int i = -15; i <= 15; ++i)
    {
        const float distance = abs((float)i);
        const float normalizedDistance = distance / 15.0f;
        const float weight = (1.0f - normalizedDistance) * (1.0f - normalizedDistance);
        sum += tex2D(SrcSampler, uv + diagonalStep * i) * weight;
        weightSum += weight;
    }

    return sum / weightSum;
}

float4 CombinePS(float2 uv : TEXCOORD0) : COLOR
{
    const float4 scene = tex2D(SceneSampler, uv);
    const float4 burst =
        tex2D(BlurSampler0, uv) * g_BurstWeightsA.x +
        tex2D(BlurSampler1, uv) * g_BurstWeightsA.y +
        tex2D(BlurSampler2, uv) * g_BurstWeightsA.z +
        tex2D(BlurSampler3, uv) * g_BurstWeightsA.w +
        tex2D(BlurSampler4, uv) * g_BurstWeightsB.x +
        tex2D(BlurSampler5, uv) * g_BurstWeightsB.y +
        tex2D(BlurSampler6, uv) * g_BurstWeightsB.z;

    float4 outColor = scene + burst;
    outColor.a = 1.0f;
    return outColor;
}

technique BrightPass
{
    pass P0
    {
        PixelShader = compile ps_3_0 BrightPassPS();
    }
}

technique Downsample
{
    pass P0
    {
        PixelShader = compile ps_3_0 DownsamplePS();
    }
}

technique DiagonalBlur3x3
{
    pass P0
    {
        PixelShader = compile ps_3_0 DiagonalBlur3x3PS();
    }
}

technique Combine
{
    pass P0
    {
        PixelShader = compile ps_3_0 CombinePS();
    }
}
