
// ============================
// ポストエフェクト（201×201, tap=25, step=8）
// サンプル: 0, ±8, ±16, …, ±96
// σ=40, 離散和(間引き)で正規化済み（1D和=1）
// ============================

bool g_bFilterON = false;

float2 g_TexelSize;
texture g_SrcTex;
sampler SrcSampler = sampler_state
{
    Texture = <g_SrcTex>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

// 奇数にすること 101くらいが限界
#define SAMPLE_SIZE_MAX 101
int g_sampleSize = 25;

// ---- 横方向 ----
float4 GaussianSparseH(float2 texCoord : TEXCOORD0) : COLOR
{
    float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (!g_bFilterON)
    {
        c = tex2D(SrcSampler, texCoord);
        return c;
    }
    
    float2 step = float2(g_TexelSize.x, 0.0);
    c = tex2D(SrcSampler, texCoord) / g_sampleSize;

    // 直接g_sampleSizeを使うことはできない。
    // コンパイル時に定数じゃないとfor文は使えないから
    [unroll]
    for (int i = 1; i <= SAMPLE_SIZE_MAX / 2; i++)
    {
        if ((g_sampleSize / 2) < i)
        {
            break;
        }

        c += tex2D(SrcSampler, texCoord + step * i) / g_sampleSize;
        c += tex2D(SrcSampler, texCoord - step * i) / g_sampleSize;
    }

    return c;
}

// ---- 縦方向 ----
float4 GaussianSparseV(float2 texCoord : TEXCOORD0) : COLOR
{

    float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (!g_bFilterON)
    {
        c = tex2D(SrcSampler, texCoord);
        return c;
    }
    
    float2 step = float2(0.0, g_TexelSize.y);
    c = tex2D(SrcSampler, texCoord) / g_sampleSize;

    // 直接g_sampleSizeを使うことはできない。
    // コンパイル時に定数じゃないとfor文は使えないから
    [unroll]
    for (int i = 1; i <= SAMPLE_SIZE_MAX / 2; i++)
    {
        if ((g_sampleSize / 2) < i)
        {
            break;
        }

        c += tex2D(SrcSampler, texCoord + step * i) / g_sampleSize;
        c += tex2D(SrcSampler, texCoord - step * i) / g_sampleSize;
    }

    return c;
}

technique GaussianH
{
    pass P0
    {
        PixelShader = compile ps_3_0 GaussianSparseH();
    }
}
technique GaussianV
{
    pass P0
    {
        PixelShader = compile ps_3_0 GaussianSparseV();
    }
}

