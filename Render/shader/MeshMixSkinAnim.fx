float4 g_lightDir = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_cameraPos = { 10.0f, 5.0f, 10.0f, 1.0f };

float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float g_fAmbientIntensity = 1.0f;
float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };

float g_fSunLightIntensity = 1.0f;
bool g_fresnelEnable = true;
float g_fresnelIntensity = 0.08f;
bool g_bSaturateShadow = false;
float g_fSaturateShadowIntensity = 0.2f;
float g_fShadowDarkness = 1.0f;
float g_specularPower = 1.0f;
float g_specularIntensity = 0.1f;
bool g_treatTextureAsWhite = false;
bool g_alphaClipEnabled = false;
bool g_mirrorClipEnable = false;
float4 g_mirrorClipPlane = { 0.0f, 1.0f, 0.0f, 0.0f };

float3 g_pointLightPos[16];
float  g_pointLightBrightness[16];
float  g_pointLightShape[16];
float  g_pointLightLineLength[16];
float  g_pointLightSquareWidth[16];
float  g_pointLightSquareHeight[16];
float4 g_pointLightRotation[16];
float3 g_pointLightColor[16];

static const float POINT_LIGHT_CUBE_HALF_SIZE = 4.0f;
static const float POINT_LIGHT_SPHERE_RADIUS = 5.0f;

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

float3 SampleBaseTextureColor(float2 uv)
{
    float3 textureColor = tex2D(g_textureSampler, uv).rgb;
    if (g_treatTextureAsWhite)
    {
        textureColor = 1.0f.xxx;
    }

    return textureColor;
}

void ApplyAlphaClip(float2 uv)
{
    if (g_alphaClipEnabled)
    {
        clip(tex2D(g_textureSampler, uv).a - 0.5f);
    }
}

float3 RotateVectorXYZ(float3 inputVector, float3 rotation)
{
    float sinX = sin(rotation.x);
    float cosX = cos(rotation.x);
    float sinY = sin(rotation.y);
    float cosY = cos(rotation.y);
    float sinZ = sin(rotation.z);
    float cosZ = cos(rotation.z);

    float3 rotated = inputVector;

    rotated = float3(rotated.x,
                     rotated.y * cosX - rotated.z * sinX,
                     rotated.y * sinX + rotated.z * cosX);

    rotated = float3(rotated.x * cosY + rotated.z * sinY,
                     rotated.y,
                     -rotated.x * sinY + rotated.z * cosY);

    rotated = float3(rotated.x * cosZ - rotated.y * sinZ,
                     rotated.x * sinZ + rotated.y * cosZ,
                     rotated.z);

    return rotated;
}

float3 ClosestPointOnPointLightShape(float3 lightPos,
                                     float lightShape,
                                     float lightLineLength,
                                     float lightSquareWidth,
                                     float lightSquareHeight,
                                     float3 lightRotation,
                                     float3 worldPos)
{
    float3 delta = worldPos - lightPos;

    if (lightShape < 0.5f)
    {
        return lightPos;
    }

    if (lightShape < 1.5f)
    {
        float halfLength = max(lightLineLength * 0.5f, 0.0f);
        float3 lineAxis = normalize(RotateVectorXYZ(float3(1.0f, 0.0f, 0.0f), lightRotation));
        float projected = clamp(dot(delta, lineAxis), -halfLength, halfLength);
        return lightPos + (lineAxis * projected);
    }

    if (lightShape < 2.5f)
    {
        float halfWidth = max(lightSquareWidth * 0.5f, 0.0f);
        float halfHeight = max(lightSquareHeight * 0.5f, 0.0f);
        float3 squareAxisX = normalize(RotateVectorXYZ(float3(1.0f, 0.0f, 0.0f), lightRotation));
        float3 squareAxisZ = normalize(RotateVectorXYZ(float3(0.0f, 0.0f, 1.0f), lightRotation));
        float projectedX = clamp(dot(delta, squareAxisX), -halfWidth, halfWidth);
        float projectedZ = clamp(dot(delta, squareAxisZ), -halfHeight, halfHeight);
        return lightPos + (squareAxisX * projectedX) + (squareAxisZ * projectedZ);
    }

    if (lightShape < 3.5f)
    {
        return lightPos + clamp(delta,
                                float3(-POINT_LIGHT_CUBE_HALF_SIZE, -POINT_LIGHT_CUBE_HALF_SIZE, -POINT_LIGHT_CUBE_HALF_SIZE),
                                float3( POINT_LIGHT_CUBE_HALF_SIZE,  POINT_LIGHT_CUBE_HALF_SIZE,  POINT_LIGHT_CUBE_HALF_SIZE));
    }

    float distanceToCenter = length(delta);
    if (distanceToCenter <= POINT_LIGHT_SPHERE_RADIUS || distanceToCenter <= 1e-6f)
    {
        return worldPos;
    }

    return lightPos + (delta / distanceToCenter) * POINT_LIGHT_SPHERE_RADIUS;
}

void AccumulateSingleLightSample(float3 samplePos,
                                 float sampleBrightness,
                                 float3 lightColor,
                                 float3 worldPos,
                                 float3 normal,
                                 float3 cameraDirWS,
                                 out float3 diffuseContribution,
                                 out float3 specularContribution)
{
    float3 Lvec = samplePos - worldPos;
    float dist = length(Lvec);
    float3 L = Lvec / max(dist, 1e-6);

    float NdotL = saturate(dot(normal, L));
    float3 H = normalize(L + cameraDirWS);
    float NdotH = saturate(dot(normal, H));
    float atten = saturate(1.0 / max(dist, 1e-6));

    float diff = sampleBrightness * atten * NdotL;
    float spec = 0.0f;
    if (g_specularIntensity > 0.0f)
    {
        spec = pow(NdotH, g_specularPower) * g_specularIntensity * sampleBrightness * atten;
    }

    diffuseContribution = lightColor * diff;
    specularContribution = lightColor * spec;
}

float CalcFresnelFactor(float3 normal, float3 cameraDir)
{
    float viewDot = saturate(dot(normalize(normal), normalize(cameraDir)));
    return pow(1.0f - viewDot, 5.0f);
}

void ApplyMirrorClip(float3 worldPos)
{
    if (g_mirrorClipEnable)
    {
        clip(dot(float4(worldPos, 1.0f), g_mirrorClipPlane));
    }
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
    float3 normal = 0.0f;
    float lastWeight = 0.0f;

    int4 indexVector = (int4)inBlendIndices;

    float blendWeightsArray[4] = (float[4])inBlendWeights;
    int indexArray[4] = (int[4])indexVector;

    [unroll] for (int i = 0; i < boneNumber - 1; ++i)
    {
        lastWeight += blendWeightsArray[i];

        position += mul(inPosition, g_matWorldArray[indexArray[i]]) * blendWeightsArray[i];
        normal += mul(inNormal.xyz, (float3x3)g_matWorldArray[indexArray[i]]) * blendWeightsArray[i];
    }

    lastWeight = 1.0f - lastWeight;

    position += mul(inPosition, g_matWorldArray[indexArray[boneNumber - 1]]) * lastWeight;
    normal += mul(inNormal.xyz, (float3x3)g_matWorldArray[indexArray[boneNumber - 1]]) * lastWeight;
    normal = normalize(normal);

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
    ApplyMirrorClip(inPosWorld);
    ApplyAlphaClip(inTexCoord);

    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);

    float4 textureColor = tex2D(g_textureSampler, inTexCoord);
    float3 albedo = SampleBaseTextureColor(inTexCoord) * g_diffuse.rgb;

    float NdotL = saturate(dot(normal, lightDir));
    float NdotH = saturate(dot(normal, halfVector));

    float shadowAmount = saturate(1.0f - NdotL);
    float3 shadowAlbedo = albedo;
    if (g_bSaturateShadow)
    {
        float saturationAmount = 1.0f + (shadowAmount * g_fSaturateShadowIntensity);
        shadowAlbedo = IncreaseSaturation(albedo, saturationAmount);
    }

    float3 ambient = g_ambient.rgb * g_fAmbientIntensity * albedo;
    float3 lambert = shadowAlbedo
                   * (1.0f - ((1.0f - NdotL) * g_fShadowDarkness))
                   * g_lightColor.rgb
                   * g_fSunLightIntensity;
    float3 specular = pow(NdotH, g_specularPower)
                    * g_specularIntensity
                    * g_specularColor.rgb
                    * g_lightColor.rgb;
    float fresnel = g_fresnelEnable ? (CalcFresnelFactor(normal, cameraDir) * g_fresnelIntensity) : 0.0f;
    float3 fresnelColor = g_specularColor.rgb * fresnel;

    outColor = saturate(float4(ambient + lambert + specular + fresnelColor,
                               textureColor.a * g_diffuse.a));
}

void PixelShaderPointLight(in  float4 inPosition    : POSITION,
                           in  float3 inPosWorld    : TEXCOORD0,
                           in  float3 inNormalWorld : TEXCOORD1,
                           in  float2 inTexCoord    : TEXCOORD2,
                           out float4 outColor      : COLOR)
{
    ApplyMirrorClip(inPosWorld);
    ApplyAlphaClip(inTexCoord);

    float3 N = normalize(inNormalWorld);
    float3 cameraDirWS = normalize(g_cameraPos.xyz - inPosWorld);
    float3 albedo = SampleBaseTextureColor(inTexCoord) * g_diffuse.rgb;

    float3 diffuseAccum = 0.0f;
    float3 specularAccum = 0.0f;

    for (int i = 0; i < 16; ++i)
    {
        if (g_pointLightBrightness[i] <= 0.0f)
        {
            continue;
        }

        float3 lightSurfacePos = ClosestPointOnPointLightShape(g_pointLightPos[i],
                                                               g_pointLightShape[i],
                                                               g_pointLightLineLength[i],
                                                               g_pointLightSquareWidth[i],
                                                               g_pointLightSquareHeight[i],
                                                               g_pointLightRotation[i].xyz,
                                                               inPosWorld);
        float3 sampleDiffuse = 0.0f;
        float3 sampleSpecular = 0.0f;
        AccumulateSingleLightSample(lightSurfacePos,
                                    g_pointLightBrightness[i],
                                    g_pointLightColor[i],
                                    inPosWorld,
                                    N,
                                    cameraDirWS,
                                    sampleDiffuse,
                                    sampleSpecular);
        diffuseAccum += sampleDiffuse;
        specularAccum += sampleSpecular;
    }

    outColor = float4((albedo * diffuseAccum) + specularAccum, 0.0f);
}

void PixelShaderAlphaDepthPrePass(in  float2 inTexCoord : TEXCOORD2,
                                  out float4 outColor   : COLOR)
{
    clip(tex2D(g_textureSampler, inTexCoord).a - 0.5f);
    outColor = 0.0f;
}

technique Technique1
{
    pass Pass0
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ColorWriteEnable = 15;
        CullMode = NONE;

        VertexShader = (vsArray[g_currentBoneIndex]);
        PixelShader = compile ps_3_0 PixelShader1();
    }

    pass PassPointLight
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = ONE;
        DestBlend = ONE;
        ColorWriteEnable = 15;
        CullMode = NONE;

        VertexShader = (vsArray[g_currentBoneIndex]);
        PixelShader = compile ps_3_0 PixelShaderPointLight();
    }
}

technique TechniqueAlphaDepthPrePass
{
    pass Pass0
    {
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = FALSE;
        AlphaTestEnable = FALSE;
        ColorWriteEnable = 0;
        CullMode = NONE;

        VertexShader = (vsArray[g_currentBoneIndex]);
        PixelShader = compile ps_3_0 PixelShaderAlphaDepthPrePass();
    }
}

technique TechniqueAlphaClip
{
    pass Pass0
    {
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = FALSE;
        AlphaTestEnable = FALSE;
        ColorWriteEnable = 15;
        CullMode = NONE;

        VertexShader = (vsArray[g_currentBoneIndex]);
        PixelShader = compile ps_3_0 PixelShader1();
    }

    pass PassPointLight
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = ONE;
        DestBlend = ONE;
        ColorWriteEnable = 15;
        CullMode = NONE;

        VertexShader = (vsArray[g_currentBoneIndex]);
        PixelShader = compile ps_3_0 PixelShaderPointLight();
    }
}
