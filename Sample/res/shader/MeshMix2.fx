
float4x4 g_matWorld;
float4x4 g_matViewProj;
float4x4 g_matWorldViewProj;
float4x4 g_matMirrorViewProj;
float4x4 g_gBufferView;
float g_gBufferNear = 0.1f;
float g_gBufferFar = 33000.0f;
float g_gBufferFogNear = 0.1f;
float g_gBufferFogFar = 33000.0f;
float g_gBufferPositionRange = 33000.0f;
bool g_gBufferShadowReceiverEnabled = false;

float4 g_lightDir = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_lightPos = { -5.f, 7.f, -10.f, 0.0f };

float4 g_cameraPos = { 10.f, 5.f, 10.f, 0.0f };

float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float g_fAmbientIntensity = 1.0f;
float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

float4 g_specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };
float2 g_screenSize = { 1600.0f, 900.0f };
bool g_fresnelEnable = true;
float g_fresnelIntensity = 0.08f;
bool g_waterMirrorEnable = false;
bool g_treatTextureAsWhite = false;
bool g_damageFlash = false;
bool g_mirrorClipEnable = false;
float4 g_mirrorClipPlane = { 0.0f, 1.0f, 0.0f, 0.0f };

// スペキュラ光の鋭さ
//float g_specularPower = 16.0f;
// float g_specularPower = 128.0f;
float g_specularPower = 1.0f;

// スペキュラ光の強さ
float g_specularIntensity = 0.1f;
//float g_specularIntensity = 0.2f;
//float g_specularIntensity = 0.0f;

float g_cubeMappingRate = 1.0f;
float g_cubeMappingGauss = 0.0f;
float g_emitIntensity = 1.0f;
float4 g_emitColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// 距離フォグの色
float4 g_fogDistanceColor = { 0.5f, 0.5f, 1.0f, 1.0f };

// 距離フォグの強さ
float g_fogDistanceDensity = 0.01f;

// 高さフォグの色
float4 g_fogHeightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// 高さフォグの強さ
float g_fogHeightDensity = 0.01f;

// 空間の明るさ
// 0なら洞窟、0.1なら夜、1なら明るい室内、3なら快晴、という感じ
// 1.0を超えると彩度が上がり、逆に暗くなるようにすると面白い気がする。
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

// 環境マップ
textureCUBE g_texCubeMap;

samplerCUBE g_cubeMapSampler = sampler_state
{
    Texture = <g_texCubeMap>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;

    // どれくらいぼかすか
    // 数字が大きいほどぼかされる
    //MaxMipLevel = 7;
    MaxMipLevel = 1;
};

// 法線マップ
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

texture g_texMirror;
sampler g_mirrorSampler = sampler_state
{
    Texture = (g_texMirror);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

//------------------------------------------------------
// 視差遮蔽マッピング関連
//------------------------------------------------------

bool g_bPOM = false;
bool g_bNormalMapping = false;
bool g_bSSS = false;
float g_sssIntensity = 1.0f;
float4 g_sssColor = { 0.5f, 1.0f, 0.5f, 1.0f };

// 高さ 0.0 ~ 1.0
float g_fHeightMapScale = 0.1f;

// サンプリング数（最小）
int g_nMinSamples = 50;

// サンプリング数（最大）
int g_nMaxSamples = 100;

// 高さマップ
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

float3 SampleBaseTextureColor(float2 uv)
{
    float3 textureColor = tex2D(g_textureSampler, uv).rgb;
    if (g_treatTextureAsWhite)
    {
        textureColor = 1.0f.xxx;
    }

    return textureColor;
}


float g_time = 0.0f;
bool  g_waveEnable = false;

bool ShouldUseAlphaCutout()
{
    return (!g_waveEnable) && (g_diffuse.a <= 0.0f);
}

bool ShouldUseWaterTextureAlpha()
{
    return g_waveEnable && g_waterMirrorEnable && (g_diffuse.a <= 0.0f);
}

float GetSurfaceAlpha(float2 uv)
{
    if (ShouldUseWaterTextureAlpha())
    {
        return tex2D(g_textureSampler, uv).a;
    }

    return saturate(g_diffuse.a);
}

void ApplyAlphaCutout(float2 uv)
{
    if (ShouldUseAlphaCutout())
    {
        clip(tex2D(g_textureSampler, uv).a - 0.5f);
    }
}

//---------------------------------------------------------
// 揺らしエフェクト用パラメータ
//---------------------------------------------------------
bool  g_swayEnable = false;
float g_swayAmount = 0.5f;
float g_swaySpeed  = 5.0f;
float g_swayHeight = 3.0f;
float g_waveAmount = 0.1f;
float g_waveSpeed  = 10.0f;
float g_waveDensity = 6.5f;

float CalcWaveHeight(float x, float z)
{
    float wavePrimary = sin((x * g_waveDensity) + (g_time * g_waveSpeed * 1.7f));
    float waveSecondary = cos((z * g_waveDensity) + (g_time * g_waveSpeed * 1.2f));
    float waveGrid = wavePrimary * waveSecondary;
    float waveDiagonal = sin(((x + z) * (g_waveDensity * 0.7384615f)) + (g_time * g_waveSpeed * 2.1f));
    return ((waveGrid * 0.85f) + (waveDiagonal * 0.15f)) * g_waveAmount;
}

float3 CalcWaveNormal(float x, float z)
{
    const float sampleOffset = 0.05f;

    float heightLeft = CalcWaveHeight(x - sampleOffset, z);
    float heightRight = CalcWaveHeight(x + sampleOffset, z);
    float heightBack = CalcWaveHeight(x, z - sampleOffset);
    float heightFront = CalcWaveHeight(x, z + sampleOffset);

    float3 tangentX = normalize(float3(sampleOffset * 2.0f,
                                       heightRight - heightLeft,
                                       0.0f));
    float3 tangentZ = normalize(float3(0.0f,
                                       heightFront - heightBack,
                                       sampleOffset * 2.0f));
    return normalize(cross(tangentZ, tangentX));
}

//---------------------------------------------------------
// ポイントライト
//---------------------------------------------------------
float3 g_pointLightPos[16];
float  g_pointLightBrightness[16];
float  g_pointLightRange[16];
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
                                 float sampleRange,
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
    float fadeStart = sampleRange * 0.5f;
    float fadeLength = max(sampleRange - fadeStart, 1e-6f);
    float fadeProgress = saturate((dist - fadeStart) / fadeLength);
    float smoothFade = fadeProgress * fadeProgress * (3.0f - 2.0f * fadeProgress);
    float rangeFalloff = 1.0f - smoothFade;
    float atten = saturate(1.0f / max(dist, 1e-6f)) * rangeFalloff;

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

float3 SampleMirrorColor(float3 worldPos)
{
    float4 mirrorProj = mul(float4(worldPos, 1.0f), g_matMirrorViewProj);
    float2 mirrorUV;
    mirrorUV.x = mirrorProj.x / mirrorProj.w * 0.5f + 0.5f;
    mirrorUV.y = -mirrorProj.y / mirrorProj.w * 0.5f + 0.5f;
    return tex2D(g_mirrorSampler, mirrorUV).rgb;
}

void WriteIntegratedGBuffer(float3 worldPos,
                            float3 worldNormal,
                            out float4 outDepth,
                            out float4 outPosition,
                            out float4 outNormal)
{
    float viewSpaceZ = mul(float4(worldPos, 1.0f), g_gBufferView).z;
    float linearDepth = saturate((viewSpaceZ - g_gBufferNear) /
                                 max(g_gBufferFar - g_gBufferNear, 0.0001f));
    float fogLinearDepth = saturate((viewSpaceZ - g_gBufferFogNear) /
                                    max(g_gBufferFogFar - g_gBufferFogNear, 0.0001f));
    outDepth = float4(linearDepth, fogLinearDepth, 0.0f, 1.0f);

    float3 world01 = saturate((worldPos / g_gBufferPositionRange) * 0.5f + 0.5f);
    outPosition = float4(world01, 1.0f);

    float shadowReceiverMask = 0.0f;
    if (g_gBufferShadowReceiverEnabled)
    {
        shadowReceiverMask = 1.0f;
    }
    outNormal = float4(saturate(normalize(worldNormal) * 0.5f + 0.5f),
                       shadowReceiverMask);
}

//---------------------------------------------------------
// 頂点シェーダー
// 視差マッピングは「1パス目では実施せず、2パス目で実装する」というようなことはできない
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
    float3 localNormal = inNormal.xyz;
    float3 localTangent = inTangent.xyz;
    float3 localBinorm = inBinormal.xyz;

    // ゆらぎ効果（草とか）
    if (g_waveEnable)
    {
        float4 pos = inPosition;
        pos.y += CalcWaveHeight(pos.x, pos.z);
        inPosition = pos;

        localNormal = CalcWaveNormal(pos.x, pos.z);
        float3 referenceAxis = (abs(localNormal.y) < 0.999f) ? float3(0.0f, 1.0f, 0.0f)
                                                             : float3(1.0f, 0.0f, 0.0f);
        localTangent = normalize(cross(referenceAxis, localNormal));
        localBinorm = normalize(cross(localNormal, localTangent));
    }

    if (g_swayEnable)
    {
        float4 pos = inPosition;
    
        // 揺らしエフェクトを適用
        // Y座標の高さに基づいて揺らしの強度を変える（上にいくほど大きく揺れる）
        float heightFactor = (pos.y + 1.0) / 3.0; // 円柱の高さに合わせて調整
        heightFactor = pow(heightFactor, 2.0);
        heightFactor = clamp(heightFactor, 0.0, 1.0);
    
        // 複数の波を組み合わせて自然な揺らしを作成
        float wave1 = sin(g_time * g_swaySpeed) * g_swayAmount;
        float wave2 = sin(g_time * g_swaySpeed * 0.7 + 1.5) * g_swayAmount * 0.5;
        float wave3 = cos(g_time * g_swaySpeed * 1.3 + 2.0) * g_swayAmount * 0.3;
    
        // X軸とZ軸の両方向に揺らしを適用
        float swayX = (wave1 + wave2 + wave3) * heightFactor;
        float swayZ = (sin(g_time * g_swaySpeed * 0.8 + 0.5) * g_swayAmount * 0.7 +
                   cos(g_time * g_swaySpeed * 1.1 + 1.0) * g_swayAmount * 0.4) * heightFactor;
    
        pos.x += swayX;
        pos.z += swayZ;
        inPosition = pos;
    }

    outPosition = mul(inPosition, g_matWorldViewProj);

    // outPosWorldでは4x4を使いoutNormalWorldでは3x3の変換行列を使っている
    // こうしないと環境マップがおかしくなる
    outPosWorld = mul(inPosition, g_matWorld).xyz;

    float3x3 world3x3 = (float3x3) g_matWorld;
    outNormalWorld = mul(localNormal, world3x3);

    outTexCood = inTexCoord.xy;

    outTangent = normalize(mul(localTangent, world3x3));
    outBinorm = normalize(mul(localBinorm, world3x3));

    float3 vViewWS = g_cameraPos.xyz - outPosWorld.xyz;
    outvViewWS = vViewWS;

    // 光源ベクトル（正規化しない）
    float3 vLightWS = g_lightDir.xyz;

    // 光源ベクトル・カメラ方向ベクトルを接空間へ変換
    float3x3 mWorldToTangent = float3x3(outTangent, outBinorm, outNormalWorld);

    outvLightTS = mul(mWorldToTangent, vLightWS);
    outvViewTS = mul(mWorldToTangent, vViewWS);

    // ズレ量
    // グレージング角なら沢山ズレるし、正面を向いてるならズレない。
    // それを表す数値
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

                  out float4 outColor     : COLOR0,
                  out float4 outDepth     : COLOR1,
                  out float4 outPosition  : COLOR2,
                  out float4 outNormal    : COLOR3,
                  uniform int pointLightCount)
{
    ApplyMirrorClip(inPosWorld);

    // 正規化はピクセルシェーダーでやらないといけない
    float3 normal = normalize(inNormalWorld);
    WriteIntegratedGBuffer(inPosWorld, normal, outDepth, outPosition, outNormal);
    float3 lightDir = normalize(g_lightDir.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);

    // 接線空間へ変換済みの光ベクトルはここで正規化して使う。
    invLightTS = normalize(invLightTS);

    //----------------------------------------------------
    // 視差遮蔽マッピング
    //----------------------------------------------------
    if (g_bPOM)
    {
        inTexCoord = CalcUVCoordWithPOM(normal,
                                        inTexCoord,
                                        invViewWS,
                                        invViewTS,
                                        invParallaxOffsetTS);
    }

    ApplyAlphaCutout(inTexCoord);
    if (g_damageFlash)
    {
        outColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
        return;
    }

    float surfaceAlpha = GetSurfaceAlpha(inTexCoord);
    float3 albedo = SampleBaseTextureColor(inTexCoord) * g_diffuse.rgb;

    //-----------------------------------------------------------------------
    // 法線マッピングでNdotLを調節
    //-----------------------------------------------------------------------
    float NdotL = 0.f;
    float NdotH = 0.f;
    float3 shadingNormal = normal;

    // 法線マッピングを行うか
    if (g_bNormalMapping)
    {
        float3 normalInTangent = float3(0, 0, 0);
        normalInTangent.x = tex2D(g_normalMapSampler, inTexCoord).r * 2.0 - 1.0;
        normalInTangent.y = tex2D(g_normalMapSampler, inTexCoord).g * 2.0 - 1.0;
        normalInTangent.z = tex2D(g_normalMapSampler, inTexCoord).b * 2.0 - 1.0;
        normalInTangent.x *= -1;
        normalInTangent = normalize(normalInTangent);

        // TBN（Tangent, Binormal, Normal）でワールドへ
        float3x3 tangentToWorld = float3x3(-inTangent, -inBinorm, normal);
        float3 normalInWorld = normalize(mul(normalInTangent, tangentToWorld));

        // Lambert 拡散（光線方向）
        // saturate関数をここで実行するとマイナス成分が消える。
        shadingNormal = normalInWorld;
        NdotL = dot(normalInWorld, lightDir);
        NdotH = saturate(dot(normalInWorld, halfVector));
    }
    else
    {
        NdotL = dot(normal, lightDir);
        NdotH = saturate(dot(normal, halfVector));
    }


    float3 lambert = 0.f;
    
    // ハーフランバート
    // 深度バッファシャドウを実行すると、影が2重に表示されてしまう。
    // ハーフランバートならマシになる
    if (false)
    {
        NdotL = (NdotL + 1.0f) * 0.5f;

        // 0.5が0.7になるような補正をかける
        // 対数グラフのイメージ
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

    // 陰の彩度を上げる
    // 要らないかもしれない
    if (false)
    {
        if (NdotL <= 0.0f)
        {
            // アルベドの彩度を強調した色をアンビエント色に設定する
            // 陰の彩度を上げたいが、これだと全体的に彩度が高くなってしまう。
            float3 workColor = albedo;

            float average = (workColor.r + workColor.g + workColor.b) / 3;

            // 彩度を上げ下げする
            workColor.r = average + (workColor.r - average) * 8.0f;
            workColor.g = average + (workColor.g - average) * 8.0f;
            workColor.b = average + (workColor.b - average) * 8.0f;
            lambert = workColor * 0.05f * -NdotL;
        }
    }

    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz * g_lightColor.rgb;
    float fresnel = g_fresnelEnable ? (CalcFresnelFactor(shadingNormal, cameraDir) * g_fresnelIntensity) : 0.0f;
    float3 fresnelColor = g_specularColor.xyz * fresnel;

    float3 finalColor = ambient.rgb + lambert + specular + fresnelColor;
    if (g_waterMirrorEnable)
    {
        float reflectionBlend = saturate(fresnel);
        float3 mirrorColor = SampleMirrorColor(inPosWorld);
        finalColor = lerp(finalColor, mirrorColor * albedo, reflectionBlend);
    }

    if (g_bSSS)
    {
        float2 thicknessUV = (inScreenPos.xy + 0.5f) / g_screenSize;
        float thickness = tex2D(g_thicknessSampler, thicknessUV).r;
        float sigmaT = max(g_sssIntensity, 0.001f);
        float sssBlend = saturate(exp(-sigmaT * thickness));
        float3 sssColor = albedo * g_sssColor.rgb;
        finalColor += sssColor * sssBlend;
    }

    float3 pointLightDiffuse = 0.0f;
    float3 pointLightSpecular = 0.0f;
    if (pointLightCount > 0)
    {
        for (int pointLightIndex = 0; pointLightIndex < pointLightCount; ++pointLightIndex)
        {
            if (g_pointLightBrightness[pointLightIndex] <= 0.0f)
            {
                continue;
            }

            float3 lightSurfacePos = ClosestPointOnPointLightShape(g_pointLightPos[pointLightIndex],
                                                                   g_pointLightShape[pointLightIndex],
                                                                   g_pointLightLineLength[pointLightIndex],
                                                                   g_pointLightSquareWidth[pointLightIndex],
                                                                   g_pointLightSquareHeight[pointLightIndex],
                                                                   g_pointLightRotation[pointLightIndex].xyz,
                                                                   inPosWorld);
            float3 sampleDiffuse = 0.0f;
            float3 sampleSpecular = 0.0f;
            AccumulateSingleLightSample(lightSurfacePos,
                                        g_pointLightBrightness[pointLightIndex],
                                        g_pointLightRange[pointLightIndex],
                                        g_pointLightColor[pointLightIndex],
                                        inPosWorld,
                                        shadingNormal,
                                        cameraDir,
                                        sampleDiffuse,
                                        sampleSpecular);
            pointLightDiffuse += sampleDiffuse;
            pointLightSpecular += sampleSpecular;
        }
    }

    float3 baseColor = saturate(finalColor);
    float3 pointLightColor = (albedo * pointLightDiffuse) + pointLightSpecular;
    outColor = float4(baseColor + pointLightColor, saturate(surfaceAlpha));
    WriteIntegratedGBuffer(inPosWorld, shadingNormal, outDepth, outPosition, outNormal);

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

    ApplyMirrorClip(inPosWorld);

    ApplyAlphaCutout(inTexCoord);

    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);
    float3 reflectWorld = reflect(-cameraDir, normal);

    float cubeLod = g_cubeMappingGauss * 7.0f;
    float4 cubeSample = texCUBElod(g_cubeMapSampler, float4(reflectWorld, cubeLod));
    float3 cubeColor = cubeSample.rgb;
    float NdotH = saturate(dot(normal, halfVector));
    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz * g_lightColor.rgb;
    float fresnel = g_fresnelEnable ? (CalcFresnelFactor(normal, cameraDir) * g_fresnelIntensity) : 0.0f;
    outColor = float4(cubeColor + specular + (g_specularColor.xyz * fresnel), saturate(g_cubeMappingRate * cubeSample.a));
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

    ApplyMirrorClip(inPosWorld);

    ApplyAlphaCutout(inTexCoord);

    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);
    float3 refractWorld = refract(-cameraDir, normal, 1.f / 1.5f);
    float cubeLod = g_cubeMappingGauss * 7.0f;
    float4 cubeSample = texCUBElod(g_cubeMapSampler, float4(refractWorld, cubeLod));
    float3 cubeColor = cubeSample.rgb;
    float NdotH = saturate(dot(normal, halfVector));
    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz * g_lightColor.rgb;
    float fresnel = g_fresnelEnable ? (CalcFresnelFactor(normal, cameraDir) * g_fresnelIntensity) : 0.0f;

    outColor = float4(cubeColor + specular + (g_specularColor.xyz * fresnel), saturate(g_cubeMappingRate * cubeSample.a));
}

//-------------------------------------------------------------
// Pass 3
//-------------------------------------------------------------
// 霧の減衰関数（やわらか）
float FogAmountExp(float distance, float density)
{
    return 1 - exp(-density * distance);
}

// 霧の減衰関数（リアル）
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
    ApplyMirrorClip(inPosWorld);

    float3 normalWS = normalize(inNormalWorld);
    float3 cameraDirWS = normalize(g_cameraPos.xyz - inPosWorld);

    // POMでUV更新（必要なときだけ）
    float2 uv = inTexCoord;
    if (g_bPOM)
    {
        uv = CalcUVCoordWithPOM(normalWS,
                                inTexCoord,
                                invViewWS,
                                invViewTS,
                                invParallaxOffsetTS );
    }

    ApplyAlphaCutout(uv);
    if (g_damageFlash)
    {
        outColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float3 albedo = SampleBaseTextureColor(uv) * g_diffuse.rgb;

    float3 N = normalWS;
    if (g_bNormalMapping)
    {
        // 法線マップがある場合だけ、接空間法線をワールド空間へ変換して使う
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
                                    g_pointLightRange[i],
                                    g_pointLightColor[i],
                                    inPosWorld,
                                    N,
                                    cameraDirWS,
                                    sampleDiffuse,
                                    sampleSpecular);
        diffuseAccum += sampleDiffuse;
        specularAccum += sampleSpecular;
    }

    // 点光源はベースパスへ加算する。拡散はアルベドを保持し、鏡面だけライト色を乗せる。
    outColor = float4((albedo * diffuseAccum) + specularAccum, 0.0f);
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
                     out float4 outColor       : COLOR0,
                     out float4 outDepth       : COLOR1,
                     out float4 outPosition    : COLOR2,
                     out float4 outNormal      : COLOR3)
{
    ApplyMirrorClip(inPosWorld);

    ApplyAlphaCutout(inTexCoord);
    float3 albedo = SampleBaseTextureColor(inTexCoord) * g_diffuse.rgb;
    float3 normal = normalize(inNormalWorld);
    WriteIntegratedGBuffer(inPosWorld, normal, outDepth, outPosition, outNormal);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float fresnel = g_fresnelEnable ? (CalcFresnelFactor(normal, cameraDir) * g_fresnelIntensity) : 0.0f;
    outColor = float4((albedo * g_emitColor.rgb * g_emitIntensity) + (g_specularColor.xyz * fresnel), 1.0f);
}

void VertexShaderMirror(in  float4 inPosition   : POSITION,
                        in  float4 inNormal     : NORMAL0,
                        in  float4 inTangent    : TANGENT0,
                        in  float4 inBinormal   : BINORMAL0,
                        in  float4 inTexCoord   : TEXCOORD0,
                        out float4 outPosition  : POSITION,
                        out float4 outMirrorProj : TEXCOORD0,
                        out float3 outPosWorld   : TEXCOORD1,
                        out float3 outNormalWorld : TEXCOORD2)
{
    float4 worldPos = mul(inPosition, g_matWorld);
    outPosition = mul(inPosition, g_matWorldViewProj);
    outMirrorProj = mul(worldPos, g_matMirrorViewProj);
    outPosWorld = worldPos.xyz;
    outNormalWorld = normalize(mul(inNormal.xyz, (float3x3)g_matWorld));
}

void PixelShaderMirror(in float4 inMirrorProj : TEXCOORD0,
                       in float3 inPosWorld   : TEXCOORD1,
                       in float3 inNormalWorld : TEXCOORD2,
                       out float4 outColor    : COLOR0,
                       out float4 outDepth    : COLOR1,
                       out float4 outPosition : COLOR2,
                       out float4 outNormal   : COLOR3)
{
    float2 uv;
    uv.x = inMirrorProj.x / inMirrorProj.w * 0.5f + 0.5f;
    uv.y = -inMirrorProj.y / inMirrorProj.w * 0.5f + 0.5f;
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float fresnel = g_fresnelEnable ? (CalcFresnelFactor(inNormalWorld, cameraDir) * g_fresnelIntensity) : 0.0f;
    float4 mirrorColor = tex2D(g_mirrorSampler, uv);
    WriteIntegratedGBuffer(inPosWorld, inNormalWorld, outDepth, outPosition, outNormal);
    float mirrorBrightness = saturate(0.8f + fresnel);
    outColor = saturate(float4((mirrorColor.rgb * mirrorBrightness) + fresnel.xxx, mirrorColor.a * mirrorBrightness));
}

float2 CalcUVCoordWithPOM(float3 inNormalizedNormalWS,
                          float2 inTexCoord,
                          float3 invViewWS,
                          float3 invViewTS,
                          float2 invParallaxOffsetTS)
{
    invViewWS = normalize(invViewWS);
    invViewTS= normalize(invViewTS);

    // 視角に応じてサンプル数を変更。
    // グレージング角であるほどステップを細かくして精度を上げる。
    int nNumSteps = (int) lerp(g_nMaxSamples, g_nMinSamples, dot(invViewWS, inNormalizedNormalWS));

    float fStepSize = 1.0 / (float) nNumSteps;
    int nStepIndex = 0;

    float fCurrHeight = 0.0;

    float2 vTexOffsetPerStep = fStepSize * invParallaxOffsetTS;
    float2 vTexCurrentOffset = inTexCoord;

    // 今どの深さの層（Layer）までレイを進めたか
    float fCurrentLayer = 1.0;

    while (nStepIndex < nNumSteps)
    {
        vTexCurrentOffset -= vTexOffsetPerStep;

        // tex2Dgrad関数を使うとPIX For Windowsが落ちる
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

    // 疑似的に押し出された表面上の最終テクスチャ座標
    inTexCoord -= vParallaxOffset;
    return inTexCoord;
}

PixelShader pixelShaderPointLightVariants[4] =
{
    compile ps_3_0 PixelShader1(0),
    compile ps_3_0 PixelShader1(4),
    compile ps_3_0 PixelShader1(8),
    compile ps_3_0 PixelShader1(16)
};
int g_currentPointLightShaderIndex = 0;

technique Technique1
{
    pass Pass1
    {
        CullMode        = NONE;
        ZEnable         = TRUE;
        ZWriteEnable    = TRUE;
        AlphaBlendEnable = FALSE;
        SrcBlend        = ONE;
        DestBlend       = ZERO;
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = (pixelShaderPointLightVariants[g_currentPointLightShaderIndex]);
    }

    pass PassCubeMapping
    {
        CullMode = NONE;
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderCubeMapping();
    }

    pass PassGlass
    {
        CullMode = NONE;
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderGlass();
    }

    pass PassEmit
    {
        CullMode = NONE;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = FALSE;
        SrcBlend = ONE;
        DestBlend = ZERO;
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderEmit();
    }

    pass PassMirror
    {
        CullMode = NONE;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = FALSE;
        SrcBlend = ONE;
        DestBlend = ZERO;
        VertexShader = compile vs_3_0 VertexShaderMirror();
        PixelShader = compile ps_3_0 PixelShaderMirror();
    }

    pass PassTransparent
    {
        CullMode = NONE;
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = TRUE;
        ZWriteEnable = FALSE;
        ColorWriteEnable1 = 0;
        ColorWriteEnable2 = 0;
        ColorWriteEnable3 = 0;
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = (pixelShaderPointLightVariants[g_currentPointLightShaderIndex]);
    }

}

