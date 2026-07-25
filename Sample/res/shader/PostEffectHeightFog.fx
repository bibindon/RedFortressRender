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

float4 g_FogColor = float4(0.72, 0.78, 0.86, 1.0);
float g_IntensityHeight = 0.3;
float g_HeightStart = 0.0;
float g_HeightMax = -5.0;
float g_DistanceStart = 0.0;
float g_DistanceMax = 20.0;
float g_DepthDecodeNear = 0.1;
float g_DepthDecodeFar = 30000.0;
float4 g_CameraPos = float4(0.0, 0.0, 0.0, 1.0);
float4x4 g_InvView;
float2 g_ProjectionScale = float2(1.0, 1.0);
float2 g_ProjectionOffset = float2(0.0, 0.0);

float3 ReconstructWorldPosition(float2 uv)
{
    float encodedDepth = tex2D(sZ, uv).g;
    float viewZ = lerp(g_DepthDecodeNear, g_DepthDecodeFar, saturate(encodedDepth));
    float2 ndc = float2((uv.x * 2.0f) - 1.0f, 1.0f - (uv.y * 2.0f));
    float3 viewPos = float3((ndc.x - g_ProjectionOffset.x) * viewZ / g_ProjectionScale.x,
                            (ndc.y - g_ProjectionOffset.y) * viewZ / g_ProjectionScale.y,
                            viewZ);
    return mul(float4(viewPos, 1.0f), g_InvView).xyz;
}

float HeightFogDistanceAmountAt(float2 uv)
{
    if (g_DistanceMax <= g_DistanceStart)
    {
        return 1.0f;
    }

    float encodedDepth = tex2D(sZ, uv).g;
    float decodedDepth = lerp(g_DepthDecodeNear, g_DepthDecodeFar, saturate(encodedDepth));
    return saturate((decodedDepth - g_DistanceStart) / (g_DistanceMax - g_DistanceStart));
}

float HeightFogAmountAt(float2 uv)
{
    float3 wp = ReconstructWorldPosition(uv);

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

    float fogByDistance = HeightFogDistanceAmountAt(uv);

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
