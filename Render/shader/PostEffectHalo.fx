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

texture g_HaloTex;
sampler HaloSampler = sampler_state
{
    Texture = <g_HaloTex>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

float  g_Threshold = 2.5f;
float  g_HaloIntensity = 0.35f;
float  g_HaloRadiusPixels = 200.0f;
float2 g_ScreenSize = float2(1600.0f, 900.0f);
float2 g_TexelSize = float2(1.0f / 1600.0f, 1.0f / 900.0f);

float4 BrightPassPS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float2 texel = g_TexelSize;
    float3 sum = 0.0f;
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f, -1.0f)).rgb;
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f, -1.0f)).rgb;
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  1.0f)).rgb;
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  1.0f)).rgb;
    const float3 color = sum * 0.25f;
    const float lum = dot(color, float3(0.299f, 0.587f, 0.114f));
    const float strength = saturate((lum - g_Threshold) / max(g_Threshold, 0.0001f));
    return float4(color * strength, 1.0f);
}

float4 Blur5x5PS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float2 texel = g_TexelSize;
    float3 sum = 0.0f;
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f, -2.0f)).rgb * (1.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f, -2.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f, -2.0f)).rgb * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f, -2.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f, -2.0f)).rgb * (1.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f, -1.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f, -1.0f)).rgb * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f, -1.0f)).rgb * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f, -1.0f)).rgb * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f, -1.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f,  0.0f)).rgb * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  0.0f)).rgb * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord).rgb * (36.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  0.0f)).rgb * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f,  0.0f)).rgb * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f,  1.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  1.0f)).rgb * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f,  1.0f)).rgb * (24.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  1.0f)).rgb * (16.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f,  1.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-2.0f,  2.0f)).rgb * (1.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2(-1.0f,  2.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 0.0f,  2.0f)).rgb * (6.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 1.0f,  2.0f)).rgb * (4.0f / 256.0f);
    sum += tex2D(SrcSampler, texCoord + texel * float2( 2.0f,  2.0f)).rgb * (1.0f / 256.0f);
    return float4(sum, 1.0f);
}

float3 HaloColor(float phase)
{
    const float r = saturate(1.0f - abs(phase * 6.0f - 3.0f) + 1.0f);
    const float g = saturate(1.0f - abs(phase * 6.0f - 2.0f) + 1.0f);
    const float b = saturate(1.0f - abs(phase * 6.0f - 4.0f) + 1.0f);
    return float3(r, g, b);
}

float3 SampleHaloRing(float2 texCoord, float2 direction, float phase)
{
    const float2 radiusUv = direction * (g_HaloRadiusPixels / g_ScreenSize);
    const float3 nearRing = tex2D(HaloSampler, texCoord + radiusUv * 0.92f).rgb;
    const float3 midRing = tex2D(HaloSampler, texCoord + radiusUv).rgb;
    const float3 farRing = tex2D(HaloSampler, texCoord + radiusUv * 1.08f).rgb;
    const float lum = dot(nearRing + midRing * 1.5f + farRing, float3(0.299f, 0.587f, 0.114f)) * 0.285714f;
    return HaloColor(phase) * lum;
}

float4 CombinePS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float4 scene = tex2D(SceneSampler, texCoord);
    float3 halo = 0.0f;
    halo += SampleHaloRing(texCoord, float2( 1.000f,  0.000f), 0.00f);
    halo += SampleHaloRing(texCoord, float2( 0.924f,  0.383f), 0.06f);
    halo += SampleHaloRing(texCoord, float2( 0.707f,  0.707f), 0.12f);
    halo += SampleHaloRing(texCoord, float2( 0.383f,  0.924f), 0.18f);
    halo += SampleHaloRing(texCoord, float2( 0.000f,  1.000f), 0.24f);
    halo += SampleHaloRing(texCoord, float2(-0.383f,  0.924f), 0.30f);
    halo += SampleHaloRing(texCoord, float2(-0.707f,  0.707f), 0.36f);
    halo += SampleHaloRing(texCoord, float2(-0.924f,  0.383f), 0.42f);
    halo += SampleHaloRing(texCoord, float2(-1.000f,  0.000f), 0.48f);
    halo += SampleHaloRing(texCoord, float2(-0.924f, -0.383f), 0.54f);
    halo += SampleHaloRing(texCoord, float2(-0.707f, -0.707f), 0.60f);
    halo += SampleHaloRing(texCoord, float2(-0.383f, -0.924f), 0.66f);
    halo += SampleHaloRing(texCoord, float2( 0.000f, -1.000f), 0.72f);
    halo += SampleHaloRing(texCoord, float2( 0.383f, -0.924f), 0.78f);
    halo += SampleHaloRing(texCoord, float2( 0.707f, -0.707f), 0.84f);
    halo += SampleHaloRing(texCoord, float2( 0.924f, -0.383f), 0.90f);

    halo *= g_HaloIntensity * (1.0f / 16.0f);
    return float4(scene.rgb + halo, 1.0f);
}

technique BrightPass
{
    pass P0
    {
        PixelShader = compile ps_3_0 BrightPassPS();
    }
}

technique Blur5x5
{
    pass P0
    {
        PixelShader = compile ps_3_0 Blur5x5PS();
    }
}

technique Combine
{
    pass P0
    {
        PixelShader = compile ps_3_0 CombinePS();
    }
}
