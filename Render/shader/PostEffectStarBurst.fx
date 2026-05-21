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

float g_Threshold = 2.8f;
float2 g_TexelSize;
float4 g_BurstWeightsA = float4(0.34f, 0.24f, 0.17f, 0.12f);
float4 g_BurstWeightsB = float4(0.08f, 0.05f, 0.0f, 0.0f);

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

    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
    sum += tex2D(SrcSampler, uv + texel * float2(-1.0f, -1.0f)) * (2.0f / 8.0f);
    sum += tex2D(SrcSampler, uv + texel * float2( 1.0f, -1.0f)) * (1.0f / 8.0f);
    sum += tex2D(SrcSampler, uv)                                   * (2.0f / 8.0f);
    sum += tex2D(SrcSampler, uv + texel * float2(-1.0f,  1.0f)) * (1.0f / 8.0f);
    sum += tex2D(SrcSampler, uv + texel * float2( 1.0f,  1.0f)) * (2.0f / 8.0f);
    return sum;
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
        tex2D(BlurSampler5, uv) * g_BurstWeightsB.y;

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
