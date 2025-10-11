// bloom.fx

// === 入力テクスチャ ===
// 元のシーン
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

// 処理対象（BrightPassやBlurの入力として使う）
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

// ブラー済みテクスチャ（Combineで使用）
texture g_BlurTex;
sampler BlurSampler = sampler_state
{
    Texture = <g_BlurTex>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

// === パラメータ ===
float g_Threshold = 0.5f; // 輝度しきい値
float2 g_TexelSize; // (1/width, 1/height) : ブラー用

// === BrightPass ===
float4 BrightPassPS(float2 texCoord : TEXCOORD0) : COLOR
{
    float4 c = tex2D(SrcSampler, texCoord);
    float lum = dot(c.rgb, float3(0.299, 0.587, 0.114));
    if (lum > g_Threshold)
    {
        return c;
    }
    return float4(0, 0, 0, 1);
}

#define SAMPLE_SIZE_MAX 101
int g_sampleSize = 25;

float4 g_Direction;

// bloom.fx の BlurPS 改造版
float4 BlurPS(float2 texCoord : TEXCOORD0) : COLOR
{
    // g_Direction = (1,0) のとき横、(0,1) のとき縦
    float2 step = g_TexelSize * g_Direction.xy;

    float4 sum = 0;

    // 半径固定（7 → 15tap）
    static const int RADIUS = SAMPLE_SIZE_MAX / 2; // 奇数

    [unroll]
    for (int i = 1; i <= RADIUS; i++)
    {
        if ((g_sampleSize / 2) < i)
        {
            break;
        }

        sum += tex2D(SrcSampler, texCoord + step * i) / g_sampleSize;
        sum += tex2D(SrcSampler, texCoord - step * i) / g_sampleSize;
    }

    return sum;
}

// === Combine ===
// SceneTex + BlurTex を加算合成
float4 CombinePS(float2 texCoord : TEXCOORD0) : COLOR
{
    float4 scene = tex2D(SceneSampler, texCoord);
    float4 bloom = tex2D(BlurSampler, texCoord);

    // ブルームの濃さ
    float4 outColor = scene + bloom * 0.5f;
    outColor.a = 1.f;
    return outColor;
}

// === Techniques ===
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

