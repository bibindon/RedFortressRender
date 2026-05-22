float2 g_TexelSize;
bool g_HistoryValid = false;
float g_HistoryWeight = 0.85f;

texture g_CurrentTex;
sampler currentSampler = sampler_state
{
    Texture = (g_CurrentTex);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_HistoryTex;
sampler historySampler = sampler_state
{
    Texture = (g_HistoryTex);
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

float3 SampleCurrent(float2 uv, float2 offset)
{
    return tex2D(currentSampler, uv + offset).rgb;
}

void PixelShader1(in float2 inTexCoord : TEXCOORD0,
                  out float4 outColor  : COLOR0)
{
    float2 uv = inTexCoord;
    float3 currentColor = SampleCurrent(uv, 0.0f.xx);
    if (!g_HistoryValid)
    {
        outColor = float4(currentColor, 1.0f);
        return;
    }

    float3 c00 = SampleCurrent(uv, float2(-g_TexelSize.x, -g_TexelSize.y));
    float3 c10 = SampleCurrent(uv, float2(0.0f, -g_TexelSize.y));
    float3 c20 = SampleCurrent(uv, float2(g_TexelSize.x, -g_TexelSize.y));
    float3 c01 = SampleCurrent(uv, float2(-g_TexelSize.x, 0.0f));
    float3 c21 = SampleCurrent(uv, float2(g_TexelSize.x, 0.0f));
    float3 c02 = SampleCurrent(uv, float2(-g_TexelSize.x, g_TexelSize.y));
    float3 c12 = SampleCurrent(uv, float2(0.0f, g_TexelSize.y));
    float3 c22 = SampleCurrent(uv, float2(g_TexelSize.x, g_TexelSize.y));

    float3 minColor = min(min(min(c00, c10), min(c20, c01)),
                          min(min(currentColor, c21), min(c02, min(c12, c22))));
    float3 maxColor = max(max(max(c00, c10), max(c20, c01)),
                          max(max(currentColor, c21), max(c02, max(c12, c22))));

    float3 historyColor = tex2D(historySampler, uv).rgb;
    historyColor = clamp(historyColor, minColor, maxColor);

    float3 resolvedColor = lerp(currentColor, historyColor, g_HistoryWeight);
    outColor = float4(resolvedColor, 1.0f);
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
