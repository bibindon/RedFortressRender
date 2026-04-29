
// ------------------------------------------------------------
// PostEffectFog.fx  (DX9 ps_3_0)
//   ・Z距離ベースの指数霧
//   ・高さ霧（HeightStart より下で濃くなる簡易モデル）
//   ・合成は “加算” ： fogAmount = saturate( fogZ + fogH )
//   ・最終色 = lerp(Scene, FogColor, fogAmount)
//   ・前提：Zテクスチャは「線形距離」が格納されている
// ------------------------------------------------------------

float2  g_TexelSize;

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

texture g_ZTex;   // Rに線形距離
sampler2D sZ = sampler_state
{
    Texture = <g_ZTex>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_PosTex; // world-space position (xyz)
sampler2D sPos = sampler_state
{
    Texture = <g_PosTex>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

// Parameters
float4 g_FogColor        = float4(0.32, 0.38, 0.86, 1.0);
float  g_IntensityZ      = 0.02;   // 距離密度
float  g_IntensityHeight = 0.00;   // 高さ密度
float  g_HeightStart     = 0.00;   // この高さより下で濃くなる

bool   g_EnableZ         = true;
bool   g_EnableHeight    = false;

float FogAmountAt(float2 uv)
{
    float fogZ = 0.0;
    float fogH = 0.0;

    if (g_EnableZ)
    {
        float d = tex2D(sZ, uv).r;               // 線形距離前提
        float trans = exp(-g_IntensityZ * d);
        fogZ = 1.0 - trans;
    }

    if (g_EnableHeight)
    {
        float3 wp = tex2D(sPos, uv).xyz;
        float hDelta = max(0.0, g_HeightStart - wp.y);
        float trans = exp(-g_IntensityHeight * hDelta);
        fogH = 1.0 - trans;
    }

    return saturate(fogZ + fogH);                 // ご要望どおり「加算」
}

struct PS_IN
{
    float2 uv : TEXCOORD0;
};

float4 PS_Fog(PS_IN i) : COLOR0
{
    // 中心UV（半テクセル補正は入力ごとに個別に）
    float2 uv= i.uv + g_TexelSize;

    float3 scene = tex2D(sSrc, uv).rgb;

    // 1px ステップ = 2 * HalfPixel
    float2 stepZ   = g_TexelSize * 2.0;
    float2 stepPos = g_TexelSize * 2.0;

    // 中心フォグ
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
