float2 g_TexelSize = float2(1.0 / 1600.0, 1.0 / 900.0);
float4 g_cameraPos = float4(0.0, 0.0, 0.0, 1.0);
float g_focalDistanceMeters = 8.0;
float g_focusBandHalfWidthMeters = 2.0;
float g_blurRadiusPixels = 1.0;
float g_positionRange = 50.0;

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

#define DOF_SAMPLE_SIZE 11

float4 PS(in float4 pos : POSITION, in float2 uv : TEXCOORD0) : COLOR0
{
    float2 sampleUv = uv + g_TexelSize * 0.5f;
    float4 baseColor = tex2D(colorSampler, sampleUv);

    float centerValid = 0.0f;
    float centerDistanceMeters = GetDistanceMeters(sampleUv, centerValid);
    if (centerValid <= 0.0f)
    {
        return baseColor;
    }

    if (abs(centerDistanceMeters - g_focalDistanceMeters) <= g_focusBandHalfWidthMeters)
    {
        return baseColor;
    }

    float4 sumColor = baseColor;
    float weightSum = 1.0f;

    const int sampleHalf = DOF_SAMPLE_SIZE / 2;
    [unroll]
    for (int y = -sampleHalf; y <= sampleHalf; ++y)
    {
        [unroll]
        for (int x = -sampleHalf; x <= sampleHalf; ++x)
        {
            if (x == 0 && y == 0)
            {
                continue;
            }

            float2 offset = float2((float)x, (float)y) * g_TexelSize * g_blurRadiusPixels;
            float tapValid = 0.0f;
            float tapDistanceMeters = GetDistanceMeters(sampleUv + offset, tapValid);
            if (tapValid <= 0.0f)
            {
                continue;
            }

            if (abs(tapDistanceMeters - g_focalDistanceMeters) <= g_focusBandHalfWidthMeters)
            {
                continue;
            }

            sumColor += tex2D(colorSampler, sampleUv + offset);
            weightSum += 1.0f;
        }
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
