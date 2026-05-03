float4x4 g_matInvCurrentViewProj;
float4x4 g_matPrevViewProj;
float g_fBlurScale = 0.3f;
float g_fMaxBlurPixels = 24.0f;
int g_iSampleCount = 21;
int g_iMotionBlurEnabled = 1;
int g_iDebugGridEnabled = 0;
float4 g_vTexelSize = { 1.0f / 1600.0f, 1.0f / 900.0f, 1600.0f, 900.0f };
float g_fNear = 0.1f;
float g_fFar = 30000.0f;

texture texture1;
sampler colorSampler = sampler_state
{
    Texture = (texture1);
    MinFilter = NONE;
    MagFilter = NONE;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture depthTexture;
sampler depthSampler = sampler_state
{
    Texture = (depthTexture);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

void VertexShader1(in  float4 inPosition  : POSITION,
                   in  float2 inTexCoord  : TEXCOORD0,
                   out float4 outPosition : POSITION,
                   out float2 outTexCoord : TEXCOORD0)
{
    outPosition = inPosition;
    outTexCoord = inTexCoord;
}

float LinearDepthToProjectionDepth(float linearDepth)
{
    float viewZ = lerp(g_fNear, g_fFar, saturate(linearDepth));
    float a = g_fFar / (g_fFar - g_fNear);
    float b = -g_fNear * g_fFar / (g_fFar - g_fNear);
    return saturate(a + b / max(viewZ, 0.0001f));
}

float2 GetPrevUv(float2 uv, float linearDepth)
{
    float4 currentClip;
    currentClip.x = uv.x * 2.0f - 1.0f;
    currentClip.y = (1.0f - uv.y) * 2.0f - 1.0f;
    currentClip.z = LinearDepthToProjectionDepth(linearDepth);
    currentClip.w = 1.0f;

    float4 worldPos = mul(currentClip, g_matInvCurrentViewProj);
    worldPos /= max(abs(worldPos.w), 0.0001f);

    float4 prevClip = mul(worldPos, g_matPrevViewProj);
    prevClip /= max(abs(prevClip.w), 0.0001f);

    float2 prevUv;
    prevUv.x = prevClip.x * 0.5f + 0.5f;
    prevUv.y = (1.0f - prevClip.y) * 0.5f;
    return prevUv;
}

float2 GetVelocity(float2 uv, float linearDepth)
{
    float2 prevUv = GetPrevUv(uv, linearDepth);
    float2 velocity = (uv - prevUv) * g_fBlurScale;

    float2 velocityPixels;
    velocityPixels.x = velocity.x / g_vTexelSize.x;
    velocityPixels.y = velocity.y / g_vTexelSize.y;

    float velocityPixelLength = length(velocityPixels);
    if (velocityPixelLength > g_fMaxBlurPixels)
    {
        velocityPixels *= g_fMaxBlurPixels / velocityPixelLength;
        velocity.x = velocityPixels.x * g_vTexelSize.x;
        velocity.y = velocityPixels.y * g_vTexelSize.y;
    }

    return velocity;
}

float4 SampleMotionBlur(float2 uv, float2 velocity)
{
    float4 accumColor = 0.0f;
    int sampleCount = clamp(g_iSampleCount, 2, 21);

    [loop]
    for (int i = 0; i < 21; ++i)
    {
        float t = ((float)i / (float)(sampleCount - 1)) * 2.0f - 1.0f;
        float2 sampleUv = saturate(uv - velocity * t);
        float active = (i < sampleCount) ? 1.0f : 0.0f;
        accumColor += tex2D(colorSampler, sampleUv) * active;
    }

    return accumColor / (float)sampleCount;
}

void PixelShader1(in float2 inTexCoord : TEXCOORD0,
                  out float4 outColor  : COLOR0)
{
    float2 sampleUv = saturate(inTexCoord + g_vTexelSize.xy * 0.5f);
    float linearDepth = tex2D(depthSampler, sampleUv).a;
    float2 velocity = GetVelocity(sampleUv, linearDepth);
    float4 finalColor = (g_iMotionBlurEnabled == 0) ?
                        tex2D(colorSampler, sampleUv) :
                        SampleMotionBlur(sampleUv, velocity);

    if (g_iDebugGridEnabled != 0)
    {
        float2 pixelCoord = floor(sampleUv * g_vTexelSize.zw);
        float gridX = fmod(pixelCoord.x, 5.0f);
        float gridY = fmod(pixelCoord.y, 5.0f);
        if (gridX == 0.0f || gridY == 0.0f)
        {
            finalColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
    }

    outColor = finalColor;
}

technique Technique1
{
    pass Pass1
    {
        CullMode = NONE;
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
    }
}
