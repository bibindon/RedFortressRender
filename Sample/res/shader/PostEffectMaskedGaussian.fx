bool g_bFilterON = false;

float2 g_TexelSize;
texture g_SrcTex;
sampler SrcSampler = sampler_state
{
    Texture = <g_SrcTex>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_SrcTex2;
sampler SrcSampler2 = sampler_state
{
    Texture = <g_SrcTex2>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_MaskTex;
sampler MaskSampler = sampler_state
{
    Texture = <g_MaskTex>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

#define SAMPLE_SIZE_MAX 101
int g_sampleSize = 25;

float4 GaussianSparseH(float2 texCoord : TEXCOORD0) : COLOR
{
    float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (!g_bFilterON)
    {
        return tex2D(SrcSampler, texCoord);
    }

    float2 step = float2(g_TexelSize.x, 0.0);
    c = tex2D(SrcSampler, texCoord) / g_sampleSize;

    [unroll]
    for (int i = 1; i <= SAMPLE_SIZE_MAX / 2; i++)
    {
        if ((g_sampleSize / 2) < i)
        {
            break;
        }

        c += tex2D(SrcSampler, texCoord + step * i) / g_sampleSize;
        c += tex2D(SrcSampler, texCoord - step * i) / g_sampleSize;
    }

    return c;
}

float4 GaussianSparseV(float2 texCoord : TEXCOORD0) : COLOR
{
    float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (!g_bFilterON)
    {
        return tex2D(SrcSampler, texCoord);
    }

    float2 step = float2(0.0, g_TexelSize.y);
    c = tex2D(SrcSampler, texCoord) / g_sampleSize;

    [unroll]
    for (int i = 1; i <= SAMPLE_SIZE_MAX / 2; i++)
    {
        if ((g_sampleSize / 2) < i)
        {
            break;
        }

        c += tex2D(SrcSampler, texCoord + step * i) / g_sampleSize;
        c += tex2D(SrcSampler, texCoord - step * i) / g_sampleSize;
    }

    return c;
}

float4 CompositeMaskedBlur(float2 uv : TEXCOORD0) : COLOR
{
    const float4 blurColor = tex2D(SrcSampler, uv);
    const float4 originalColor = tex2D(SrcSampler2, uv);
    const float maskValue = tex2D(MaskSampler, uv).r;
    return lerp(originalColor, blurColor, maskValue);
}

technique GaussianH
{
    pass P0
    {
        PixelShader = compile ps_3_0 GaussianSparseH();
    }
}

technique GaussianV
{
    pass P0
    {
        PixelShader = compile ps_3_0 GaussianSparseV();
    }
}

technique CompositeMasked
{
    pass P0
    {
        PixelShader = compile ps_3_0 CompositeMaskedBlur();
    }
}
