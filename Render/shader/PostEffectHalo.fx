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

float  g_Threshold = 1.5f;
float  g_HaloIntensity = 1.0f;
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
    return float4(strength, strength, strength, 1.0f);
}

float SampleCircle(float2 texCoord, float radius)
{
    float sum = 0.0f;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 1.000f,  0.000f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.995f,  0.098f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.981f,  0.195f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.957f,  0.290f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.924f,  0.383f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.882f,  0.471f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.831f,  0.556f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.773f,  0.634f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.707f,  0.707f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.634f,  0.773f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.556f,  0.831f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.471f,  0.882f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.383f,  0.924f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.290f,  0.957f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.195f,  0.981f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.098f,  0.995f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.000f,  1.000f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.098f,  0.995f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.195f,  0.981f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.290f,  0.957f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.383f,  0.924f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.471f,  0.882f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.556f,  0.831f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.634f,  0.773f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.707f,  0.707f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.773f,  0.634f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.831f,  0.556f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.882f,  0.471f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.924f,  0.383f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.957f,  0.290f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.981f,  0.195f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.995f,  0.098f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-1.000f,  0.000f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.995f, -0.098f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.981f, -0.195f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.957f, -0.290f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.924f, -0.383f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.882f, -0.471f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.831f, -0.556f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.773f, -0.634f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.707f, -0.707f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.634f, -0.773f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.556f, -0.831f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.471f, -0.882f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.383f, -0.924f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.290f, -0.957f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.195f, -0.981f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2(-0.098f, -0.995f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.000f, -1.000f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.098f, -0.995f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.195f, -0.981f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.290f, -0.957f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.383f, -0.924f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.471f, -0.882f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.556f, -0.831f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.634f, -0.773f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.707f, -0.707f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.773f, -0.634f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.831f, -0.556f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.882f, -0.471f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.924f, -0.383f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.957f, -0.290f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.981f, -0.195f) * radius).r;
    sum += tex2D(SrcSampler, texCoord + g_TexelSize * float2( 0.995f, -0.098f) * radius).r;
    return sum * (1.0f / 64.0f);
}

float4 HaloPassPS(float2 texCoord : TEXCOORD0) : COLOR
{
    return float4(SampleCircle(texCoord, 20.0f),
                  SampleCircle(texCoord, 24.0f),
                  SampleCircle(texCoord, 28.0f),
                  1.0f);
}

float4 CombinePS(float2 texCoord : TEXCOORD0) : COLOR
{
    const float4 scene = tex2D(SceneSampler, texCoord);
    const float3 halo = tex2D(HaloSampler, texCoord).rgb * g_HaloIntensity;
    return float4(scene.rgb + halo, 1.0f);
}

technique BrightPass
{
    pass P0
    {
        PixelShader = compile ps_3_0 BrightPassPS();
    }
}

technique Combine
{
    pass P0
    {
        PixelShader = compile ps_3_0 CombinePS();
    }
}

technique HaloPass
{
    pass P0
    {
        PixelShader = compile ps_3_0 HaloPassPS();
    }
}
