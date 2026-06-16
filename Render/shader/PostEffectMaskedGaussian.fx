bool g_bFilterON = false;

float2 g_TexelSize;
float g_FilterSpacing = 1.0f;
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

float2 g_MaskBaseSize = float2(1600.0f, 900.0f);
float2 g_MaskScreenSize = float2(1600.0f, 900.0f);
float2 g_MaskOffset = float2(0.0f, 0.0f);

float4 CompositeMaskedBlur(float2 uv : TEXCOORD0) : COLOR
{
    const float4 blurColor = tex2D(SrcSampler, uv);
    const float4 originalColor = tex2D(SrcSampler2, uv);
    float2 maskUV = uv;
    const float2 screenUV = uv * g_MaskScreenSize;
    maskUV = (screenUV - g_MaskOffset) / g_MaskBaseSize;
    const float maskValue = tex2D(MaskSampler, maskUV).r;
    return lerp(originalColor, blurColor, maskValue);
}

float4 Gaussian3x3(float2 uv)
{
    float2 ts = g_TexelSize * g_FilterSpacing;
    float4 sum = tex2D(SrcSampler, uv) * 4.0f;
    sum += tex2D(SrcSampler, uv + float2( ts.x, 0.0f)) * 2.0f;
    sum += tex2D(SrcSampler, uv + float2(-ts.x, 0.0f)) * 2.0f;
    sum += tex2D(SrcSampler, uv + float2(0.0f,  ts.y)) * 2.0f;
    sum += tex2D(SrcSampler, uv + float2(0.0f, -ts.y)) * 2.0f;
    sum += tex2D(SrcSampler, uv + float2( ts.x,  ts.y));
    sum += tex2D(SrcSampler, uv + float2( ts.x, -ts.y));
    sum += tex2D(SrcSampler, uv + float2(-ts.x,  ts.y));
    sum += tex2D(SrcSampler, uv + float2(-ts.x, -ts.y));
    return sum / 16.0f;
}

float4 Down3x3PS(float2 uv : TEXCOORD0) : COLOR
{
    return Gaussian3x3(uv);
}

float4 UpsampleOnly3x3PS(float2 uv : TEXCOORD0) : COLOR
{
    return Gaussian3x3(uv);
}

float4 CopyPS(float2 uv : TEXCOORD0) : COLOR
{
    return tex2D(SrcSampler, uv);
}

texture g_BlendTex;
sampler BlendSampler = sampler_state
{
    Texture = <g_BlendTex>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

float g_BlendAmount = 1.0f;

float4 BlendTwoPS(float2 uv : TEXCOORD0) : COLOR
{
    return lerp(tex2D(SrcSampler, uv), tex2D(BlendSampler, uv), g_BlendAmount);
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

technique Down3x3
{
    pass P0
    {
        PixelShader = compile ps_3_0 Down3x3PS();
    }
}

technique UpsampleOnly3x3
{
    pass P0
    {
        PixelShader = compile ps_3_0 UpsampleOnly3x3PS();
    }
}

technique Copy
{
    pass P0
    {
        PixelShader = compile ps_3_0 CopyPS();
    }
}

technique BlendTwo
{
    pass P0
    {
        PixelShader = compile ps_3_0 BlendTwoPS();
    }
}
