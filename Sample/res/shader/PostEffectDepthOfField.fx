float2 g_TexelSize = float2(1.0 / 1600.0, 1.0 / 900.0);
float4 g_cameraPos = float4(0.0, 0.0, 0.0, 1.0);
float g_focalDistanceMeters = 8.0;
float g_startNearMeters = 0.0;
float g_maxBlurDistanceMeters = 16.0;
float g_focusBandHalfWidthMeters = 2.0;
float g_blurRadiusPixels = 1.0;
float g_positionRange = 50.0;
float g_dofBlend = 1.0;

// 焦点範囲の外側から、何mごとに 3x3 -> 5x5 -> 7x7 -> 9x9 -> 11x11 と強くするか。
// C++ 側から渡さなくても、この初期値で動作します。
float g_blurStepMeters = 1.0;

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
    float2 positionUv = uv + g_TexelSize;
    float4 encoded = tex2Dlod(positionSampler, float4(positionUv, 0.0f, 0.0f));
    float3 worldPos = ((encoded.xyz * 2.0f) - 1.0f) * g_positionRange;
    return float4(worldPos, encoded.a);
}

float GetDistanceMeters(float2 uv, out float valid)
{
    uv -= g_TexelSize * 0.5f;
    uv -= g_TexelSize * 0.5f;

    float4 worldPos = DecodeWorldPosition(uv);
    valid = worldPos.w;
    return length(worldPos.xyz - g_cameraPos.xyz);
}

float GetDepthCompareThresholdMeters(float centerDistanceMeters)
{
    float relativeThreshold = centerDistanceMeters * 0.02f;
    float focusBandThreshold = g_focusBandHalfWidthMeters * 0.5f;
    return max(0.25f, min(relativeThreshold, focusBandThreshold));
}

float GetBlurStartDistanceMeters()
{
    return min(g_focalDistanceMeters, g_maxBlurDistanceMeters);
}

float GetBlurAmountNormalized(float distanceMeters)
{
    const float farBlurStartDistance = min(g_focalDistanceMeters, g_maxBlurDistanceMeters);
    const float farBlurMaxDistance = max(g_focalDistanceMeters, g_maxBlurDistanceMeters);
    const float farBlurRange = max(farBlurMaxDistance - farBlurStartDistance, 0.0001f);
    const float farBlurNormalized = saturate((distanceMeters - farBlurStartDistance) / farBlurRange);

    float nearBlurNormalized = 0.0f;
    if (g_startNearMeters > 0.0f && distanceMeters < g_startNearMeters)
    {
        nearBlurNormalized = saturate((g_startNearMeters - distanceMeters) / max(g_startNearMeters, 0.0001f));
    }

    return max(farBlurNormalized, nearBlurNormalized);
}

int GetBlurHalfSizeFromNormalized(float normalized)
{
    if (normalized < 0.20f)
    {
        return 1; // 3x3
    }

    if (normalized < 0.40f)
    {
        return 2; // 5x5
    }

    if (normalized < 0.60f)
    {
        return 3; // 7x7
    }

    if (normalized < 0.80f)
    {
        return 4; // 9x9
    }

    return 5; // 11x11
}

int GetBlurHalfSize(float distanceMeters)
{
    float normalized = GetBlurAmountNormalized(distanceMeters);
    if (normalized <= 0.0f)
    {
        return 0; // 1x1
    }

    return GetBlurHalfSizeFromNormalized(normalized);
}

bool IsDistanceBlurred(float distanceMeters)
{
    return GetBlurAmountNormalized(distanceMeters) > 0.0f;
}

// ps_3_0 では POSITION をピクセルシェーダー入力にしない。
// 画面位置は使わず、元の uv と sampleUv の扱いを維持する。
float4 PS(in float2 uv : TEXCOORD0) : COLOR0
{
    // 元のコードと同じ 0.5 texel 補正を残す。
    float2 sampleUv = uv;
    sampleUv += g_TexelSize * 0.5f;

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

    float4 sumColor = baseColor;
    float weightSum = 1.0f;
    float depthCompareThresholdMeters = GetDepthCompareThresholdMeters(centerDistanceMeters);

    [loop]
    for (int y = -blurHalfSize; y <= blurHalfSize; ++y)
    {
        [loop]
        for (int x = -blurHalfSize; x <= blurHalfSize; ++x)
        {
            if (x == 0 && y == 0)
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

            bool tapAlreadyBlurred = IsDistanceBlurred(tapDistanceMeters);
            if (!tapAlreadyBlurred &&
                abs(tapDistanceMeters - centerDistanceMeters) > depthCompareThresholdMeters)
            {
                continue;
            }

            sumColor += tex2Dlod(colorSampler, float4(tapUv, 0.0f, 0.0f));
            weightSum += 1.0f;
        }
    }

    if (weightSum <= 0.0f)
    {
        return baseColor;
    }

    const float4 blurredColor = sumColor / weightSum;
    return lerp(baseColor, blurredColor, saturate(g_dofBlend));
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
