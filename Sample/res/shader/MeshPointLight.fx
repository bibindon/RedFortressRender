// simple.fx — Physically-based diffuse (Lambert × 1/r^2) + soft core/edge
// D3D9 / ps_3_0。最小構成＆調整用パラメータあり。

float4x4 g_matWorldViewProj;
float4x4 g_matWorld;

struct PointLight
{
    float3 pos;
    float range; // 有効レンジ（見栄え用のカットに使用）
    float3 color;
    float pad;
};

static const int MAX_LIGHTS = 10;
PointLight g_pointLights[MAX_LIGHTS];
int g_numLights; // C++側で SetInt("g_numLights", 10) を忘れずに

// 調整用パラメータ
float g_lightIntensity = 3000.0; // 全体の明るさ（例: 800〜3000）
float g_ambient = 0.20; // 環境光
float g_coreRadius = 10.0; // “光源半径”の擬似（中心を柔らかく）[world units]
float g_falloffStartRatio = 0.60; // レンジ開始比（0.0〜1.0未満）。大きいほどエッジが立つ

texture texture1;
sampler textureSampler = sampler_state
{
    Texture = (texture1);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

void VS(in float4 inPosition : POSITION,
        in float3 inNormal : NORMAL0,
        in float2 inTexCoord : TEXCOORD0,

        out float4 outPos : POSITION,
        out float3 outWorldPos : TEXCOORD0,
        out float3 outNormal : TEXCOORD1,
        out float2 outTex : TEXCOORD2)
{
    outPos = mul(inPosition, g_matWorldViewProj);
    outWorldPos = mul(inPosition, g_matWorld).xyz;

    // 等方スケール前提（非等方なら逆転置行列に変更）
    outNormal = normalize(mul(inNormal, (float3x3) g_matWorld));
    outTex = inTexCoord;
}

void PS(in float3 inWorldPos : TEXCOORD0,
        in float3 inNormal : TEXCOORD1,
        in float2 inTex : TEXCOORD2,
        out float4 outColor : COLOR)
{
    float3 albedo = tex2D(textureSampler, inTex).rgb;
    float3 N = normalize(inNormal);

    float3 sumIrradiance = 0.0;

    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        float enabled = (i < g_numLights) ? 1.0 : 0.0;

        float3 Lvec = g_pointLights[i].pos - inWorldPos;

        // --- 中心を柔らかくする擬似“光源半径” ---
        float r2 = dot(Lvec, Lvec) + g_coreRadius * g_coreRadius;
        r2 = max(r2, 1.0); // 発散抑制
        float invR = rsqrt(r2);
        float3 L = Lvec * invR; // normalize(Lvec)
        float r = 1.0 / invR;

        // 物理：Lambert × 逆二乗
        float NdotL = max(dot(N, L), 0.0);
        float E = NdotL * (invR * invR);

        // --- エッジの柔らかさ（レンジ内をスムーズに0へ） ---
        float range = g_pointLights[i].range;
        float start = g_falloffStartRatio * range; // 例: 0.6R
        float denom = max(range - start, 1e-3);
        float t = saturate((r - start) / denom); // 0..1
        float falloff = 1.0 - (t * t * (3.0 - 2.0 * t)); // 1 - smoothstep

        sumIrradiance += enabled * (g_pointLights[i].color * (E * falloff));
    }

    float3 color = g_ambient * albedo + albedo * (sumIrradiance * g_lightIntensity);
    outColor = float4(saturate(color), 1.0);
}

technique Technique1
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS();
    }
}
