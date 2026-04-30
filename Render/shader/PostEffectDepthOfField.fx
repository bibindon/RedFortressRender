float2 g_TexelSize = float2(1.0 / 1600.0, 1.0 / 900.0);
float4 g_cameraPos = float4(0.0, 0.0, 0.0, 1.0);
float g_focalDistanceMeters = 8.0;
float g_focusBandHalfWidthMeters = 2.0;
float g_blurRadiusPixels = 1.0;
float g_positionRange = 50.0;

// 焦点範囲の外側から、何mごとに 3x3 -> 5x5 -> 7x7 -> 9x9 -> 11x11 と強くするか。
// C++ 側から渡さなくても、この初期値で動作します。
float g_blurStepMeters = 2.0;

texture g_SrcTex;
sampler colorSampler = sampler_state
{
    Texture = (g_SrcTex);
    MinFilter = NONE;
    MagFilter = NONE;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_PositionTex;
sampler positionSampler = sampler_state
{
    Texture = (g_PositionTex);
    MinFilter = NONE;
    MagFilter = NONE;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

void VS(in float4 inPos : POSITION,
        in float2 inUv : TEXCOORD0,
        out float4 outPos : POSITION,
        out float2 outUv : TEXCOORD0)
{
    outPos = inPos;
    outUv = inUv;
}

float4 DecodeWorldPosition(float2 uv)
{
    float4 encoded = tex2D(positionSampler, uv);
    float3 worldPos = ((encoded.xyz * 2.0f) - 1.0f) * g_positionRange;
    return float4(worldPos, encoded.a);
}

float GetDistanceMeters(float2 uv, out float valid)
{
    float4 worldPos = DecodeWorldPosition(uv);
    valid = worldPos.w;
    return length(worldPos.xyz - g_cameraPos.xyz);
}

bool IsInFocusRange(float distanceMeters)
{
    return abs(distanceMeters - g_focalDistanceMeters) <= g_focusBandHalfWidthMeters;
}

int GetBlurHalfSize(float distanceMeters)
{
    float distanceFromFocus = abs(distanceMeters - g_focalDistanceMeters);
    float outOfFocusDistance = distanceFromFocus - g_focusBandHalfWidthMeters;

    if (outOfFocusDistance <= 0.0f)
    {
        return 0;
    }

    if (outOfFocusDistance < g_blurStepMeters * 1.0f)
    {
        return 1; // 3x3
    }

    if (outOfFocusDistance < g_blurStepMeters * 2.0f)
    {
        return 2; // 5x5
    }

    if (outOfFocusDistance < g_blurStepMeters * 3.0f)
    {
        return 3; // 7x7
    }

    if (outOfFocusDistance < g_blurStepMeters * 4.0f)
    {
        return 4; // 9x9
    }

    return 5; // 11x11
}

float GetGaussianWeight(int x, int y, int halfSize)
{
    float fx = (float)x;
    float fy = (float)y;
    float dist2 = fx * fx + fy * fy;

    // 小さいカーネルでは狭く、大きいカーネルでは広くする。
    float sigma = max(1.0f, (float)halfSize * 0.75f);
    float sigma2 = sigma * sigma;

    return exp(-dist2 / (2.0f * sigma2));
}

// ps_3_0 では POSITION をピクセルシェーダー入力にしない。
// 画面位置は使わず、元の uv と sampleUv の扱いを維持する。
float4 PS(in float2 uv : TEXCOORD0) : COLOR0
{
    // 元のコードと同じ 0.5 texel 補正を残す。
    float2 sampleUv = uv + g_TexelSize * 0.5f;

    // 0.5ピクセルずれ確認用のデバッグ表示。
    // 5ピクセルごとに縦線・横線を描く。
    if (false)
    {
        float2 pixelCoord = floor(sampleUv / g_TexelSize);
        bool isGridLine = (fmod(pixelCoord.x, 5.0f) == 0.0f) || (fmod(pixelCoord.y, 5.0f) == 0.0f);
        if (isGridLine)
        {
            return float4(0.0f, 1.0f, 0.0f, 1.0f);
        }

        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    float4 baseColor = tex2D(colorSampler, sampleUv);

    float centerValid = 0.0f;
    float centerDistanceMeters = GetDistanceMeters(sampleUv, centerValid);
    if (centerValid <= 0.0f)
    {
        return baseColor;
    }

    int blurHalfSize = GetBlurHalfSize(centerDistanceMeters);
    if (blurHalfSize <= 0)
    {
        return baseColor;
    }

    float4 sumColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float weightSum = 0.0f;

    const int maxHalfSize = 5;

    [unroll]
    for (int y = -maxHalfSize; y <= maxHalfSize; ++y)
    {
        [unroll]
        for (int x = -maxHalfSize; x <= maxHalfSize; ++x)
        {
            if (x < -blurHalfSize || x > blurHalfSize || y < -blurHalfSize || y > blurHalfSize)
            {
                continue;
            }

            float2 offset = float2((float)x, (float)y) * g_TexelSize * g_blurRadiusPixels;
            float2 tapUv = sampleUv + offset;

            float tapValid = 0.0f;
            float tapDistanceMeters = GetDistanceMeters(tapUv, tapValid);
            if (tapValid <= 0.0f)
            {
                continue;
            }

            // くっきり表示される範囲はサンプリングに混ぜない。
            if (IsInFocusRange(tapDistanceMeters))
            {
                continue;
            }

            float weight = GetGaussianWeight(x, y, blurHalfSize);
            sumColor += tex2D(colorSampler, tapUv) * weight;
            weightSum += weight;
        }
    }

    if (weightSum <= 0.0f)
    {
        return baseColor;
    }

    return sumColor / weightSum;
}

technique Technique1
{
    pass P0
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS();
    }
}
