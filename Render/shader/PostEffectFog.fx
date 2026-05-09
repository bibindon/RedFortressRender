// ------------------------------------------------------------
// PostEffectFog.fx  (DX9 ps_3_0)
//   - Exponential fog based on decoded linear depth
//   - Optional height fog blended additively
//   - Final color = lerp(Scene, FogColor, fogAmount)
// ------------------------------------------------------------

float2 g_TexelSize;

texture g_SrcTex;
sampler2D sSrc = sampler_state
{
    Texture = <g_SrcTex>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_ZTex;
sampler2D sZ = sampler_state
{
    Texture = <g_ZTex>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_PosTex;
sampler2D sPos = sampler_state
{
    Texture = <g_PosTex>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

float4 g_FogColor = float4(0.32, 0.38, 0.86, 1.0);
float g_IntensityZ = 0.02;
float g_IntensityHeight = 0.00;
float g_HeightStart = 0.00;
float g_PosRange = 1000.0;
float g_DepthDecodeNear = 0.1;
float g_DepthDecodeFar = 30000.0;
float g_FogNear = 0.1;
float g_FogFar = 30000.0;

bool g_EnableZ = true;
bool g_EnableHeight = false;

float FogAmountAt(float2 uv)
{
    float fogZ = 0.0;
    float fogH = 0.0;

    if (g_EnableZ)
    {
        float encodedDepth = tex2D(sZ, uv).r;
        float decodedDepth = lerp(g_DepthDecodeNear, g_DepthDecodeFar, saturate(encodedDepth));
        float fogDepth = saturate((decodedDepth - g_FogNear) / max(g_FogFar - g_FogNear, 0.0001));
        float trans = exp(-g_IntensityZ * fogDepth);
        fogZ = 1.0 - trans;
    }

    if (g_EnableHeight)
    {
        float3 wp01 = tex2D(sPos, uv).xyz;
        float3 wp = (wp01 * 2.0 - 1.0) * g_PosRange;
        float hDelta = max(0.0, g_HeightStart - wp.y);
        float trans = exp(-g_IntensityHeight * hDelta);
        fogH = 1.0 - trans;
    }

    return saturate(fogZ + fogH);
}

struct PS_IN
{
    float2 uv : TEXCOORD0;
};

float4 PS_Fog(PS_IN i) : COLOR0
{
    float2 uv = i.uv + g_TexelSize;
    float3 scene = tex2D(sSrc, uv).rgb;
    float fog = FogAmountAt(uv);
    float3 outColor = lerp(scene, g_FogColor.rgb, saturate(fog));
    return float4(outColor, 1.0);
}

technique TechFog
{
    pass P0
    {
        PixelShader = compile ps_3_0 PS_Fog();
    }
}
