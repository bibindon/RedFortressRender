float2 g_TexelSize;
int g_sampleSize = 21;

texture g_SrcTex;
sampler SrcSampler = sampler_state
{
    Texture = <g_SrcTex>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

#define SAMPLE_SIZE_MAX 21

float4 GaussianBlur(float2 texCoord, float2 stepDir) : COLOR
{
    const int radius = SAMPLE_SIZE_MAX / 2;
    const int activeRadius = min(radius, g_sampleSize / 2);

    float4 color = tex2D(SrcSampler, texCoord);
    float weightSum = 1.0f;

    [unroll]
    for (int i = 1; i <= radius; ++i)
    {
        if (activeRadius < i)
        {
            break;
        }

        const float2 offset = stepDir * i;
        color += tex2D(SrcSampler, texCoord + offset);
        color += tex2D(SrcSampler, texCoord - offset);
        weightSum += 2.0f;
    }

    return color / weightSum;
}

float4 GaussianBlurHPS(float2 texCoord : TEXCOORD0) : COLOR
{
    return GaussianBlur(texCoord, float2(g_TexelSize.x, 0.0f));
}

float4 GaussianBlurVPS(float2 texCoord : TEXCOORD0) : COLOR
{
    return GaussianBlur(texCoord, float2(0.0f, g_TexelSize.y));
}

float4 GaussianBlurVCompositePS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float4 blurred = GaussianBlur(texCoord, float2(0.0f, g_TexelSize.y));
    const float blurColor = 0.5f;
    return float4(blurColor, blurColor, 1.f, blurred.a);
}

technique GaussianH
{
    pass P0
    {
        PixelShader = compile ps_3_0 GaussianBlurHPS();
    }
}

technique GaussianV
{
    pass P0
    {
        PixelShader = compile ps_3_0 GaussianBlurVPS();
    }
}

technique GaussianVComposite
{
    pass P0
    {
        PixelShader = compile ps_3_0 GaussianBlurVCompositePS();
    }
}
