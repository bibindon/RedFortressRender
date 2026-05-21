float4x4 g_matWorld;
float4x4 g_matViewProj;
float4x4 g_matWorldViewProj;
float4x4 g_matMirrorViewProj;

float4 g_lightDir = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_cameraPos = { 10.0f, 5.0f, 10.0f, 1.0f };
float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float g_fSunLightIntensity = 1.0f;
float g_fAmbientIntensity = 1.0f;

float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_pbrBaseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
float g_pbrRoughness = 0.85f;
float g_pbrMetallic = 0.0f;
bool g_enableSrgbToLinear = true;
bool g_enableLinearToSrgb = true;
float g_envReflectionIntensity = 0.05f;
float g_envMaxMipLevel = 5.0f;
float g_envDiffuseIntensity = 0.8f;
float g_envDiffuseMipLevel = 3.0f;
float g_specularIntensity = 0.0f;
float g_cubeMappingRate = 1.0f;
float g_cubeMappingGauss = 0.0f;
bool g_bPOM = false;
bool g_bNormalMapping = false;
bool g_bSSS = false;
float g_sssIntensity = 1.0f;
float4 g_sssColor = { 0.5f, 1.0f, 0.5f, 1.0f };
float g_time = 0.0f;
bool g_swayEnable = false;
float g_swayAmount = 0.0f;
float g_swaySpeed = 0.0f;

bool g_hasDiffuseTexture = false;
bool g_hasNormalTexture = false;
bool g_hasEnvTexture = false;

float g_emitIntensity = 1.0f;
float4 g_emitColor = { 1.0f, 1.0f, 1.0f, 1.0f };

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
static const float PI = 3.14159265f;

texture g_texture;
sampler2D g_textureSampler = sampler_state
{
    Texture = (g_texture);
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MaxAnisotropy = 8;
    AddressU = WRAP;
    AddressV = WRAP;
};

texture g_texNormalMap;
sampler2D g_normalMapSampler = sampler_state
{
    Texture = (g_texNormalMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

textureCUBE g_texCubeMap;
samplerCUBE g_cubeMapSampler = sampler_state
{
    Texture = <g_texCubeMap>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_texMirror;
sampler2D g_mirrorSampler = sampler_state
{
    Texture = (g_texMirror);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_texHeightMap;
texture g_texThickness;

struct VSIn
{
    float4 pos : POSITION;
    float4 nrm : NORMAL0;
    float4 tangent : TANGENT0;
    float4 binorm : BINORMAL0;
    float4 uv : TEXCOORD0;
};

struct VSOut
{
    float4 pos : POSITION;
    float3 posWorld : TEXCOORD0;
    float3 normWorld : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 tangentWorld : TEXCOORD3;
    float3 binormWorld : TEXCOORD4;
};

VSOut VertexShader1(VSIn i)
{
    VSOut o;
    o.pos = mul(i.pos, g_matWorldViewProj);
    o.posWorld = mul(i.pos, g_matWorld).xyz;

    float3x3 world3x3 = (float3x3)g_matWorld;
    o.normWorld = normalize(mul(i.nrm.xyz, world3x3));
    o.tangentWorld = normalize(mul(i.tangent.xyz, world3x3));
    o.binormWorld = normalize(mul(i.binorm.xyz, world3x3));
    o.uv = i.uv.xy;
    return o;
}

float3 SrgbToLinear(float3 c)
{
    return pow(saturate(c), 2.2f);
}

float3 LinearToSrgb(float3 c)
{
    return pow(saturate(c), 1.0f / 2.2f);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    float denom = NdotV * (1.0f - k) + k;
    return NdotV / max(denom, 0.0001f);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - saturate(cosTheta), 5.0f);
}

float4 SampleDiffuseTexture(float2 uv)
{
    if (!g_hasDiffuseTexture)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float4 textureColor = tex2D(g_textureSampler, uv);
    if (g_enableSrgbToLinear)
    {
        textureColor.rgb = SrgbToLinear(textureColor.rgb);
    }

    return textureColor;
}

float3 SampleNormalWorld(VSOut i)
{
    float3 normalWorld = normalize(i.normWorld);
    if (!g_hasNormalTexture || !g_bNormalMapping)
    {
        return normalWorld;
    }

    float3 normalTS = tex2D(g_normalMapSampler, i.uv).xyz * 2.0f - 1.0f;
    normalTS.x *= -1.0f;
    normalTS = normalize(normalTS);

    float3x3 tangentToWorld = float3x3(-normalize(i.tangentWorld),
                                       -normalize(i.binormWorld),
                                       normalWorld);
    return normalize(mul(normalTS, tangentToWorld));
}

float3 GetEnvSpecular(float3 R, float3 N, float3 V, float3 F0, float roughness, float metallic)
{
    if (!g_hasEnvTexture)
    {
        return 0.0f.xxx;
    }

    float mipLevel = clamp(saturate(roughness) * g_envMaxMipLevel, 0.0f, g_envMaxMipLevel);
    float3 envColor = texCUBElod(g_cubeMapSampler, float4(R, mipLevel)).rgb;
    if (g_enableSrgbToLinear)
    {
        envColor = SrgbToLinear(envColor);
    }

    float3 envF = FresnelSchlick(saturate(dot(N, V)), F0);
    float envSpecularStrength = lerp(0.1f, 1.0f, metallic);
    return envColor * envF * envSpecularStrength * g_envReflectionIntensity;
}

float3 GetEnvDiffuse(float3 N, float3 albedo, float3 kD)
{
    if (!g_hasEnvTexture)
    {
        return g_ambient.rgb * g_fAmbientIntensity * albedo * kD;
    }

    float diffuseMipLevel = clamp(g_envDiffuseMipLevel, 0.0f, g_envMaxMipLevel);
    float3 envDiffuseColor = texCUBElod(g_cubeMapSampler, float4(N, diffuseMipLevel)).rgb;
    if (g_enableSrgbToLinear)
    {
        envDiffuseColor = SrgbToLinear(envDiffuseColor);
    }

    return envDiffuseColor * albedo * kD * g_envDiffuseIntensity;
}

float3 EvaluatePbrLight(float3 N,
                        float3 V,
                        float3 L,
                        float3 radiance,
                        float3 albedo,
                        float roughness,
                        float metallic,
                        float3 F0)
{
    float3 H = normalize(L + V);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

    float3 numerator = D * G * F;
    float denominator = 4.0f * max(NdotV, 0.0001f) * max(NdotL, 0.0001f);
    float3 specularBRDF = numerator / max(denominator, 0.0001f);

    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    float3 diffuseBRDF = kD * albedo * (1.0f / PI);

    return (diffuseBRDF + specularBRDF) * radiance * NdotL;
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

float4 PixelShaderBase(VSOut i) : COLOR0
{
    float4 diffuseSample = SampleDiffuseTexture(i.uv);
    float3 albedo = diffuseSample.rgb * g_diffuse.rgb * g_pbrBaseColorFactor.rgb;
    float alpha = diffuseSample.a * g_diffuse.a * g_pbrBaseColorFactor.a;

    float roughness = clamp(g_pbrRoughness, 0.04f, 1.0f);
    float metallic = saturate(g_pbrMetallic);

    float3 N = SampleNormalWorld(i);
    float3 V = normalize(g_cameraPos.xyz - i.posWorld);
    float3 L = normalize(g_lightDir.xyz);
    float3 R = reflect(-V, N);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 directColor = EvaluatePbrLight(N,
                                          V,
                                          L,
                                          g_lightColor.rgb * g_fSunLightIntensity,
                                          albedo,
                                          roughness,
                                          metallic,
                                          F0);

    for (int lightIndex = 0; lightIndex < 16; ++lightIndex)
    {
        if (g_pointLightBrightness[lightIndex] <= 0.0f)
        {
            continue;
        }

        float3 lightSurfacePos = ClosestPointOnPointLightShape(g_pointLightPos[lightIndex],
                                                               g_pointLightShape[lightIndex],
                                                               g_pointLightLineLength[lightIndex],
                                                               g_pointLightSquareWidth[lightIndex],
                                                               g_pointLightSquareHeight[lightIndex],
                                                               g_pointLightRotation[lightIndex].xyz,
                                                               i.posWorld);
        float3 lightVector = lightSurfacePos - i.posWorld;
        float distanceToLight = length(lightVector);
        float attenuation = saturate(1.0f / max(distanceToLight, 1e-6f));
        float3 pointLightDir = lightVector / max(distanceToLight, 1e-6f);
        float3 pointRadiance = g_pointLightColor[lightIndex] * g_pointLightBrightness[lightIndex] * attenuation;
        directColor += EvaluatePbrLight(N,
                                        V,
                                        pointLightDir,
                                        pointRadiance,
                                        albedo,
                                        roughness,
                                        metallic,
                                        F0);
    }

    float3 kS = FresnelSchlick(saturate(dot(N, V)), F0);
    float3 kD = (1.0f - kS) * (1.0f - metallic);
    float3 color = directColor;
    color += GetEnvSpecular(R, N, V, F0, roughness, metallic);
    color += GetEnvDiffuse(N, albedo, kD);

    if (g_enableLinearToSrgb)
    {
        color = LinearToSrgb(color);
    }

    return float4(saturate(color), saturate(alpha));
}

float4 PixelShaderPointLight(VSOut i) : COLOR0
{
    float4 diffuseSample = SampleDiffuseTexture(i.uv);
    float3 albedo = diffuseSample.rgb * g_diffuse.rgb * g_pbrBaseColorFactor.rgb;
    float roughness = clamp(g_pbrRoughness, 0.04f, 1.0f);
    float metallic = saturate(g_pbrMetallic);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 N = SampleNormalWorld(i);
    float3 V = normalize(g_cameraPos.xyz - i.posWorld);

    float3 color = 0.0f.xxx;
    for (int lightIndex = 0; lightIndex < 16; ++lightIndex)
    {
        if (g_pointLightBrightness[lightIndex] <= 0.0f)
        {
            continue;
        }

        float3 lightSurfacePos = ClosestPointOnPointLightShape(g_pointLightPos[lightIndex],
                                                               g_pointLightShape[lightIndex],
                                                               g_pointLightLineLength[lightIndex],
                                                               g_pointLightSquareWidth[lightIndex],
                                                               g_pointLightSquareHeight[lightIndex],
                                                               g_pointLightRotation[lightIndex].xyz,
                                                               i.posWorld);
        float3 lightVector = lightSurfacePos - i.posWorld;
        float distanceToLight = length(lightVector);
        float attenuation = saturate(1.0f / max(distanceToLight, 1e-6f));
        float3 L = lightVector / max(distanceToLight, 1e-6f);
        float3 radiance = g_pointLightColor[lightIndex] * g_pointLightBrightness[lightIndex] * attenuation;
        color += EvaluatePbrLight(N, V, L, radiance, albedo, roughness, metallic, F0);
    }

    if (g_enableLinearToSrgb)
    {
        color = LinearToSrgb(color);
    }

    return float4(saturate(color), 0.0f);
}

float4 PixelShaderEmit(VSOut i) : COLOR0
{
    float4 diffuseSample = SampleDiffuseTexture(i.uv);
    float3 albedo = diffuseSample.rgb * g_diffuse.rgb * g_pbrBaseColorFactor.rgb;
    float3 color = albedo * g_emitColor.rgb * g_emitIntensity;
    if (g_enableLinearToSrgb)
    {
        color = LinearToSrgb(color);
    }
    return float4(saturate(color), saturate(diffuseSample.a * g_diffuse.a * g_pbrBaseColorFactor.a));
}

float4 PixelShaderMirror(VSOut i) : COLOR0
{
    float4 mirrorProj = mul(float4(i.posWorld, 1.0f), g_matMirrorViewProj);
    float2 uv;
    uv.x = mirrorProj.x / mirrorProj.w * 0.5f + 0.5f;
    uv.y = -mirrorProj.y / mirrorProj.w * 0.5f + 0.5f;
    return tex2D(g_mirrorSampler, uv) * 0.8f;
}

technique Technique1
{
    pass P0
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        CullMode = NONE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderBase();
    }

    pass P1
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = ONE;
        DestBlend = ONE;
        CullMode = NONE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderPointLight();
    }

    pass P2
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        CullMode = NONE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderEmit();
    }

    pass P3
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        CullMode = NONE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderMirror();
    }
}
