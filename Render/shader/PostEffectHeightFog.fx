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

float4 g_FogColor = float4(0.72, 0.78, 0.86, 1.0);
float g_IntensityHeight = 0.3;
float g_HeightStart = 0.0;
float g_HeightMax = -5.0;
float g_PosRange = 50.0;

float HeightFogAmountAt(float2 uv)
{
    float3 wp01 = tex2D(sPos, uv).xyz;
    float3 wp = (wp01 * 2.0f - 1.0f) * g_PosRange;

    float amount = 0.0;
    if (g_HeightMax < g_HeightStart)
    {
        amount = saturate((g_HeightStart - wp.y) / (g_HeightStart - g_HeightMax));
    }
    else if (g_HeightMax > g_HeightStart)
    {
        amount = saturate((wp.y - g_HeightStart) / (g_HeightMax - g_HeightStart));
    }

    return saturate(amount * g_IntensityHeight);
}

struct PS_IN
{
    float2 uv : TEXCOORD0;
};

float4 PS_HeightFog(PS_IN i) : COLOR0
{
    float2 uv = i.uv;
    uv += (g_TexelSize * 0.5f);
    float3 scene = tex2D(sSrc, uv).rgb;
    float fog = HeightFogAmountAt(uv);
    float3 outColor = lerp(scene, g_FogColor.rgb, fog);
    return float4(outColor, 1.0);
}

technique TechHeightFog
{
    pass P0
    {
        PixelShader = compile ps_3_0 PS_HeightFog();
    }
}
