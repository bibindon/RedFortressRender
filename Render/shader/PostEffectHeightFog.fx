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
float g_DistanceStart = 0.0;
float g_DistanceMax = 20.0;
float g_PosRange = 50.0;
float4 g_CameraPos = float4(0.0, 0.0, 0.0, 1.0);

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

    float fogByHeight = saturate(amount * g_IntensityHeight);

    float fogByDistance = 1.0f;
    if (g_DistanceMax > g_DistanceStart)
    {
        float distanceToCamera = distance(wp, g_CameraPos.xyz);
        fogByDistance = saturate((distanceToCamera - g_DistanceStart) / (g_DistanceMax - g_DistanceStart));
    }

    return saturate(fogByHeight * fogByDistance);
}

struct PS_IN
{
    float2 uv : TEXCOORD0;
};

float4 PS_HeightFog(PS_IN i) : COLOR0
{
    float2 uv = i.uv;
    float3 scene = tex2D(sSrc, uv).rgb;

     // なぜか1ピクセル右下のピクセルを見るとうまくいく。
     uv += (g_TexelSize * 0.5f);
     uv += (g_TexelSize * 0.5f);

    float fog = HeightFogAmountAt(uv);
    float3 outColor = lerp(scene, g_FogColor.rgb, fog);

    if (false)
    {
        float2 pixelPos = uv / g_TexelSize;
        float gridX = frac(pixelPos.x / 5.0f);
        float gridY = frac(pixelPos.y / 5.0f);
        if (gridX < 0.2f || gridY < 0.2f)
        {
            outColor = float3(0.0f, 1.0f, 0.0f);
        }
    }

    return float4(outColor, 1.0);
}

technique TechHeightFog
{
    pass P0
    {
        PixelShader = compile ps_3_0 PS_HeightFog();
    }
}
