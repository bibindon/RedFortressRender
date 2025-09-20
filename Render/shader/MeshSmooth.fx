
float4x4 g_matWorldViewProj;
float4 g_lightPos = float4(-10.f, 10.f, -10.f, 0.0f);
float4 g_cameraPos = float4(10.f, 5.f, 10.f, 0.0f);
float3 g_ambient = float3(0.2f, 0.2f, 0.2f);

// ピクセル・スペキュラの調整用
float g_SpecPower = 128.0f; // 鋭さ（ハイライトの広がり）
float g_SpecIntensity = 1.0f; // 強さ
float3 g_SpecColor = float3(1, 1, 1); // 色（必要なら変更）

texture texture1;
sampler textureSampler = sampler_state
{
    Texture = (texture1);
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MaxAnisotropy = 8;
};

// -------------------- VS: 位置変換＋PS用属性の受け渡し --------------------
struct VSIn
{
    float4 pos : POSITION;
    float3 nrm : NORMAL0;
    float2 uv : TEXCOORD0;
};

struct VSOut
{
    float4 pos : POSITION;
    float3 opos : TEXCOORD0; // オブジェクト座標（=ワールド座標、World=I想定）
    float3 onrm : TEXCOORD1; // オブジェクト法線
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

// -------------------- PS: 拡散＋スペキュラをピクセルで計算 --------------------
float4 PixelShader1(VSOut i) : COLOR0
{
    // オブジェクト空間で計算（World=Iのため）。将来World≠Iにするなら here をワールドに合わせて修正。
    float3 N = normalize(i.onrm);
    float3 L = normalize(g_lightPos.xyz - i.opos);
    float3 V = normalize(g_cameraPos.xyz - i.opos);
    float3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));

    float3 albedo = tex2D(textureSampler, i.uv).rgb;

    // 拡散（Lambert）＋ 環境
    float3 diffuse = albedo * NdotL;
    float3 ambient = albedo * g_ambient;

    // スペキュラ（Blinn-Phong）
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

