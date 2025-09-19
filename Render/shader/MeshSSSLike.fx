float4x4 g_matWorldViewProj;
float4 g_lightPos = float4(-10.f, 10.f, -10.f, 0.0f);
float4 g_cameraPos = float4(10.f, 5.f, 10.f, 0.0f);
float3 g_ambient = float3(0.2f, 0.2f, 0.2f);

// スペキュラ
float g_SpecPower = 32.0f;
float g_SpecIntensity = 0.5f;
float3 g_SpecColor = float3(1, 1, 1);

// ★ 影の持ち上げ（まず明るくする）
float g_ShadowLift = 0.5f; // 0～1：影の最大持ち上げ量（0で無効）
float g_ShadowLiftGamma = 1.6f; // >1で“より暗部だけ”を持ち上げ

// ★ 影の色操作（明るくした後に適用）
float g_ShadowHueDegrees = 18.0f; // 影の色相回転（度）
float g_ShadowSatBoost = 2.15f; // 影の彩度ブースト（+15%）
float g_ShadowStrength = 3.0f; // 影色のブレンド強度 0～1
float g_ShadowGamma = 1.2f; // 影判定のカーブ（>1で深部寄りに効く）

texture texture1;
sampler textureSampler = sampler_state
{
    Texture = (texture1);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

struct VSIn
{
    float4 pos : POSITION;
    float3 nrm : NORMAL0;
    float2 uv : TEXCOORD0;
};
struct VSOut
{
    float4 pos : POSITION;
    float3 opos : TEXCOORD0;
    float3 onrm : TEXCOORD1;
    float2 uv : TEXCOORD2;
};

VSOut VertexShader1(VSIn i)
{
    VSOut o;
    o.pos = mul(i.pos, g_matWorldViewProj);
    o.opos = i.pos.xyz;
    o.onrm = i.nrm;
    o.uv = i.uv;
    return o;
}

// RGBの色相回転＆彩度変更（YIQ空間）
float3 AdjustHueSat_YIQ(float3 rgb, float hueRad, float satMul)
{
    float Y = dot(rgb, float3(0.299, 0.587, 0.114));
    float I = dot(rgb, float3(0.596, -0.274, -0.322));
    float Q = dot(rgb, float3(0.212, -0.523, 0.311));

    float2 iq = float2(I, Q) * satMul;
    float s = sin(hueRad), c = cos(hueRad);
    float2 iqR = float2(c * iq.x - s * iq.y, s * iq.x + c * iq.y);

    float3 outRGB;
    outRGB.r = Y + 0.956 * iqR.x + 0.621 * iqR.y;
    outRGB.g = Y - 0.272 * iqR.x - 0.647 * iqR.y;
    outRGB.b = Y - 1.106 * iqR.x + 1.703 * iqR.y;
    return outRGB;
}

float4 PixelShader1(VSOut i) : COLOR0
{
    float3 N = normalize(i.onrm);
    float3 L = normalize(g_lightPos.xyz - i.opos);
    float3 V = normalize(g_cameraPos.xyz - i.opos);
    float3 H = normalize(L + V);

    float NdotL_raw = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));

    // --- 1) 影の“持ち上げ”を先に適用（暗いところほど足し込む）
    //     NdotL_lifted = NdotL_raw + lift * (1 - NdotL_raw)^gamma
    float NdotL_lifted = saturate(NdotL_raw + g_ShadowLift * pow(1.0f - NdotL_raw, g_ShadowLiftGamma));

    float3 albedo = tex2D(textureSampler, i.uv).rgb;

    // --- 2) 影判定（持ち上げ後のN·Lで判定）→ 色相/彩度を影だけに
    float t = saturate(pow(1.0f - NdotL_lifted, g_ShadowGamma)); // 0=明るい, 1=影
    float hueRad = (g_ShadowHueDegrees * 0.01745329252f) * t; // deg→rad
    float satMul = 1.0f + (g_ShadowSatBoost * t);

    float3 albedoShadowed = AdjustHueSat_YIQ(albedo, hueRad, satMul);
    float3 albedoFinal = lerp(albedo, albedoShadowed, saturate(g_ShadowStrength * t));

    // --- 3) ライティング
    float3 diffuse = albedoFinal * NdotL_lifted; // ← 持ち上げ後のN·Lを使用
    float3 ambient = albedoFinal * g_ambient;
    float3 spec = g_SpecColor * (pow(NdotH, g_SpecPower) * g_SpecIntensity);

    float3 color = ambient + diffuse + spec;
    return float4(saturate(color), 1.0f);
}

technique Technique1
{
    pass P0
    {
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
    }
}
