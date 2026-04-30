float4 g_lightDir = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_cameraPos = { 10.0f, 5.0f, 10.0f, 1.0f };

float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };

float g_fSunLightIntensity = 1.0f;
bool g_bSaturateShadow = false;
float g_fSaturateShadowIntensity = 0.2f;
float g_fShadowDarkness = 1.0f;
float g_specularPower = 1.0f;
float g_specularIntensity = 0.1f;

static const int MAX_MATRICES = 8;
float4x3 g_matWorldArray[MAX_MATRICES];
float4x4 g_matViewProj;

texture g_texture;
sampler g_textureSampler = sampler_state
{
    Texture = (g_texture);
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MaxAnisotropy = 8;

    AddressU = Wrap;
    AddressV = Wrap;

    MaxMipLevel = 1;
};

float3 IncreaseSaturation(float3 color, float amount)
{
    float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
    return saturate(lerp(luminance.xxx, color, amount));
}

void VertexShader1(in  float4 inPosition     : POSITION,
                   in  float4 inBlendWeights : BLENDWEIGHT,
                   in  float4 inBlendIndices : BLENDINDICES,
                   in  float4 inNormal       : NORMAL,
                   in  float3 inTexCoord     : TEXCOORD0,

                   out float4 outPosition    : POSITION,
                   out float3 outPosWorld    : TEXCOORD0,
                   out float3 outNormalWorld : TEXCOORD1,
                   out float2 outTexCoord    : TEXCOORD2,
                   uniform int boneNumber);

int g_currentBoneIndex;
VertexShader vsArray[4] =
{
    compile vs_3_0 VertexShader1(1),
    compile vs_3_0 VertexShader1(2),
    compile vs_3_0 VertexShader1(3),
    compile vs_3_0 VertexShader1(4)
};

void VertexShader1(in  float4 inPosition     : POSITION,
                   in  float4 inBlendWeights : BLENDWEIGHT,
                   in  float4 inBlendIndices : BLENDINDICES,
                   in  float4 inNormal       : NORMAL,
                   in  float3 inTexCoord     : TEXCOORD0,

                   out float4 outPosition    : POSITION,
                   out float3 outPosWorld    : TEXCOORD0,
                   out float3 outNormalWorld : TEXCOORD1,
                   out float2 outTexCoord    : TEXCOORD2,
                   uniform int boneNumber)
{
    float3 position = 0.0f;
    float3 normalPosition = 0.0f;
    float lastWeight = 0.0f;

    int4 indexVector = (int4)inBlendIndices;

    float blendWeightsArray[4] = (float[4])inBlendWeights;
    int indexArray[4] = (int[4])indexVector;

    [unroll] for (int i = 0; i < boneNumber - 1; ++i)
    {
        lastWeight += blendWeightsArray[i];

        position += mul(inPosition, g_matWorldArray[indexArray[i]]) * blendWeightsArray[i];
        normalPosition += mul(inNormal, g_matWorldArray[indexArray[i]]) * blendWeightsArray[i];
    }

    lastWeight = 1.0f - lastWeight;

    position += mul(inPosition, g_matWorldArray[indexArray[boneNumber - 1]]) * lastWeight;
    normalPosition += mul(inNormal, g_matWorldArray[indexArray[boneNumber - 1]]) * lastWeight;

    float3 normal = normalize(normalPosition - position);

    outPosition = mul(float4(position.xyz, 1.0f), g_matViewProj);
    outPosWorld = position.xyz;
    outNormalWorld = normal.xyz;
    outTexCoord = inTexCoord.xy;
}

void PixelShader1(in  float3 inPosWorld    : TEXCOORD0,
                  in  float3 inNormalWorld : TEXCOORD1,
                  in  float2 inTexCoord    : TEXCOORD2,
                  out float4 outColor      : COLOR)
{
    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);

    float4 textureColor = tex2D(g_textureSampler, inTexCoord);
    float3 albedo = textureColor.rgb * g_diffuse.rgb;

    float NdotL = saturate(dot(normal, lightDir));
    float NdotH = saturate(dot(normal, halfVector));

    float shadowAmount = saturate(1.0f - NdotL);
    float3 shadowAlbedo = albedo;
    if (g_bSaturateShadow)
    {
        float saturationAmount = 1.0f + (shadowAmount * g_fSaturateShadowIntensity);
        shadowAlbedo = IncreaseSaturation(albedo, saturationAmount);
    }

    float3 ambient = g_ambient.rgb * albedo;
    float3 lambert = shadowAlbedo
                   * (1.0f - ((1.0f - NdotL) * g_fShadowDarkness))
                   * g_fSunLightIntensity;
    float3 specular = pow(NdotH, g_specularPower)
                    * g_specularIntensity
                    * g_specularColor.rgb;

    outColor = saturate(float4(ambient + lambert + specular,
                               textureColor.a * g_diffuse.a));
}

technique Technique1
{
    pass Pass0
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = (vsArray[g_currentBoneIndex]);
        PixelShader = compile ps_3_0 PixelShader1();
    }
}
