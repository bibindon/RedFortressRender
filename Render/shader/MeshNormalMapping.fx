// simple.fx — Normal Mapping（頂点 TANGENT/BINORMAL 使用／構造体なしI/O／三項演算子なし）
// 前提：RGBノーマルマップ（rgb*2-1）。sRGB補正はかけないでサンプリング。
// 定数名は main.cpp と一致（g_colorMap / g_normalMap / g_lightDirectionWS）。

float4x4 g_matWorldViewProj;
float4x4 g_matWorld;

// 光が進む方向（ワールド）。Lambert では -方向 を使用。
float4 g_lightDirectionWS;

// 照明（簡易）
float3 g_ambientColor = float3(0.25, 0.25, 0.25);
float3 g_lightColor = float3(1.5, 1.5, 1.5);
float g_diffuseGain = 2.0;

texture g_colorMap;
texture g_normalMap;

sampler sColor = sampler_state
{
    Texture = <g_colorMap>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};
sampler sNormal = sampler_state
{
    Texture = <g_normalMap>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

// ===== Vertex Shader =====
void VS(float4 inPos : POSITION,
        float3 inNormOS : NORMAL0,
        float3 inTangentOS : TANGENT0,
        float3 inBinormalOS : BINORMAL0,
        float2 inUV : TEXCOORD0,

        out float4 outPos : POSITION,
        out float3 wsNorm : TEXCOORD0,
        out float3 wsTangent : TEXCOORD1,
        out float3 wsBinorm : TEXCOORD2,
        out float2 outUV : TEXCOORD3)
{
    float3x3 world3x3 = (float3x3) g_matWorld; // 等方スケール前提

    outPos = mul(inPos, g_matWorldViewProj);
    wsNorm = normalize(mul(inNormOS, world3x3));
    wsTangent = normalize(mul(inTangentOS, world3x3));
    wsBinorm = normalize(mul(inBinormalOS, world3x3));

    // 直交性の調整（T を N に直交化）
    wsTangent = normalize(wsTangent - wsNorm * dot(wsNorm, wsTangent));

    outUV = inUV;
}

// ===== Pixel Shader =====
float4 PS(float3 wsNorm : TEXCOORD0,
          float3 wsTangent : TEXCOORD1,
          float3 wsBinorm : TEXCOORD2,
          float2 uv : TEXCOORD3) : COLOR
{
    // アルベド
    float3 albedo = tex2D(sColor, uv).rgb;

    float3 normalInTangent = float3(0, 0, 0);
    normalInTangent.x = tex2D(sNormal, uv).r * 2.0 - 1.0;
    normalInTangent.y = tex2D(sNormal, uv).g * 2.0 - 1.0;
    normalInTangent.z = tex2D(sNormal, uv).b * 2.0 - 1.0;
    normalInTangent.x *= -1;
    normalInTangent = normalize(normalInTangent);

    // TBN（Tangent, Binormal, Normal）でワールドへ
    float3x3 tangentToWorld = float3x3(-wsTangent, -wsBinorm, wsNorm);
    float3 normalInWorld = normalize(mul(normalInTangent, tangentToWorld));

    // Lambert 拡散（-光線方向）
    float3 L = normalize(g_lightDirectionWS.xyz);
    float ndotl = dot(normalInWorld, L);
    if (ndotl < 0.0)
    {
        ndotl = 0.0;
    }

    float3 lit = g_ambientColor + g_lightColor * (ndotl * g_diffuseGain);
    float3 color = albedo * lit;

    return float4(saturate(color), 1.0);
}

technique Technique_NormalMap_TBN
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS();
    }
}
