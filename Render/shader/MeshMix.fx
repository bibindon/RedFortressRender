
float4x4 g_matWorld;
float4x4 g_matViewProj;
float4x4 g_matWorldViewProj;

float4 g_lightDir = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_lightPos = { -5.f, 7.f, -10.f, 0.0f };

float4 g_cameraPos = { 10.f, 5.f, 10.f, 0.0f };

float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float g_fAmbientIntensity = 1.0f;
float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

float4 g_specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };
float2 g_screenSize = { 1600.0f, 900.0f };

// ã‚¹ãƒšã‚­ãƒ¥ãƒ©å…‰ã®é‹­ã•
//float g_specularPower = 16.0f;
// float g_specularPower = 128.0f;
float g_specularPower = 1.0f;

// ã‚¹ãƒšã‚­ãƒ¥ãƒ©å…‰ã®å¼·ã•
float g_specularIntensity = 0.1f;
//float g_specularIntensity = 0.2f;
//float g_specularIntensity = 0.0f;

float g_cubeMappingRate = 1.0f;
float g_cubeMappingGauss = 0.0f;
float g_emitIntensity = 1.0f;
float4 g_emitColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// è·é›¢ãƒ•ã‚©ã‚°ã®è‰²
float4 g_fogDistanceColor = { 0.5f, 0.5f, 1.0f, 1.0f };

// è·é›¢ãƒ•ã‚©ã‚°ã®å¼·ã•
float g_fogDistanceDensity = 0.01f;

// é«˜ã•ãƒ•ã‚©ã‚°ã®è‰²
float4 g_fogHeightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// é«˜ã•ãƒ•ã‚©ã‚°ã®å¼·ã•
float g_fogHeightDensity = 0.01f;

// ç©ºé–“ã®æ˜Žã‚‹ã•
// 0ãªã‚‰æ´žçªŸã€0.1ãªã‚‰å¤œã€1ãªã‚‰æ˜Žã‚‹ã„å®¤å†…ã€3ãªã‚‰å¿«æ™´ã€ã¨ã„ã†æ„Ÿã˜
// 1.0ã‚’è¶…ãˆã‚‹ã¨å½©åº¦ãŒä¸ŠãŒã‚Šã€é€†ã«æš—ããªã‚‹ã‚ˆã†ã«ã™ã‚‹ã¨é¢ç™½ã„æ°—ãŒã™ã‚‹ã€‚
float g_fSunLightIntensity = 1.0f;
bool g_bSaturateShadow = false;
float g_fSaturateShadowIntensity = 0.2f;
float g_fShadowDarkness = 1.0f;

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

// ç’°å¢ƒãƒžãƒƒãƒ—
textureCUBE g_texCubeMap;

samplerCUBE g_cubeMapSampler = sampler_state
{
    Texture = <g_texCubeMap>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;

    // ã©ã‚Œãã‚‰ã„ã¼ã‹ã™ã‹
    // æ•°å­—ãŒå¤§ãã„ã»ã©ã¼ã‹ã•ã‚Œã‚‹
    //MaxMipLevel = 7;
    MaxMipLevel = 1;
};

// æ³•ç·šãƒžãƒƒãƒ—
texture g_texNormalMap;
sampler g_normalMapSampler = sampler_state
{
    Texture = (g_texNormalMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = Wrap;
    AddressV = Wrap;

    MaxMipLevel = 1;
};

texture g_texThickness;
sampler g_thicknessSampler = sampler_state
{
    Texture = (g_texThickness);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

//------------------------------------------------------
// è¦–å·®é®è”½ãƒžãƒƒãƒ”ãƒ³ã‚°é–¢é€£
//------------------------------------------------------

bool g_bPOM = false;
bool g_bNormalMapping = false;
bool g_bSSS = false;
float g_sssIntensity = 1.0f;
float4 g_sssColor = { 0.5f, 1.0f, 0.5f, 1.0f };

// é«˜ã• 0.0 ~ 1.0
float g_fHeightMapScale = 0.1f;

// ã‚µãƒ³ãƒ—ãƒªãƒ³ã‚°æ•°ï¼ˆæœ€å°ï¼‰
int g_nMinSamples = 50;

// ã‚µãƒ³ãƒ—ãƒªãƒ³ã‚°æ•°ï¼ˆæœ€å¤§ï¼‰
int g_nMaxSamples = 100;

// é«˜ã•ãƒžãƒƒãƒ—
texture g_texHeightMap;
sampler g_heightMapSampler = sampler_state
{
    Texture = (g_texHeightMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

float3 IncreaseSaturation(float3 color, float amount)
{
    float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
    return saturate(lerp(luminance.xxx, color, amount));
}


float g_time = 0.0f;

//---------------------------------------------------------
// æºã‚‰ã—ã‚¨ãƒ•ã‚§ã‚¯ãƒˆç”¨ãƒ‘ãƒ©ãƒ¡ãƒ¼ã‚¿
//---------------------------------------------------------
bool  g_swayEnable = false;
float g_swayAmount = 0.5f;
float g_swaySpeed  = 2.0f;
float g_swayHeight = 3.0f;

//---------------------------------------------------------
// ãƒã‚¤ãƒ³ãƒˆãƒ©ã‚¤ãƒˆ
//---------------------------------------------------------
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
                                 out float3 accumContribution,
                                 out float diffContribution)
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

    accumContribution = lightColor * (diff + spec);
    diffContribution = diff;
}

//---------------------------------------------------------
// é ‚ç‚¹ã‚·ã‚§ãƒ¼ãƒ€ãƒ¼
// è¦–å·®ãƒžãƒƒãƒ”ãƒ³ã‚°ã¯ã€Œ1ãƒ‘ã‚¹ç›®ã§ã¯å®Ÿæ–½ã›ãšã€2ãƒ‘ã‚¹ç›®ã§å®Ÿè£…ã™ã‚‹ã€ã¨ã„ã†ã‚ˆã†ãªã“ã¨ã¯ã§ããªã„
//
// WS ... WorldSpace
// TS ... TangentSpace
// OS ... ObjectSpace(Local coordinate)
//---------------------------------------------------------
void VertexShader1(in  float4 inPosition     : POSITION,
                   in  float4 inNormal       : NORMAL0,
                   in  float4 inTangent      : TANGENT0,
                   in  float4 inBinormal     : BINORMAL0,
                   in  float4 inTexCoord     : TEXCOORD0,

                   out float4 outPosition    : POSITION,
                   out float3 outPosWorld    : TEXCOORD0,
                   out float3 outNormalWorld : TEXCOORD1,
                   out float2 outTexCood     : TEXCOORD2,
                   out float3 outTangent     : TEXCOORD3,
                   out float3 outBinorm      : TEXCOORD4,
                   out float3 outvViewWS     : TEXCOORD5,
                   out float3 outvLightTS    : TEXCOORD6,
                   out float3 outvViewTS     : TEXCOORD7,
                   out float2 outvParallaxOffsetTS    : TEXCOORD8)
{
    // ã‚†ã‚‰ãŽåŠ¹æžœï¼ˆè‰ã¨ã‹ï¼‰
    if (g_swayEnable)
    {
        float4 pos = inPosition;
    
        // æºã‚‰ã—ã‚¨ãƒ•ã‚§ã‚¯ãƒˆã‚’é©ç”¨
        // Yåº§æ¨™ã®é«˜ã•ã«åŸºã¥ã„ã¦æºã‚‰ã—ã®å¼·åº¦ã‚’å¤‰ãˆã‚‹ï¼ˆä¸Šã«ã„ãã»ã©å¤§ããæºã‚Œã‚‹ï¼‰
        float heightFactor = (pos.y + 1.0) / 3.0; // å††æŸ±ã®é«˜ã•ã«åˆã‚ã›ã¦èª¿æ•´
        heightFactor = pow(heightFactor, 2.0);
        heightFactor = clamp(heightFactor, 0.0, 1.0);
    
        // è¤‡æ•°ã®æ³¢ã‚’çµ„ã¿åˆã‚ã›ã¦è‡ªç„¶ãªæºã‚‰ã—ã‚’ä½œæˆ
        float wave1 = sin(g_time * g_swaySpeed) * g_swayAmount;
        float wave2 = sin(g_time * g_swaySpeed * 0.7 + 1.5) * g_swayAmount * 0.5;
        float wave3 = cos(g_time * g_swaySpeed * 1.3 + 2.0) * g_swayAmount * 0.3;
    
        // Xè»¸ã¨Zè»¸ã®ä¸¡æ–¹å‘ã«æºã‚‰ã—ã‚’é©ç”¨
        float swayX = (wave1 + wave2 + wave3) * heightFactor;
        float swayZ = (sin(g_time * g_swaySpeed * 0.8 + 0.5) * g_swayAmount * 0.7 +
                   cos(g_time * g_swaySpeed * 1.1 + 1.0) * g_swayAmount * 0.4) * heightFactor;
    
        pos.x += swayX;
        pos.z += swayZ;
        inPosition = pos;
    }

    outPosition = mul(inPosition, g_matWorldViewProj);

    // outPosWorldã§ã¯4x4ã‚’ä½¿ã„outNormalWorldã§ã¯3x3ã®å¤‰æ›è¡Œåˆ—ã‚’ä½¿ã£ã¦ã„ã‚‹
    // ã“ã†ã—ãªã„ã¨ç’°å¢ƒãƒžãƒƒãƒ—ãŒãŠã‹ã—ããªã‚‹
    outPosWorld = mul(inPosition, g_matWorld).xyz;

    float3x3 world3x3 = (float3x3) g_matWorld;
    outNormalWorld = mul(inNormal.xyz, world3x3);

    outTexCood = inTexCoord.xy;

    outTangent = normalize(mul(inTangent.xyz, world3x3));
    outBinorm = normalize(mul(inBinormal.xyz, world3x3));

    float3 vViewWS = g_cameraPos.xyz - outPosWorld.xyz;
    outvViewWS = vViewWS;

    // å…‰æºãƒ™ã‚¯ãƒˆãƒ«ï¼ˆæ­£è¦åŒ–ã—ãªã„ï¼‰
    float3 vLightWS = g_lightDir.xyz;

    // å…‰æºãƒ™ã‚¯ãƒˆãƒ«ãƒ»ã‚«ãƒ¡ãƒ©æ–¹å‘ãƒ™ã‚¯ãƒˆãƒ«ã‚’æŽ¥ç©ºé–“ã¸å¤‰æ›
    float3x3 mWorldToTangent = float3x3(outTangent, outBinorm, outNormalWorld);

    outvLightTS = mul(mWorldToTangent, vLightWS);
    outvViewTS = mul(mWorldToTangent, vViewWS);

    // ã‚ºãƒ¬é‡
    // ã‚°ãƒ¬ãƒ¼ã‚¸ãƒ³ã‚°è§’ãªã‚‰æ²¢å±±ã‚ºãƒ¬ã‚‹ã—ã€æ­£é¢ã‚’å‘ã„ã¦ã‚‹ãªã‚‰ã‚ºãƒ¬ãªã„ã€‚
    // ãã‚Œã‚’è¡¨ã™æ•°å€¤
    outvParallaxOffsetTS = outvViewTS.xy / outvViewTS.z;

    outvParallaxOffsetTS *= g_fHeightMapScale;
}

float2 CalcUVCoordWithPOM(float3 inNormalizedNormalWS,
                          float2 inTexCoord,
                          float3 invViewWS,
                          float3 invViewTS,
                          float2 invParallaxOffsetTS);

//-------------------------------------------------------------
// Pass 0
//-------------------------------------------------------------
void PixelShader1(in float2 inScreenPos   : VPOS,
                  in float3 inPosWorld    : TEXCOORD0,
                  in float3 inNormalWorld : TEXCOORD1,
                  in float2 inTexCoord    : TEXCOORD2,
                  in float3 inTangent     : TEXCOORD3,
                  in float3 inBinorm      : TEXCOORD4,
                  in float3 invViewWS     : TEXCOORD5,
                  in float3 invLightTS    : TEXCOORD6,
                  in float3 invViewTS     : TEXCOORD7,
                  in float2 invParallaxOffsetTS  : TEXCOORD8,

                  out float4 outColor     : COLOR)
{
    // æ­£è¦åŒ–ã¯ãƒ”ã‚¯ã‚»ãƒ«ã‚·ã‚§ãƒ¼ãƒ€ãƒ¼ã§ã‚„ã‚‰ãªã„ã¨ã„ã‘ãªã„
    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);

    // æŽ¥ç·šç©ºé–“ã¸å¤‰æ›æ¸ˆã¿ã®å…‰ãƒ™ã‚¯ãƒˆãƒ«ã¯ã“ã“ã§æ­£è¦åŒ–ã—ã¦ä½¿ã†ã€‚
    invLightTS = normalize(invLightTS);

    //----------------------------------------------------
    // è¦–å·®é®è”½ãƒžãƒƒãƒ”ãƒ³ã‚°
    //----------------------------------------------------
    if (g_bPOM)
    {
        inTexCoord = CalcUVCoordWithPOM(normal,
                                        inTexCoord,
                                        invViewWS,
                                        invViewTS,
                                        invParallaxOffsetTS);
    }
    
    float3 albedo = tex2D(g_textureSampler, inTexCoord).rgb * g_diffuse.rgb;

    //-----------------------------------------------------------------------
    // æ³•ç·šãƒžãƒƒãƒ”ãƒ³ã‚°ã§NdotLã‚’èª¿ç¯€
    //-----------------------------------------------------------------------
    float NdotL = 0.f;
    float NdotH = 0.f;

    // æ³•ç·šãƒžãƒƒãƒ”ãƒ³ã‚°ã‚’è¡Œã†ã‹
    if (g_bNormalMapping)
    {
        float3 normalInTangent = float3(0, 0, 0);
        normalInTangent.x = tex2D(g_normalMapSampler, inTexCoord).r * 2.0 - 1.0;
        normalInTangent.y = tex2D(g_normalMapSampler, inTexCoord).g * 2.0 - 1.0;
        normalInTangent.z = tex2D(g_normalMapSampler, inTexCoord).b * 2.0 - 1.0;
        normalInTangent.x *= -1;
        normalInTangent = normalize(normalInTangent);

        // TBNï¼ˆTangent, Binormal, Normalï¼‰ã§ãƒ¯ãƒ¼ãƒ«ãƒ‰ã¸
        float3x3 tangentToWorld = float3x3(-inTangent, -inBinorm, normal);
        float3 normalInWorld = normalize(mul(normalInTangent, tangentToWorld));

        // Lambert æ‹¡æ•£ï¼ˆå…‰ç·šæ–¹å‘ï¼‰
        // saturateé–¢æ•°ã‚’ã“ã“ã§å®Ÿè¡Œã™ã‚‹ã¨ãƒžã‚¤ãƒŠã‚¹æˆåˆ†ãŒæ¶ˆãˆã‚‹ã€‚
        NdotL = dot(normalInWorld, lightDir);
        NdotH = saturate(dot(normalInWorld, halfVector));
    }
    else
    {
        NdotL = dot(normal, lightDir);
        NdotH = saturate(dot(normal, halfVector));
    }


    float3 lambert = 0.f;
    
    // ãƒãƒ¼ãƒ•ãƒ©ãƒ³ãƒãƒ¼ãƒˆ
    // æ·±åº¦ãƒãƒƒãƒ•ã‚¡ã‚·ãƒ£ãƒ‰ã‚¦ã‚’å®Ÿè¡Œã™ã‚‹ã¨ã€å½±ãŒ2é‡ã«è¡¨ç¤ºã•ã‚Œã¦ã—ã¾ã†ã€‚
    // ãƒãƒ¼ãƒ•ãƒ©ãƒ³ãƒãƒ¼ãƒˆãªã‚‰ãƒžã‚·ã«ãªã‚‹
    if (false)
    {
        NdotL = (NdotL + 1.0f) * 0.5f;

        // 0.5ãŒ0.7ã«ãªã‚‹ã‚ˆã†ãªè£œæ­£ã‚’ã‹ã‘ã‚‹
        // å¯¾æ•°ã‚°ãƒ©ãƒ•ã®ã‚¤ãƒ¡ãƒ¼ã‚¸
        NdotL = pow(NdotL, 0.5);
    }
    else
    {
        NdotL = saturate(NdotL);
    }

    float shadowAmount = saturate(1.0f - NdotL);
    float3 shadowAlbedo = albedo;
    if (g_bSaturateShadow)
    {
        float saturationAmount = 1.0f + (shadowAmount * g_fSaturateShadowIntensity);
        shadowAlbedo = IncreaseSaturation(albedo, saturationAmount);
    }

    lambert = shadowAlbedo
            * (1.0f - ((1.0f - NdotL) * g_fShadowDarkness))
            * g_lightColor.rgb
            * g_fSunLightIntensity;

    float3 ambient = g_ambient.rgb * g_fAmbientIntensity * albedo;

    // é™°ã®å½©åº¦ã‚’ä¸Šã’ã‚‹
    // è¦ã‚‰ãªã„ã‹ã‚‚ã—ã‚Œãªã„
    if (false)
    {
        if (NdotL <= 0.0f)
        {
            // ã‚¢ãƒ«ãƒ™ãƒ‰ã®å½©åº¦ã‚’å¼·èª¿ã—ãŸè‰²ã‚’ã‚¢ãƒ³ãƒ“ã‚¨ãƒ³ãƒˆè‰²ã«è¨­å®šã™ã‚‹
            // é™°ã®å½©åº¦ã‚’ä¸Šã’ãŸã„ãŒã€ã“ã‚Œã ã¨å…¨ä½“çš„ã«å½©åº¦ãŒé«˜ããªã£ã¦ã—ã¾ã†ã€‚
            float3 workColor = albedo;

            float average = (workColor.r + workColor.g + workColor.b) / 3;

            // å½©åº¦ã‚’ä¸Šã’ä¸‹ã’ã™ã‚‹
            workColor.r = average + (workColor.r - average) * 8.0f;
            workColor.g = average + (workColor.g - average) * 8.0f;
            workColor.b = average + (workColor.b - average) * 8.0f;
            lambert = workColor * 0.05f * -NdotL;
        }
    }

    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz * g_lightColor.rgb;

    float3 finalColor = ambient.rgb + lambert + specular;

    if (g_bSSS)
    {
        float2 thicknessUV = (inScreenPos.xy + 0.5f) / g_screenSize;
        float thickness = tex2D(g_thicknessSampler, thicknessUV).r;
        float sigmaT = max(g_sssIntensity, 0.001f);
        float sssBlend = saturate(exp(-sigmaT * thickness));
        float3 sssColor = albedo * g_sssColor.rgb;
        finalColor += sssColor * sssBlend;
    }

    outColor = saturate(float4(finalColor, 1.f));

    if (false)
    {
        if (abs(inScreenPos.x - (g_screenSize.x * 0.5f)) <= 0.5f)
        {
            outColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
    }
}

//-------------------------------------------------------------
// Pass 1
//-------------------------------------------------------------
void PixelShaderCubeMapping(in float4 inPosition     : POSITION,
                            in float3 inPosWorld     : TEXCOORD0,
                            in float3 inNormalWorld  : TEXCOORD1,
                            in float2 inTexCoord      : TEXCOORD2,
                            in float3 inTangent      : TEXCOORD3,
                            in float3 inBinorm       : TEXCOORD4,

                            out float4 outColor      : COLOR)
{
    outColor = float4(0, 0, 0, 0);

    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);
    float3 reflectWorld = reflect(-cameraDir, normal);

    float cubeLod = g_cubeMappingGauss * 7.0f;
    float3 cubeColor = texCUBElod(g_cubeMapSampler, float4(reflectWorld, cubeLod)).rgb;
    float NdotH = saturate(dot(normal, halfVector));
    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz * g_lightColor.rgb;
    outColor = float4(cubeColor + specular, saturate(g_cubeMappingRate));
}

//-------------------------------------------------------------
// Pass 2
//-------------------------------------------------------------
void PixelShaderGlass(in float4 inPosition     : POSITION,
                      in float3 inPosWorld     : TEXCOORD0,
                      in float3 inNormalWorld  : TEXCOORD1,
                      in float2 inTexCoord      : TEXCOORD2,
                      in float3 inTangent      : TEXCOORD3,
                      in float3 inBinorm       : TEXCOORD4,

                      out float4 outColor      : COLOR)
{
    outColor = float4(0, 0, 0, 0);

    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);
    float3 refractWorld = refract(-cameraDir, normal, 1.f / 1.5f);
    float cubeLod = g_cubeMappingGauss * 7.0f;
    float3 cubeColor = texCUBElod(g_cubeMapSampler, float4(refractWorld, cubeLod)).rgb;
    float NdotH = saturate(dot(normal, halfVector));
    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz * g_lightColor.rgb;

    outColor = float4(cubeColor + specular, saturate(g_cubeMappingRate));
}

//-------------------------------------------------------------
// Pass 3
//-------------------------------------------------------------
// éœ§ã®æ¸›è¡°é–¢æ•°ï¼ˆã‚„ã‚ã‚‰ã‹ï¼‰
float FogAmountExp(float distance, float density)
{
    return 1 - exp(-density * distance);
}

// éœ§ã®æ¸›è¡°é–¢æ•°ï¼ˆãƒªã‚¢ãƒ«ï¼‰
float FogAmountExp2(float distance, float density)
{
    float x = density * distance;
    return 1 - exp(-x * x);
}

void PixelShaderPointLight(in  float4 inPosition            : POSITION,
                           in  float3 inPosWorld            : TEXCOORD0,
                           in  float3 inNormalWorld         : TEXCOORD1,
                           in  float2 inTexCoord            : TEXCOORD2,
                           in  float3 inTangent             : TEXCOORD3,
                           in  float3 inBinorm              : TEXCOORD4,
                           in  float3 invViewWS             : TEXCOORD5,
                           in  float3 invViewTS             : TEXCOORD7,
                           in  float2 invParallaxOffsetTS   : TEXCOORD8,
                           out float4 outColor              : COLOR)
{
    float3 normalWS = normalize(inNormalWorld);
    float3 cameraDirWS = normalize(g_cameraPos.xyz - inPosWorld);

    // POMã§UVæ›´æ–°ï¼ˆå¿…è¦ãªã¨ãã ã‘ï¼‰
    float2 uv = inTexCoord;
    if (g_bPOM)
    {
        uv = CalcUVCoordWithPOM(normalWS,
                                inTexCoord,
                                invViewWS,
                                invViewTS,
                                invParallaxOffsetTS );
    }

    float3 N = normalWS;
    if (g_bNormalMapping)
    {
        // æ³•ç·šãƒžãƒƒãƒ—ãŒã‚ã‚‹å ´åˆã ã‘ã€æŽ¥ç©ºé–“æ³•ç·šã‚’ãƒ¯ãƒ¼ãƒ«ãƒ‰ç©ºé–“ã¸å¤‰æ›ã—ã¦ä½¿ã†
        float3 normalTS;
        float4 nTex = tex2D(g_normalMapSampler, uv);
        normalTS.x = nTex.r * 2.0 - 1.0;
        normalTS.y = nTex.g * 2.0 - 1.0;
        normalTS.z = nTex.b * 2.0 - 1.0;
        normalTS.x *= -1.0;
        normalTS = normalize(normalTS);

        float3x3 tbn = float3x3(-inTangent, -inBinorm, normalWS);
        N = normalize(mul(normalTS, tbn));
    }

    float3 accum = 0.0;

    float diffSum = 0.f;

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
        float3 sampleAccum = 0.0f;
        float sampleDiff = 0.0f;
        AccumulateSingleLightSample(lightSurfacePos,
                                    g_pointLightBrightness[i],
                                    g_pointLightColor[i],
                                    inPosWorld,
                                    N,
                                    cameraDirWS,
                                    sampleAccum,
                                    sampleDiff);
        accum += sampleAccum;
        diffSum += sampleDiff;
    }

    // HDRè“„ç©ï¼šã“ã“ã§ã‚¯ãƒ©ãƒ³ãƒ—ã—ãªã„
    outColor = float4(accum, saturate(diffSum));
}

void PixelShaderEmit(in  float4 inPosition     : POSITION,
                     in  float3 inPosWorld     : TEXCOORD0,
                     in  float3 inNormalWorld  : TEXCOORD1,
                     in  float2 inTexCoord     : TEXCOORD2,
                     in  float3 inTangent      : TEXCOORD3,
                     in  float3 inBinorm       : TEXCOORD4,
                     in  float3 invViewWS      : TEXCOORD5,
                     in  float3 invLightTS     : TEXCOORD6,
                     in  float3 invViewTS      : TEXCOORD7,
                     in  float2 invParallaxOffsetTS  : TEXCOORD8,
                     out float4 outColor       : COLOR)
{
    float3 albedo = tex2D(g_textureSampler, inTexCoord).rgb * g_diffuse.rgb;
    outColor = float4(albedo * g_emitColor.rgb * g_emitIntensity, 1.0f);
}

float2 CalcUVCoordWithPOM(float3 inNormalizedNormalWS,
                          float2 inTexCoord,
                          float3 invViewWS,
                          float3 invViewTS,
                          float2 invParallaxOffsetTS)
{
    invViewWS = normalize(invViewWS);
    invViewTS= normalize(invViewTS);

    // è¦–è§’ã«å¿œã˜ã¦ã‚µãƒ³ãƒ—ãƒ«æ•°ã‚’å¤‰æ›´ã€‚
    // ã‚°ãƒ¬ãƒ¼ã‚¸ãƒ³ã‚°è§’ã§ã‚ã‚‹ã»ã©ã‚¹ãƒ†ãƒƒãƒ—ã‚’ç´°ã‹ãã—ã¦ç²¾åº¦ã‚’ä¸Šã’ã‚‹ã€‚
    int nNumSteps = (int) lerp(g_nMaxSamples, g_nMinSamples, dot(invViewWS, inNormalizedNormalWS));

    float fStepSize = 1.0 / (float) nNumSteps;
    int nStepIndex = 0;

    float fCurrHeight = 0.0;

    float2 vTexOffsetPerStep = fStepSize * invParallaxOffsetTS;
    float2 vTexCurrentOffset = inTexCoord;

    // ä»Šã©ã®æ·±ã•ã®å±¤ï¼ˆLayerï¼‰ã¾ã§ãƒ¬ã‚¤ã‚’é€²ã‚ãŸã‹
    float fCurrentLayer = 1.0;

    while (nStepIndex < nNumSteps)
    {
        vTexCurrentOffset -= vTexOffsetPerStep;

        // tex2Dgradé–¢æ•°ã‚’ä½¿ã†ã¨PIX For WindowsãŒè½ã¡ã‚‹
        // fCurrHeight = tex2Dgrad(g_heightMapSampler, vTexCurrentOffset, dx, dy ).r;
        fCurrHeight = tex2Dlod(g_heightMapSampler, float4(vTexCurrentOffset, 0.0f, 0.0f)).r;

        fCurrentLayer -= fStepSize;

        if (fCurrHeight > fCurrentLayer)
        {
            break;
        }

        nStepIndex++;
    }

    float2 vParallaxOffset = invParallaxOffsetTS * (1 - fCurrentLayer);

    // ç–‘ä¼¼çš„ã«æŠ¼ã—å‡ºã•ã‚ŒãŸè¡¨é¢ä¸Šã®æœ€çµ‚ãƒ†ã‚¯ã‚¹ãƒãƒ£åº§æ¨™
    inTexCoord -= vParallaxOffset;
    return inTexCoord;
}

technique Technique1
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
    }

    pass PassCubeMapping
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderCubeMapping();
    }

    pass PassGlass
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderGlass();
    }

    pass PassPointLight
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderPointLight();
    }

    pass PassEmit
    {
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderEmit();
    }

}

