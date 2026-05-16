float4x4 g_matWorldViewProj;
float4 g_lightNormal = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float g_fAmbientIntensity = 1.0f;
bool g_bDitherAlpha = true;
int g_swayMode = 0;
float g_time = 0.0f;

texture texture1;
sampler textureSampler = sampler_state {
    Texture = (texture1);
    AddressU = CLAMP;
    AddressV = CLAMP;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

void VertexShader1(in  float4 inPosition  : POSITION,
                   in  float4 inNormal    : NORMAL0,
                   in  float4 inTexCood   : TEXCOORD0,
                   in  float4 inInstancePosRot : TEXCOORD1,
                   in  float4 inInstanceScale  : TEXCOORD2,

                   out float4 outPosition : POSITION,
                   out float4 outDiffuse  : COLOR0,
                   out float4 outTexCood  : TEXCOORD0)
{
    float scale = inInstanceScale.x;
    float rotationY = inInstancePosRot.w;
    float sinY = sin(rotationY);
    float cosY = cos(rotationY);

    float3 scaledPos = inPosition.xyz * scale;
    float swayWeight = saturate(1.0f - inTexCood.y);
    swayWeight *= swayWeight;
    if (g_swayMode == 1)
    {
        float phase = g_time * 1.7f + inInstancePosRot.x * 0.27f + inInstancePosRot.z * 0.19f;
        float sway = sin(phase) * 0.16f + sin(phase * 1.83f + 1.2f) * 0.06f;
        scaledPos.x += sway * swayWeight * scale;
    }

    float3 rotatedPos;
    rotatedPos.x = (scaledPos.x * cosY) + (scaledPos.z * sinY);
    rotatedPos.y = scaledPos.y;
    rotatedPos.z = (-scaledPos.x * sinY) + (scaledPos.z * cosY);
    if (g_swayMode == 2)
    {
        float2 waveDir = normalize(float2(0.82f, 0.57f));
        float waveCoord = dot(inInstancePosRot.xz, waveDir);
        float phase = g_time * 2.2f - waveCoord * 0.42f;
        float broadWave = sin(phase) * 0.22f;
        float detailWave = sin(phase * 1.7f + inInstancePosRot.x * 0.07f) * 0.07f;
        float wave = (broadWave + detailWave) * swayWeight * scale;
        rotatedPos.x += waveDir.x * wave;
        rotatedPos.z += waveDir.y * wave;
    }

    float3 worldPos = rotatedPos + inInstancePosRot.xyz;
    outPosition = mul(float4(worldPos, 1.0f), g_matWorldViewProj);

    float3 rotatedNormal;
    rotatedNormal.x = (inNormal.x * cosY) + (inNormal.z * sinY);
    rotatedNormal.y = inNormal.y;
    rotatedNormal.z = (-inNormal.x * sinY) + (inNormal.z * cosY);

    float lightIntensity = 1.0f;

    if (false)
    {
        lightIntensity = max(0, dot(float4(rotatedNormal, 0.0f), g_lightNormal));
    }
    outDiffuse.rgb = saturate(g_ambient.rgb * g_fAmbientIntensity + lightIntensity);
    outDiffuse.a = 1.0f;

    outTexCood = inTexCood;
}

float Bayer4x4Threshold(float2 screenPos)
{
    float2 p = fmod(floor(screenPos), 4.0f);
    float threshold = 0.0f;

    if (p.y < 1.0f)
    {
        if (p.x < 1.0f) threshold = 0.0f;
        else if (p.x < 2.0f) threshold = 8.0f;
        else if (p.x < 3.0f) threshold = 2.0f;
        else threshold = 10.0f;
    }
    else if (p.y < 2.0f)
    {
        if (p.x < 1.0f) threshold = 12.0f;
        else if (p.x < 2.0f) threshold = 4.0f;
        else if (p.x < 3.0f) threshold = 14.0f;
        else threshold = 6.0f;
    }
    else if (p.y < 3.0f)
    {
        if (p.x < 1.0f) threshold = 3.0f;
        else if (p.x < 2.0f) threshold = 11.0f;
        else if (p.x < 3.0f) threshold = 1.0f;
        else threshold = 9.0f;
    }
    else
    {
        if (p.x < 1.0f) threshold = 15.0f;
        else if (p.x < 2.0f) threshold = 7.0f;
        else if (p.x < 3.0f) threshold = 13.0f;
        else threshold = 5.0f;
    }

    return (threshold + 0.5f) / 16.0f;
}

void PixelShader1(in float2 inScreenPos   : VPOS,
                  in float4 inScreenColor : COLOR0,
                  in float2 inTexCood     : TEXCOORD0,

                  out float4 outColor     : COLOR)
{
    float4 workColor = (float4)0;
    workColor = tex2D(textureSampler, inTexCood);
    if (g_bDitherAlpha)
    {
        clip(workColor.a - Bayer4x4Threshold(inScreenPos));
    }
    else
    {
        clip(workColor.a - 0.1f);
    }
    outColor.rgb = inScreenColor.rgb * workColor.rgb;
    outColor.a = 1.0f;
}

technique Technique1
{
   pass Pass1
   {
      AlphaBlendEnable = FALSE;
      AlphaTestEnable = FALSE;
      CullMode = NONE;
      VertexShader = compile vs_3_0 VertexShader1();
      PixelShader = compile ps_3_0 PixelShader1();
   }
}
