// ゴッドレイ（シャフトオブライト）ポストエフェクト
//
// Pass1 OcclusionMask : シーンのZ画像を使い、光源より手前にあるピクセルを黒く塗る
// Pass2 GodRay        : オクルージョンマスクに対してレイマーチし、シーンと合成

// ---- 共通 ----
texture g_SceneTex;
texture g_OcclusionTex;

// 光源のスクリーンUV座標 (0..1)
float2 g_LightScreenPos = float2(0.5f, 0.3f);

float3 g_LightColor    = float3(1.0f, 0.9f, 0.8f);
float  g_RayLength     = 1.0f;
float  g_RayIntensity  = 0.6f;
float  g_OcclusionFalloff = 5.0f;

static const int SAMPLE_COUNT = 128;

// ---- Pass1: オクルージョンマスク ----
// Z画像からライトより奥にある部分を白、遮蔽部分を黒にする
texture g_ZTex;
float   g_LightViewZ;   // ライトのビュー空間Z（正規化済み near..far）
float   g_fNear;
float   g_fFar;

sampler g_SceneSampler = sampler_state
{
    Texture   = (g_SceneTex);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

sampler g_OcclusionSampler = sampler_state
{
    Texture   = (g_OcclusionTex);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

sampler g_ZSampler = sampler_state
{
    Texture   = (g_ZTex);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

struct VS_IN
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VS_OUT
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VS_OUT VS(VS_IN i)
{
    VS_OUT o;
    o.pos = i.pos;
    o.uv  = i.uv;
    return o;
}

// Pass1: オクルージョンマスク生成
// Z画像のα成分に線形深度が入っている（GBuffer.fx の RT0.a = linearZ）
// 光源のビュー空間線形Zより手前なら黒（遮蔽）、奥または同じなら白（透過）
float4 PS_OcclusionMask(VS_OUT i) : COLOR
{
    float pixelZ = tex2D(g_ZSampler, i.uv).a;

    // ピクセルが光源より手前にある = 遮蔽
    float mask = (pixelZ < g_LightViewZ) ? 0.0f : 1.0f;
    return float4(mask, mask, mask, 1.0f);
}

// Pass2: ゴッドレイ合成
float4 PS_GodRay(VS_OUT i) : COLOR
{
    float2 dir = g_LightScreenPos - i.uv;

    float visibilitySum   = 0.0f;
    float validSampleCount = 0.0f;

    [loop]
    for (int s = 0; s < SAMPLE_COUNT; ++s)
    {
        float t = g_RayLength * (float(s) / float(SAMPLE_COUNT - 1));
        float2 sampleUv = i.uv + dir * t;

        if (sampleUv.x >= 0.0f && sampleUv.x <= 1.0f &&
            sampleUv.y >= 0.0f && sampleUv.y <= 1.0f)
        {
            visibilitySum     += tex2D(g_OcclusionSampler, sampleUv).r;
            validSampleCount  += 1.0f;
        }
    }

    float lightRays = 0.0f;
    if (validSampleCount > 0.0f)
    {
        lightRays = visibilitySum / validSampleCount;
    }

    float occlusion = 1.0f - lightRays;
    lightRays = exp(-g_OcclusionFalloff * occlusion);

    float3 sceneColor = tex2D(g_SceneSampler, i.uv).rgb;
    float3 rayColor   = lightRays * g_RayIntensity * g_LightColor;
    return float4(sceneColor + rayColor, 1.0f);
}

technique OcclusionMask
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = FALSE;
        ZWriteEnable     = FALSE;
        AlphaBlendEnable = FALSE;
        VertexShader     = compile vs_3_0 VS();
        PixelShader      = compile ps_3_0 PS_OcclusionMask();
    }
}

technique GodRay
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = FALSE;
        ZWriteEnable     = FALSE;
        AlphaBlendEnable = FALSE;
        VertexShader     = compile vs_3_0 VS();
        PixelShader      = compile ps_3_0 PS_GodRay();
    }
}
