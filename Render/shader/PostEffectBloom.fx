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

float g_Threshold = 2.5f;
float2 g_TexelSize;
float4 g_BloomWeightsA = float4(0.28f, 0.24f, 0.18f, 0.14f);
float4 g_BloomWeightsB = float4(0.10f, 0.06f, 0.0f, 0.0f);

float4 BrightPassPS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float4 c = tex2D(SrcSampler, texCoord);
    const float lum = dot(c.rgb, float3(0.299f, 0.587f, 0.114f));
    if (lum > g_Threshold)
    {
        return c;
    }
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

float4 DownsamplePS(float2 texCoord : TEXCOORD0) : COLOR
{
    return tex2D(SrcSampler, texCoord);
}

float4 Blur3x3PS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float2 texel = g_TexelSize;

    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f, -1.0f)) * (1.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f, -1.0f)) * (2.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f, -1.0f)) * (1.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  0.0f)) * (2.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord)                                   * (4.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  0.0f)) * (2.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  1.0f)) * (1.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f,  1.0f)) * (2.0f / 16.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  1.0f)) * (1.0f / 16.0f);
    return sum;
}

float4 CombinePS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float4 scene = tex2D(SceneSampler, texCoord);
    const float4 bloom =
        tex2D(BlurSampler0, texCoord) * g_BloomWeightsA.x +
        tex2D(BlurSampler1, texCoord) * g_BloomWeightsA.y +
        tex2D(BlurSampler2, texCoord) * g_BloomWeightsA.z +
        tex2D(BlurSampler3, texCoord) * g_BloomWeightsA.w +
        tex2D(BlurSampler4, texCoord) * g_BloomWeightsB.x +
        tex2D(BlurSampler5, texCoord) * g_BloomWeightsB.y;

    float4 outColor = scene + bloom;
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

technique Blur3x3
{
    pass P0
    {
        PixelShader = compile ps_3_0 Blur3x3PS();
    }
}

technique Combine
{
    pass P0
    {
        PixelShader = compile ps_3_0 CombinePS();
    }
}
