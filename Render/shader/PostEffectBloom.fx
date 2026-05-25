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

texture g_AddTex;
sampler AddSampler = sampler_state
{
    Texture = <g_AddTex>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

float g_Threshold = 2.5f;
float2 g_TexelSize;
float4 g_BloomWeightsA = float4(0.25f, 0.25f, 0.25f, 0.25f);

float4 BrightSample(float2 texCoord)
{
    const float4 c = tex2D(SrcSampler, texCoord);
    const float lum = dot(c.rgb, float3(0.299f, 0.587f, 0.114f));
    const float3 bloomColor = lerp(lum.xxx, c.rgb, 0.65f);
    return (lum > g_Threshold) ? float4(bloomColor, c.a) : float4(0.0f, 0.0f, 0.0f, 1.0f);
}

float4 BrightPassDownsamplePS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float2 texel = g_TexelSize;
    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
    sum += BrightSample(texCoord + texel * float2(-3.0f, -3.0f));
    sum += BrightSample(texCoord + texel * float2(-1.0f, -3.0f));
    sum += BrightSample(texCoord + texel * float2( 1.0f, -3.0f));
    sum += BrightSample(texCoord + texel * float2( 3.0f, -3.0f));
    sum += BrightSample(texCoord + texel * float2(-3.0f, -1.0f));
    sum += BrightSample(texCoord + texel * float2(-1.0f, -1.0f));
    sum += BrightSample(texCoord + texel * float2( 1.0f, -1.0f));
    sum += BrightSample(texCoord + texel * float2( 3.0f, -1.0f));
    sum += BrightSample(texCoord + texel * float2(-3.0f,  1.0f));
    sum += BrightSample(texCoord + texel * float2(-1.0f,  1.0f));
    sum += BrightSample(texCoord + texel * float2( 1.0f,  1.0f));
    sum += BrightSample(texCoord + texel * float2( 3.0f,  1.0f));
    sum += BrightSample(texCoord + texel * float2(-3.0f,  3.0f));
    sum += BrightSample(texCoord + texel * float2(-1.0f,  3.0f));
    sum += BrightSample(texCoord + texel * float2( 1.0f,  3.0f));
    sum += BrightSample(texCoord + texel * float2( 3.0f,  3.0f));
    return sum * (1.0f / 16.0f);
}

float4 DownsamplePS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float2 texel = g_TexelSize;
    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-0.5f, -0.5f));
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.5f, -0.5f));
    sum += tex2D(SrcSampler, texCoord + texel * float2(-0.5f,  0.5f));
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.5f,  0.5f));
    return sum * 0.25f;
}

float4 Blur5x5PS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float2 texel = g_TexelSize;
    float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f, -2.0f)) * (1.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f, -2.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f, -2.0f)) * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f, -2.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f, -2.0f)) * (1.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f, -1.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f, -1.0f)) * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f, -1.0f)) * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f, -1.0f)) * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f, -1.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f,  0.0f)) * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  0.0f)) * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord)                                  * (36.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  0.0f)) * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f,  0.0f)) * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f,  1.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  1.0f)) * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f,  1.0f)) * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  1.0f)) * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f,  1.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f,  2.0f)) * (1.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  2.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f,  2.0f)) * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  2.0f)) * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f,  2.0f)) * (1.0f / 256.0f);
    return sum;
}

float4 UpsampleAdd5x5PS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float4 blurredLow = Blur5x5PS(texCoord);
    const float4 high = tex2D(AddSampler, texCoord);
    return blurredLow + high;
}

float4 CombinePS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float4 scene = tex2D(SceneSampler, texCoord);
    float4 bloom =
        tex2D(BlurSampler0, texCoord) * g_BloomWeightsA.x +
        tex2D(BlurSampler1, texCoord) * g_BloomWeightsA.y +
        tex2D(BlurSampler2, texCoord) * g_BloomWeightsA.z +
        tex2D(BlurSampler3, texCoord) * g_BloomWeightsA.w;

    bloom *= 0.5f;
    bloom = pow(bloom, 0.5f);

    float4 outColor = scene + float4(min(bloom.rgb, float3(0.5f, 0.5f, 0.5f)), 0.0f);
    outColor.a = 1.0f;
    return outColor;
}

technique BrightPassDownsample
{
    pass P0
    {
        PixelShader = compile ps_3_0 BrightPassDownsamplePS();
    }
}

technique Downsample
{
    pass P0
    {
        PixelShader = compile ps_3_0 DownsamplePS();
    }
}

technique Blur5x5
{
    pass P0
    {
        PixelShader = compile ps_3_0 Blur5x5PS();
    }
}

technique UpsampleAdd5x5
{
    pass P0
    {
        PixelShader = compile ps_3_0 UpsampleAdd5x5PS();
    }
}

technique Combine
{
    pass P0
    {
        PixelShader = compile ps_3_0 CombinePS();
    }
}
