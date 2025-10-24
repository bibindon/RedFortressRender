// bloom.fx  (UTF-8 / BOMなし)
// 6方向スターバースト。長い光条向けチューニング版。
// - サンプラーは CLAMP（端の回り込み防止）
// - 方向性ブラーの重みを「指数減衰」に変更して遠距離の寄与を残す
// - STRETCH を拡大してサンプル間隔を広げ、見た目の伸びを強化

// ========= テクスチャ & サンプラー =========

texture g_SceneTex;
sampler SceneSampler = sampler_state
{
    Texture = <g_SceneTex>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_SrcTex;
sampler SrcSampler = sampler_state
{
    Texture = <g_SrcTex>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTexH; // 0°
sampler BlurSamplerH = sampler_state
{
    Texture = <g_BlurTexH>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTexV; // 60°
sampler BlurSamplerV = sampler_state
{
    Texture = <g_BlurTexV>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_BlurTex60; // 120°
sampler BlurSampler60 = sampler_state
{
    Texture = <g_BlurTex60>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

// ========= パラメータ =========

float g_Threshold = 2.8f; // 明部抽出のしきい値（高め＝コアだけ伸びる）
float2 g_TexelSize; // (1/width, 1/height)
float4 g_Direction; // (cosθ, sinθ, 0, 0)

// ========= シェーダ =========

// 明部抽出
float4 BrightPassPS(float2 uv : TEXCOORD0) : COLOR
{
    float4 c = tex2D(SrcSampler, uv);
    float lum = dot(c.rgb, float3(0.299, 0.587, 0.114));

    if (lum <= g_Threshold)
    {
        c = float4(0, 0, 0, 1);
    }

    return c;
}

#define SAMPLE_SIZE_MAX 101
int g_sampleSize = 101;

// 方向性 1D ブラー（指数減衰プロファイル）
// ガウシアンより遠距離の寄与が残るため、長い“筋”が出やすい。
float4 BlurPS(float2 uv : TEXCOORD0) : COLOR
{
    float2 step = g_TexelSize * g_Direction.xy; // 1px相当のUVステップ

    float4 sum = 0;

    static const int RADIUS = SAMPLE_SIZE_MAX / 2;

    [unroll]
    for (int i = 1; i <= RADIUS; i++)
    {
        if ((g_sampleSize / 2) < i)
        {
            break;
        }

        sum += tex2D(SrcSampler, uv + step * i) / g_sampleSize;
        sum += tex2D(SrcSampler, uv - step * i) / g_sampleSize;
    }

    return sum;
}

// 合成：Scene + 3軸ブラー（=6方向）
float4 CombinePS(float2 uv : TEXCOORD0) : COLOR
{
    float4 scene = tex2D(SceneSampler, uv);
    float4 b0 = tex2D(BlurSamplerH, uv); // 0°
    float4 b1 = tex2D(BlurSamplerV, uv); // 60°
    float4 b2 = tex2D(BlurSampler60, uv); // 120°

    const float gain = 1.2f; // 強すぎる場合は下げる(1.2〜2.0目安)
    return scene + (b0 + b1 + b2) * gain;
}

// ========= テクニック =========

technique BrightPass
{
    pass P0
    {
        PixelShader = compile ps_3_0 BrightPassPS();
    }
}

technique Blur
{
    pass P0
    {
        PixelShader = compile ps_3_0 BlurPS();
    }
}

technique Combine
{
    pass P0
    {
        PixelShader = compile ps_3_0 CombinePS();
    }
}
