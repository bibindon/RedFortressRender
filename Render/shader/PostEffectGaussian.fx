
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

// 正規化済み（sum = w0 + 2*Σw[i] = 1）
static const float w[13] =
{
    0.0807799342, // 0
    0.0791803843, // ±8
    0.0745692777, // ±16
    0.0674730727, // ±24
    0.0586582714, // ±32
    0.0489955068, // ±40
    0.0393198152, // ±48
    0.0303176059, // ±56
    0.0224598348, // ±64
    0.0159862439, // ±72
    0.0109323753, // ±80
    0.0071830824, // ±88
    0.0045345624 // ±96
};

// ---- 横方向 ----
float4 GaussianSparseH(float2 texCoord : TEXCOORD0) : COLOR
{
    float2 step = float2(g_TexelSize.x, 0.0);

    float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (g_bFilterON)
    {
        c = tex2D(SrcSampler, texCoord) * w[0];

        [unroll]
        for (int i = 1; i <= 12; i++)
        {
            float ofs = (float) (i * 8); // 8,16,...,96
            c += tex2D(SrcSampler, texCoord + step * ofs) * w[i];
            c += tex2D(SrcSampler, texCoord - step * ofs) * w[i];
        }
    }
    else
    {
        c = tex2D(SrcSampler, texCoord);
    }

    return c;
}

// ---- 縦方向 ----
float4 GaussianSparseV(float2 texCoord : TEXCOORD0) : COLOR
{
    float2 step = float2(0.0, g_TexelSize.y);

    float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (g_bFilterON)
    {
        c = tex2D(SrcSampler, texCoord) * w[0];

        [unroll]
        for (int i = 1; i <= 12; i++)
        {
            float ofs = (float) (i * 8);
            c += tex2D(SrcSampler, texCoord + step * ofs) * w[i];
            c += tex2D(SrcSampler, texCoord - step * ofs) * w[i];
        }
    }
    else
    {
        c = tex2D(SrcSampler, texCoord);
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

