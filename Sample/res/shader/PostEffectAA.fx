float2 g_TexelSize;
float g_LuminanceThreshold = 0.18f;

texture g_SrcTex;
sampler srcSampler = sampler_state
{
    Texture = (g_SrcTex);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

void VertexShader1(in  float4 inPosition  : POSITION,
                   in  float2 inTexCoord  : TEXCOORD0,
                   out float4 outPosition : POSITION,
                   out float2 outTexCoord : TEXCOORD0)
{
    outPosition = inPosition;
    outTexCoord = inTexCoord;
}

float Luminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float3 SampleColor(float2 uv, float2 offset)
{
    return tex2D(srcSampler, uv + offset).rgb;
}

void PixelShader1(in float2 inTexCoord : TEXCOORD0,
                  out float4 outColor   : COLOR0)
{
    float2 uv = inTexCoord;

    float3 c00 = SampleColor(uv, float2(-g_TexelSize.x, -g_TexelSize.y));
    float3 c10 = SampleColor(uv, float2(0.0f, -g_TexelSize.y));
    float3 c20 = SampleColor(uv, float2(g_TexelSize.x, -g_TexelSize.y));
    float3 c01 = SampleColor(uv, float2(-g_TexelSize.x, 0.0f));
    float3 c11 = SampleColor(uv, float2(0.0f, 0.0f));
    float3 c21 = SampleColor(uv, float2(g_TexelSize.x, 0.0f));
    float3 c02 = SampleColor(uv, float2(-g_TexelSize.x, g_TexelSize.y));
    float3 c12 = SampleColor(uv, float2(0.0f, g_TexelSize.y));
    float3 c22 = SampleColor(uv, float2(g_TexelSize.x, g_TexelSize.y));

    float l00 = Luminance(c00);
    float l10 = Luminance(c10);
    float l20 = Luminance(c20);
    float l01 = Luminance(c01);
    float l11 = Luminance(c11);
    float l21 = Luminance(c21);
    float l02 = Luminance(c02);
    float l12 = Luminance(c12);
    float l22 = Luminance(c22);

    float minLuma = min(min(min(l00, l10), min(l20, l01)),
                        min(min(l11, l21), min(l02, min(l12, l22))));
    float maxLuma = max(max(max(l00, l10), max(l20, l01)),
                        max(max(l11, l21), max(l02, max(l12, l22))));

    if ((maxLuma - minLuma) < g_LuminanceThreshold)
    {
        outColor = float4(c11, 1.0f);
        return;
    }

    float3 blurredColor =
        (c00 + (c10 * 2.0f) + c20 +
         (c01 * 2.0f) + (c11 * 4.0f) + (c21 * 2.0f) +
         c02 + (c12 * 2.0f) + c22) / 16.0f;

    outColor = float4(blurredColor, 1.0f);
}

technique Technique1
{
    pass Pass1
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
    }
}
