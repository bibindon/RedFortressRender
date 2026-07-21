

#include "Common.fx"


float4x4 g_matWorld;
float4x4 g_matWorldViewProj;

float4x4 g_matLightView;
float    g_lightNear;
float    g_lightFar;
float4x4 g_matLightViewProj;
float4x4 g_matInverseView;
float4x4 g_matInverseProjection;
float4x4 g_matLightViewFar;
float4x4 g_matLightViewProjFar;
float    g_lightNearFarCascade;
float    g_lightFarFarCascade;
float    g_receiverDepthNear;
float    g_receiverDepthFar;
float    g_sceneDepthNear;
float    g_sceneDepthFar;
float    g_receiverTexelW;
float    g_receiverTexelH;
float    g_instancingAlphaClipThreshold;
float    g_meshAlphaClipThreshold;
int      g_swayMode;
float    g_time;
bool     g_useMeshAlphaCutout;
bool     g_writeNearCascade;

float4x3 g_matWorldArray[MAX_MATRICES];
int g_currentBoneIndex;

float g_shadowTexelW;
float g_shadowTexelH;
float g_shadowFarTexelW;
float g_shadowFarTexelH;
float g_compositeTexelW;
float g_compositeTexelH;
float g_compositeFarTexelW;
float g_compositeFarTexelH;

// 影の端に表示されるギザギザを抑制。0.002～0.005 で調整
float g_shadowBias;
float g_shadowBiasFar;

// 影の濃さ(0 ~ 1)
float g_shadowIntensity;
float g_shadowSaturationBoost;
float g_edgeDepthThreshold;
float g_edgeNormalThreshold;
int g_shadowPcfTapCount;
int g_shadowCompositeTapCount;
bool g_farCascadeEnabled = false;

texture g_texLightZ;
sampler samplerLightZ = sampler_state
{
    Texture   = (g_texLightZ);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

texture g_texLightZFar;
sampler samplerLightZFar = sampler_state
{
    Texture   = (g_texLightZFar);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

texture g_texBase;
sampler samplerBase = sampler_state {
    Texture = (g_texBase);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
};

texture g_texShadow;
sampler samplerShadow = sampler_state
{
    Texture   = (g_texShadow);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
};

texture g_texShadowFar;
sampler samplerShadowFar = sampler_state
{
    Texture   = (g_texShadowFar);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
};

texture g_texSceneDepth;
sampler samplerSceneDepth = sampler_state
{
    Texture   = (g_texSceneDepth);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

texture g_texReceiverDepth;
sampler samplerReceiverDepth = sampler_state
{
    Texture   = (g_texReceiverDepth);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

texture g_texSceneNormal;
sampler samplerSceneNormal = sampler_state
{
    Texture   = (g_texSceneNormal);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

texture g_texInstancingAlpha;
sampler sampInstancingAlpha = sampler_state
{
    Texture   = (g_texInstancingAlpha);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU  = CLAMP;
    AddressV  = CLAMP;
};

texture g_texMeshAlpha;
sampler sampMeshAlpha = sampler_state
{
    Texture   = (g_texMeshAlpha);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU  = WRAP;
    AddressV  = WRAP;
};

float3 IncreaseSaturation(float3 color, float amount)
{
    float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
    return max(lerp(luminance.xxx, color, amount), 0.0f);
}

float3 DecodeWorldNormal(float3 encodedNormal)
{
    float3 normal = encodedNormal * 2.0f - 1.0f;
    float normalLength = length(normal);
    if (normalLength <= 0.0001f)
    {
        return float3(0.0f, 1.0f, 0.0f);
    }

    return normal / normalLength;
}

void AccumulateShadowDepthSample(float2 uvLightView,
                                 float fDepthLightView,
                                 float2 uvTexel,
                                 int x,
                                 int y,
                                 inout float shadowSum,
                                 inout float sampleCount)
{
    float2 sampleUv = uvLightView + float2((float)x, (float)y) * uvTexel;
    if (any(sampleUv < 0.0f) || any(sampleUv > 1.0f))
    {
        return;
    }

    float shadowDepth = tex2Dlod(samplerLightZ, float4(sampleUv, 0, 0)).r;
    if (shadowDepth < (fDepthLightView - g_shadowBias))
    {
        shadowSum += 1.0f;
    }

    sampleCount += 1.0f;
}

float FinalizeShadowAmount(float shadowSum, float sampleCount)
{
    if (sampleCount <= 0.0f)
    {
        return 0.0f;
    }

    return shadowSum / max(sampleCount, 1.0f);
}

float SampleShadowAmount1(float2 uvLightView, float fDepthLightView)
{
    float2 uvTexel = float2(g_shadowTexelW, g_shadowTexelH);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;
    AccumulateShadowDepthSample(uvLightView, fDepthLightView, uvTexel, 0, 0, shadowSum, sampleCount);
    return FinalizeShadowAmount(shadowSum, sampleCount);
}

float SampleShadowAmount3(float2 uvLightView, float fDepthLightView)
{
    float2 uvTexel = float2(g_shadowTexelW, g_shadowTexelH);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;

    [loop]
    for (int y = -1; y <= 1; ++y)
    {
        [loop]
        for (int x = -1; x <= 1; ++x)
        {
            AccumulateShadowDepthSample(uvLightView, fDepthLightView, uvTexel, x, y, shadowSum, sampleCount);
        }
    }

    return FinalizeShadowAmount(shadowSum, sampleCount);
}

float SampleShadowAmount5(float2 uvLightView, float fDepthLightView)
{
    float2 uvTexel = float2(g_shadowTexelW, g_shadowTexelH);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;

    [loop]
    for (int y = -2; y <= 2; ++y)
    {
        [loop]
        for (int x = -2; x <= 2; ++x)
        {
            AccumulateShadowDepthSample(uvLightView, fDepthLightView, uvTexel, x, y, shadowSum, sampleCount);
        }
    }

    return FinalizeShadowAmount(shadowSum, sampleCount);
}

float SampleShadowAmount7(float2 uvLightView, float fDepthLightView)
{
    float2 uvTexel = float2(g_shadowTexelW, g_shadowTexelH);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;

    [loop]
    for (int y = -3; y <= 3; ++y)
    {
        [loop]
        for (int x = -3; x <= 3; ++x)
        {
            AccumulateShadowDepthSample(uvLightView, fDepthLightView, uvTexel, x, y, shadowSum, sampleCount);
        }
    }

    return FinalizeShadowAmount(shadowSum, sampleCount);
}

float SampleShadowAmount9(float2 uvLightView, float fDepthLightView)
{
    float2 uvTexel = float2(g_shadowTexelW, g_shadowTexelH);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;

    [loop]
    for (int y = -4; y <= 4; ++y)
    {
        [loop]
        for (int x = -4; x <= 4; ++x)
        {
            AccumulateShadowDepthSample(uvLightView, fDepthLightView, uvTexel, x, y, shadowSum, sampleCount);
        }
    }

    return FinalizeShadowAmount(shadowSum, sampleCount);
}

float SampleShadowAmount11(float2 uvLightView, float fDepthLightView)
{
    float2 uvTexel = float2(g_shadowTexelW, g_shadowTexelH);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;

    [loop]
    for (int y = -5; y <= 5; ++y)
    {
        [loop]
        for (int x = -5; x <= 5; ++x)
        {
            AccumulateShadowDepthSample(uvLightView, fDepthLightView, uvTexel, x, y, shadowSum, sampleCount);
        }
    }

    return FinalizeShadowAmount(shadowSum, sampleCount);
}

// 変数名の末尾のOSはローカル座標の意味
// 変数名の末尾のWSはグローバル座標の意味

//-------------------------------------------------------------------------
// Technique 1
//-------------------------------------------------------------------------

struct VSInDepth
{
    float4 vPosOS  : POSITION0;
    float2 uv      : TEXCOORD0;
};

struct VSOutDepth
{
    float4 vPos    : POSITION0;
    float  fDepth  : TEXCOORD0;
    float2 uv      : TEXCOORD1;
};

struct VSInShadowOccluderInstancing
{
    float4 positionObject : POSITION0;
    float3 normalObject   : NORMAL0;
    float2 texCoord       : TEXCOORD0;
    float4 instancePosRot : TEXCOORD1;
    float4 instanceScale  : TEXCOORD2;
};

struct VSOutShadowOccluderInstancing
{
    float4 positionClip : POSITION0;
    float2 alphaUV      : TEXCOORD0;
};

float3 SkinPosition(float4 inPosition, float4 inBlendWeights, float4 inBlendIndices, int boneNumber)
{
    float3 position = 0.0f;
    float lastWeight = 0.0f;

    int4 indexVector = (int4)inBlendIndices;
    float blendWeightsArray[4] = (float[4])inBlendWeights;
    int indexArray[4] = (int[4])indexVector;

    [unroll] for (int i = 0; i < boneNumber - 1; ++i)
    {
        lastWeight += blendWeightsArray[i];
        position += mul(inPosition, g_matWorldArray[indexArray[i]]) * blendWeightsArray[i];
    }

    lastWeight = 1.0f - lastWeight;
    position += mul(inPosition, g_matWorldArray[indexArray[boneNumber - 1]]) * lastWeight;
    return position;
}

VSOutDepth VS_DepthFromLight(VSInDepth vin)
{
    VSOutDepth vout;

    vout.vPos = mul(vin.vPosOS, g_matWorldViewProj);

    float4 inWorldPos = mul(vin.vPosOS, g_matWorld);
    float4 vPosLightView    = mul(inWorldPos, g_matLightView);


    // 線形深度（ライト View 空間 z を near..far で正規化）
    vout.fDepth = vPosLightView.z;
    vout.uv = vin.uv;

    return vout;
}

void VS_DepthFromLightSkin(in  float4 inPosition     : POSITION,
                           in  float4 inBlendWeights : BLENDWEIGHT,
                           in  float4 inBlendIndices : BLENDINDICES,
                           in  float2 inUV           : TEXCOORD0,
                           out float4 outPosition    : POSITION0,
                           out float  outDepth       : TEXCOORD0,
                           out float2 outUV          : TEXCOORD1,
                           uniform int boneNumber);

VertexShader vsDepthSkinArray[4] =
{
    compile vs_3_0 VS_DepthFromLightSkin(1),
    compile vs_3_0 VS_DepthFromLightSkin(2),
    compile vs_3_0 VS_DepthFromLightSkin(3),
    compile vs_3_0 VS_DepthFromLightSkin(4)
};

void VS_DepthFromLightSkin(in  float4 inPosition     : POSITION,
                           in  float4 inBlendWeights : BLENDWEIGHT,
                           in  float4 inBlendIndices : BLENDINDICES,
                           in  float2 inUV           : TEXCOORD0,
                           out float4 outPosition    : POSITION0,
                           out float  outDepth       : TEXCOORD0,
                           out float2 outUV          : TEXCOORD1,
                           uniform int boneNumber)
{
    float3 worldPos = SkinPosition(inPosition, inBlendWeights, inBlendIndices, boneNumber);
    float4 posLightView = mul(float4(worldPos, 1.0f), g_matLightView);

    outPosition = mul(float4(worldPos, 1.0f), g_matLightViewProj);
    outDepth = posLightView.z;
    outUV = inUV;
}

float4 PS_DepthFromLight(VSOutDepth pin) : COLOR0
{
    if (g_useMeshAlphaCutout)
    {
        clip(tex2D(sampMeshAlpha, pin.uv).a - g_meshAlphaClipThreshold);
    }

    float d = saturate((pin.fDepth - g_lightNear) / (g_lightFar - g_lightNear));
    return float4(d, d, d, 1.0f);
}

VSOutShadowOccluderInstancing VS_ShadowOccluderInstancing(VSInShadowOccluderInstancing inputData)
{
    VSOutShadowOccluderInstancing outputData;

    float scale = inputData.instanceScale.x;
    float rotationY = inputData.instancePosRot.w;
    float sinY = sin(rotationY);
    float cosY = cos(rotationY);

    float3 scaledPos = inputData.positionObject.xyz * scale;
    float swayWeight = saturate(1.0f - inputData.texCoord.y);
    swayWeight *= swayWeight;
    if (g_swayMode == 1)
    {
        float phase = g_time * 1.7f + inputData.instancePosRot.x * 0.27f + inputData.instancePosRot.z * 0.19f;
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
        float waveCoord = dot(inputData.instancePosRot.xz, waveDir);
        float phase = g_time * 2.2f - waveCoord * 0.42f;
        float broadWave = sin(phase) * 0.22f;
        float detailWave = sin(phase * 1.7f + inputData.instancePosRot.x * 0.07f) * 0.07f;
        float wave = (broadWave + detailWave) * swayWeight * scale;
        rotatedPos.x += waveDir.x * wave;
        rotatedPos.z += waveDir.y * wave;
    }

    float3 worldPos = rotatedPos + inputData.instancePosRot.xyz;
    outputData.positionClip = mul(float4(worldPos, 1.0f), g_matWorldViewProj);
    outputData.alphaUV = inputData.texCoord;
    return outputData;
}

float4 PS_ShadowOccluderInstancing(VSOutShadowOccluderInstancing inputData) : COLOR0
{
    clip(tex2D(sampInstancingAlpha, inputData.alphaUV).a - g_instancingAlphaClipThreshold);
    return 0.0f;
}

//-------------------------------------------------------------------------
// Technique 2
//-------------------------------------------------------------------------

void VS_Base(in  float4 inPosOS     : POSITION,
             in  float2 inUV        : TEXCOORD0,

             out float4 outPos      : POSITION0,
             out float2 outUV       : TEXCOORD0,
             out float3 outWorldPos : TEXCOORD1)
{
    float4 vPos = mul(inPosOS, g_matWorldViewProj);
    outPos = vPos;
    outUV = inUV;

    float4 posWS = mul(inPosOS, g_matWorld);
    outWorldPos = posWS.xyz;
}

void VS_BaseSkin(in  float4 inPosition     : POSITION,
                 in  float4 inBlendWeights : BLENDWEIGHT,
                 in  float4 inBlendIndices : BLENDINDICES,
                 in  float2 inUV           : TEXCOORD0,
                 out float4 outPosition    : POSITION0,
                 out float2 outUV          : TEXCOORD0,
                 out float3 outWorldPos    : TEXCOORD1,
                 uniform int boneNumber);

VertexShader vsBaseSkinArray[4] =
{
    compile vs_3_0 VS_BaseSkin(1),
    compile vs_3_0 VS_BaseSkin(2),
    compile vs_3_0 VS_BaseSkin(3),
    compile vs_3_0 VS_BaseSkin(4)
};

void VS_BaseSkin(in  float4 inPosition     : POSITION,
                 in  float4 inBlendWeights : BLENDWEIGHT,
                 in  float4 inBlendIndices : BLENDINDICES,
                 in  float2 inUV           : TEXCOORD0,
                 out float4 outPosition    : POSITION0,
                 out float2 outUV          : TEXCOORD0,
                 out float3 outWorldPos    : TEXCOORD1,
                 uniform int boneNumber)
{
    float3 worldPos = SkinPosition(inPosition, inBlendWeights, inBlendIndices, boneNumber);
    outPosition = mul(float4(worldPos, 1.0f), g_matWorldViewProj);
    outUV = inUV;
    outWorldPos = worldPos;
}

float4 BuildWriteShadowColor(float3 inWorldPos, float nShadowColor)
{
    float4 outColor = float4(0, 0, 0, 0);
    
    //---------------------------------------------------------
    // カメラから見た各ピクセルのワールド座標の位置を
    // もし、光源の位置から見たら、深度はいくら？、を求める
    //---------------------------------------------------------
    float4 vPosLightView = mul(float4(inWorldPos, 1.0f), g_matLightView);

    float  fDepthLightView = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    fDepthLightView = saturate(fDepthLightView);

    //---------------------------------------------------------
    // カメラから見た各ピクセルのワールド座標の位置を
    // もし、光源の位置から見たら、座標はどこ？、を求める
    //---------------------------------------------------------
    float4 vClipLightView = mul(float4(inWorldPos, 1.0f), g_matLightViewProj);

    // ライトから見て背面（w <= 0）は「影なし」扱い
    if (vPosLightView.w <= 0)
    {
        outColor.a = 0.0f;
        return outColor;
    }

    // 2D平面の-1 ~ +1の範囲に正規化させた座標を取得する
    float2 uvNormalizedView   = vClipLightView.xy / vClipLightView.w;                // [-1,1]

    // -1 ~ +1 なのでUV画像に合わせるために 0 ~ 1 に調節する
    float2 uvLightView   = uvNormalizedView * float2(0.5f, -0.5f) + 0.5f;  // [0,1]

    // DX9の半テクセル補正
    uvLightView += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);

    // ベースUVが枠外なら「影なし」
    if (any(uvLightView < 0.0f) || any(uvLightView > 1.0f))
    {
        outColor.a = 0.0f;
        return outColor;
    }

    outColor.r = nShadowColor;
    outColor.g = nShadowColor;
    if (g_writeNearCascade)
    {
        float edgeDistance = max(abs(uvNormalizedView.x), abs(uvNormalizedView.y));
        float nearCascadeWeight = saturate((1.0f - edgeDistance) / 0.15f);
        outColor.b = smoothstep(0.0f, 1.0f, nearCascadeWeight);
    }
    outColor.a = nShadowColor * g_shadowIntensity;
    return outColor;
}

bool BuildShadowCoordinatesFromGBuffer(float2 inUV,
                                       out float3 outWorldPos,
                                       out float2 outLightUV,
                                       out float outLightDepth)
{
    float2 receiverUV = inUV + float2(0.5f * g_receiverTexelW, 0.5f * g_receiverTexelH);
    float receiverMask = tex2D(samplerSceneNormal, receiverUV).a;
    float encodedDepth = tex2D(samplerReceiverDepth, receiverUV).r;
    if (receiverMask < 0.5f || encodedDepth >= 0.99999f)
    {
        outWorldPos = 0.0f;
        outLightUV = 0.0f;
        outLightDepth = 0.0f;
        return false;
    }

    float sceneEncodedDepth = tex2D(samplerSceneDepth, receiverUV).r;
    float viewDepth = lerp(g_receiverDepthNear, g_receiverDepthFar, saturate(encodedDepth));
    if (sceneEncodedDepth < 0.99999f)
    {
        viewDepth = lerp(g_sceneDepthNear, g_sceneDepthFar, saturate(sceneEncodedDepth));
    }
    float2 ndcPosition = float2(receiverUV.x * 2.0f - 1.0f,
                                1.0f - receiverUV.y * 2.0f);
    float4 viewRay = mul(float4(ndcPosition, 1.0f, 1.0f), g_matInverseProjection);
    viewRay.xyz /= viewRay.w;
    if (viewRay.z <= 0.00001f)
    {
        outWorldPos = 0.0f;
        outLightUV = 0.0f;
        outLightDepth = 0.0f;
        return false;
    }

    float3 viewPosition = viewRay.xyz * (viewDepth / viewRay.z);
    outWorldPos = mul(float4(viewPosition, 1.0f), g_matInverseView).xyz;

    float4 lightViewPosition = mul(float4(outWorldPos, 1.0f), g_matLightView);
    outLightDepth = saturate((lightViewPosition.z - g_lightNear) / (g_lightFar - g_lightNear));

    float4 lightClipPosition = mul(float4(outWorldPos, 1.0f), g_matLightViewProj);
    float2 normalizedLightPosition = lightClipPosition.xy / lightClipPosition.w;
    outLightUV = normalizedLightPosition * float2(0.5f, -0.5f) + 0.5f;
    outLightUV += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    return true;
}

float4 PS_BuildShadowFromGBuffer1(float4 inPos : POSITION0, float2 inUV : TEXCOORD0) : COLOR0
{
    float3 worldPosition;
    float2 lightUV;
    float lightDepth;
    if (!BuildShadowCoordinatesFromGBuffer(inUV, worldPosition, lightUV, lightDepth))
    {
        return 0.0f;
    }
    return BuildWriteShadowColor(worldPosition, SampleShadowAmount1(lightUV, lightDepth));
}

float4 PS_BuildShadowFromGBuffer3(float4 inPos : POSITION0, float2 inUV : TEXCOORD0) : COLOR0
{
    float3 worldPosition;
    float2 lightUV;
    float lightDepth;
    if (!BuildShadowCoordinatesFromGBuffer(inUV, worldPosition, lightUV, lightDepth))
    {
        return 0.0f;
    }
    return BuildWriteShadowColor(worldPosition, SampleShadowAmount3(lightUV, lightDepth));
}

float4 PS_BuildShadowFromGBuffer5(float4 inPos : POSITION0, float2 inUV : TEXCOORD0) : COLOR0
{
    float3 worldPosition;
    float2 lightUV;
    float lightDepth;
    if (!BuildShadowCoordinatesFromGBuffer(inUV, worldPosition, lightUV, lightDepth))
    {
        return 0.0f;
    }
    return BuildWriteShadowColor(worldPosition, SampleShadowAmount5(lightUV, lightDepth));
}

float4 PS_BuildShadowFromGBuffer7(float4 inPos : POSITION0, float2 inUV : TEXCOORD0) : COLOR0
{
    float3 worldPosition;
    float2 lightUV;
    float lightDepth;
    if (!BuildShadowCoordinatesFromGBuffer(inUV, worldPosition, lightUV, lightDepth))
    {
        return 0.0f;
    }
    return BuildWriteShadowColor(worldPosition, SampleShadowAmount7(lightUV, lightDepth));
}

float4 PS_BuildShadowFromGBuffer9(float4 inPos : POSITION0, float2 inUV : TEXCOORD0) : COLOR0
{
    float3 worldPosition;
    float2 lightUV;
    float lightDepth;
    if (!BuildShadowCoordinatesFromGBuffer(inUV, worldPosition, lightUV, lightDepth))
    {
        return 0.0f;
    }
    return BuildWriteShadowColor(worldPosition, SampleShadowAmount9(lightUV, lightDepth));
}

float4 PS_BuildShadowFromGBuffer11(float4 inPos : POSITION0, float2 inUV : TEXCOORD0) : COLOR0
{
    float3 worldPosition;
    float2 lightUV;
    float lightDepth;
    if (!BuildShadowCoordinatesFromGBuffer(inUV, worldPosition, lightUV, lightDepth))
    {
        return 0.0f;
    }
    return BuildWriteShadowColor(worldPosition, SampleShadowAmount11(lightUV, lightDepth));
}

void ApplyMeshAlphaCutoutShadow(float2 uv)
{
    if (g_useMeshAlphaCutout)
    {
        clip(tex2D(sampMeshAlpha, uv).a - g_meshAlphaClipThreshold);
    }
}

float4 PS_WriteShadow1(in float4 inPos : POSITION0,
                       in float2 inUV : TEXCOORD0,
                       in float3 inWorldPos : TEXCOORD1) : COLOR0
{
    ApplyMeshAlphaCutoutShadow(inUV);
    float4 vPosLightView = mul(float4(inWorldPos, 1.0f), g_matLightView);
    float fDepthLightView = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    fDepthLightView = saturate(fDepthLightView);
    float4 vClipLightView = mul(float4(inWorldPos, 1.0f), g_matLightViewProj);
    float2 uvNormalizedView = vClipLightView.xy / vClipLightView.w;
    float2 uvLightView = uvNormalizedView * float2(0.5f, -0.5f) + 0.5f;
    uvLightView += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    float nShadowColor = SampleShadowAmount1(uvLightView, fDepthLightView);
    return BuildWriteShadowColor(inWorldPos, nShadowColor);
}

float4 PS_WriteShadow3(in float4 inPos : POSITION0,
                       in float2 inUV : TEXCOORD0,
                       in float3 inWorldPos : TEXCOORD1) : COLOR0
{
    ApplyMeshAlphaCutoutShadow(inUV);
    float4 vPosLightView = mul(float4(inWorldPos, 1.0f), g_matLightView);
    float fDepthLightView = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    fDepthLightView = saturate(fDepthLightView);
    float4 vClipLightView = mul(float4(inWorldPos, 1.0f), g_matLightViewProj);
    float2 uvNormalizedView = vClipLightView.xy / vClipLightView.w;
    float2 uvLightView = uvNormalizedView * float2(0.5f, -0.5f) + 0.5f;
    uvLightView += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    float nShadowColor = SampleShadowAmount3(uvLightView, fDepthLightView);
    return BuildWriteShadowColor(inWorldPos, nShadowColor);
}

float4 PS_WriteShadow5(in float4 inPos : POSITION0,
                       in float2 inUV : TEXCOORD0,
                       in float3 inWorldPos : TEXCOORD1) : COLOR0
{
    ApplyMeshAlphaCutoutShadow(inUV);
    float4 vPosLightView = mul(float4(inWorldPos, 1.0f), g_matLightView);
    float fDepthLightView = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    fDepthLightView = saturate(fDepthLightView);
    float4 vClipLightView = mul(float4(inWorldPos, 1.0f), g_matLightViewProj);
    float2 uvNormalizedView = vClipLightView.xy / vClipLightView.w;
    float2 uvLightView = uvNormalizedView * float2(0.5f, -0.5f) + 0.5f;
    uvLightView += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    float nShadowColor = SampleShadowAmount5(uvLightView, fDepthLightView);
    return BuildWriteShadowColor(inWorldPos, nShadowColor);
}

float4 PS_WriteShadow7(in float4 inPos : POSITION0,
                       in float2 inUV : TEXCOORD0,
                       in float3 inWorldPos : TEXCOORD1) : COLOR0
{
    ApplyMeshAlphaCutoutShadow(inUV);
    float4 vPosLightView = mul(float4(inWorldPos, 1.0f), g_matLightView);
    float fDepthLightView = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    fDepthLightView = saturate(fDepthLightView);
    float4 vClipLightView = mul(float4(inWorldPos, 1.0f), g_matLightViewProj);
    float2 uvNormalizedView = vClipLightView.xy / vClipLightView.w;
    float2 uvLightView = uvNormalizedView * float2(0.5f, -0.5f) + 0.5f;
    uvLightView += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    float nShadowColor = SampleShadowAmount7(uvLightView, fDepthLightView);
    return BuildWriteShadowColor(inWorldPos, nShadowColor);
}

float4 PS_WriteShadow9(in float4 inPos : POSITION0,
                       in float2 inUV : TEXCOORD0,
                       in float3 inWorldPos : TEXCOORD1) : COLOR0
{
    ApplyMeshAlphaCutoutShadow(inUV);
    float4 vPosLightView = mul(float4(inWorldPos, 1.0f), g_matLightView);
    float fDepthLightView = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    fDepthLightView = saturate(fDepthLightView);
    float4 vClipLightView = mul(float4(inWorldPos, 1.0f), g_matLightViewProj);
    float2 uvNormalizedView = vClipLightView.xy / vClipLightView.w;
    float2 uvLightView = uvNormalizedView * float2(0.5f, -0.5f) + 0.5f;
    uvLightView += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    float nShadowColor = SampleShadowAmount9(uvLightView, fDepthLightView);
    return BuildWriteShadowColor(inWorldPos, nShadowColor);
}

float4 PS_WriteShadow11(in float4 inPos : POSITION0,
                        in float2 inUV : TEXCOORD0,
                        in float3 inWorldPos : TEXCOORD1) : COLOR0
{
    ApplyMeshAlphaCutoutShadow(inUV);
    float4 vPosLightView = mul(float4(inWorldPos, 1.0f), g_matLightView);
    float fDepthLightView = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    fDepthLightView = saturate(fDepthLightView);
    float4 vClipLightView = mul(float4(inWorldPos, 1.0f), g_matLightViewProj);
    float2 uvNormalizedView = vClipLightView.xy / vClipLightView.w;
    float2 uvLightView = uvNormalizedView * float2(0.5f, -0.5f) + 0.5f;
    uvLightView += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    float nShadowColor = SampleShadowAmount11(uvLightView, fDepthLightView);
    return BuildWriteShadowColor(inWorldPos, nShadowColor);
}

float4 PS_WriteShadow(in float4 inPos : POSITION0,
                      in float2 inUV : TEXCOORD0,
                      in float3 inWorldPos : TEXCOORD1) : COLOR0
{
    return PS_WriteShadow11(inPos, inUV, inWorldPos);
}

//-------------------------------------------------------------------------
// Technique 3
//-------------------------------------------------------------------------

void VS_Composite(in  float4 inPos  : POSITION,
                  in  float2 inUV   : TEXCOORD0,

                  out float4 outPos : POSITION,
                  out float2 outUV  : TEXCOORD0)
{
    outPos = inPos;
    outUV = inUV;
}

// 2枚の画像を線形補間で合成する
bool ReconstructDirectShadowWorldPosition(float2 uv,
                                          float receiverMask,
                                          float sceneEncodedDepth,
                                          out float3 worldPosition)
{
    float receiverEncodedDepth = tex2Dlod(samplerReceiverDepth, float4(uv, 0.0f, 0.0f)).r;
    if (receiverMask < 0.5f || receiverEncodedDepth >= 0.99999f)
    {
        worldPosition = 0.0f;
        return false;
    }

    float viewDepth = lerp(g_receiverDepthNear,
                           g_receiverDepthFar,
                           saturate(receiverEncodedDepth));
    if (sceneEncodedDepth < 0.99999f)
    {
        viewDepth = lerp(g_sceneDepthNear,
                         g_sceneDepthFar,
                         saturate(sceneEncodedDepth));
    }

    float2 ndcPosition = float2(uv.x * 2.0f - 1.0f,
                                1.0f - uv.y * 2.0f);
    float4 viewRay = mul(float4(ndcPosition, 1.0f, 1.0f), g_matInverseProjection);
    viewRay.xyz /= viewRay.w;
    if (viewRay.z <= 0.00001f)
    {
        worldPosition = 0.0f;
        return false;
    }

    float3 viewPosition = viewRay.xyz * (viewDepth / viewRay.z);
    worldPosition = mul(float4(viewPosition, 1.0f), g_matInverseView).xyz;
    return true;
}

bool BuildNearLightCoordinates(float3 worldPosition,
                               out float2 lightUV,
                               out float lightDepth,
                               out float cascadeWeight)
{
    float4 lightViewPosition = mul(float4(worldPosition, 1.0f), g_matLightView);
    lightDepth = saturate((lightViewPosition.z - g_lightNear) / (g_lightFar - g_lightNear));
    float4 lightClipPosition = mul(float4(worldPosition, 1.0f), g_matLightViewProj);
    float2 normalizedLightPosition = lightClipPosition.xy / lightClipPosition.w;
    lightUV = normalizedLightPosition * float2(0.5f, -0.5f) + 0.5f;
    lightUV += float2(0.5f * g_shadowTexelW, 0.5f * g_shadowTexelH);
    float edgeDistance = max(abs(normalizedLightPosition.x), abs(normalizedLightPosition.y));
    cascadeWeight = smoothstep(0.0f, 1.0f, saturate((1.0f - edgeDistance) / 0.15f));
    return !any(lightUV < 0.0f) && !any(lightUV > 1.0f);
}

bool BuildFarLightCoordinates(float3 worldPosition,
                              out float2 lightUV,
                              out float lightDepth)
{
    float4 lightViewPosition = mul(float4(worldPosition, 1.0f), g_matLightViewFar);
    lightDepth = saturate((lightViewPosition.z - g_lightNearFarCascade) /
                          (g_lightFarFarCascade - g_lightNearFarCascade));
    float4 lightClipPosition = mul(float4(worldPosition, 1.0f), g_matLightViewProjFar);
    float2 normalizedLightPosition = lightClipPosition.xy / lightClipPosition.w;
    lightUV = normalizedLightPosition * float2(0.5f, -0.5f) + 0.5f;
    lightUV += float2(0.5f * g_shadowFarTexelW, 0.5f * g_shadowFarTexelH);
    return !any(lightUV < 0.0f) && !any(lightUV > 1.0f);
}

#define DEFINE_NEAR_SHADOW_FIXED(FUNCTION_NAME, RADIUS) \
float FUNCTION_NAME(float2 lightUV, float lightDepth) \
{ \
    float shadowSum = 0.0f; \
    float sampleCount = 0.0f; \
    [loop] \
    for (int y = -(RADIUS); y <= (RADIUS); ++y) \
    { \
        [loop] \
        for (int x = -(RADIUS); x <= (RADIUS); ++x) \
        { \
            float2 sampleUV = lightUV + float2((float)x * g_shadowTexelW, \
                                               (float)y * g_shadowTexelH); \
            if (!any(sampleUV < 0.0f) && !any(sampleUV > 1.0f)) \
            { \
                float shadowDepth = tex2Dlod(samplerLightZ, float4(sampleUV, 0.0f, 0.0f)).r; \
                if (shadowDepth < (lightDepth - g_shadowBias)) \
                { \
                    shadowSum += 1.0f; \
                } \
                sampleCount += 1.0f; \
            } \
        } \
    } \
    return FinalizeShadowAmount(shadowSum, sampleCount); \
}

#define DEFINE_FAR_SHADOW_FIXED(FUNCTION_NAME, RADIUS) \
float FUNCTION_NAME(float2 lightUV, float lightDepth) \
{ \
    float shadowSum = 0.0f; \
    float sampleCount = 0.0f; \
    [loop] \
    for (int y = -(RADIUS); y <= (RADIUS); ++y) \
    { \
        [loop] \
        for (int x = -(RADIUS); x <= (RADIUS); ++x) \
        { \
            float2 sampleUV = lightUV + float2((float)x * g_shadowFarTexelW, \
                                               (float)y * g_shadowFarTexelH); \
            if (!any(sampleUV < 0.0f) && !any(sampleUV > 1.0f)) \
            { \
                float shadowDepth = tex2Dlod(samplerLightZFar, float4(sampleUV, 0.0f, 0.0f)).r; \
                if (shadowDepth < (lightDepth - g_shadowBiasFar)) \
                { \
                    shadowSum += 1.0f; \
                } \
                sampleCount += 1.0f; \
            } \
        } \
    } \
    return FinalizeShadowAmount(shadowSum, sampleCount); \
}

float SampleNearShadowFixed1(float2 lightUV, float lightDepth)
{
    float shadowDepth = tex2Dlod(samplerLightZ, float4(lightUV, 0.0f, 0.0f)).r;
    if (shadowDepth < (lightDepth - g_shadowBias))
    {
        return 1.0f;
    }
    return 0.0f;
}

float SampleFarShadowFixed1(float2 lightUV, float lightDepth)
{
    float shadowDepth = tex2Dlod(samplerLightZFar, float4(lightUV, 0.0f, 0.0f)).r;
    if (shadowDepth < (lightDepth - g_shadowBiasFar))
    {
        return 1.0f;
    }
    return 0.0f;
}

DEFINE_NEAR_SHADOW_FIXED(SampleNearShadowFixed3, 1)
DEFINE_NEAR_SHADOW_FIXED(SampleNearShadowFixed5, 2)
DEFINE_NEAR_SHADOW_FIXED(SampleNearShadowFixed7, 3)
DEFINE_NEAR_SHADOW_FIXED(SampleNearShadowFixed9, 4)
DEFINE_NEAR_SHADOW_FIXED(SampleNearShadowFixed11, 5)
DEFINE_FAR_SHADOW_FIXED(SampleFarShadowFixed3, 1)
DEFINE_FAR_SHADOW_FIXED(SampleFarShadowFixed5, 2)
DEFINE_FAR_SHADOW_FIXED(SampleFarShadowFixed7, 3)
DEFINE_FAR_SHADOW_FIXED(SampleFarShadowFixed9, 4)
DEFINE_FAR_SHADOW_FIXED(SampleFarShadowFixed11, 5)

#define DEFINE_DIRECT_SHADOW_EVALUATOR(FUNCTION_NAME, NEAR_SAMPLE_FUNCTION, FAR_SAMPLE_FUNCTION) \
float FUNCTION_NAME(float2 uv, float receiverMask, float sceneEncodedDepth) \
{ \
    float3 worldPosition; \
    if (!ReconstructDirectShadowWorldPosition(uv, receiverMask, sceneEncodedDepth, worldPosition)) \
    { \
        return 0.0f; \
    } \
    float2 nearLightUV; \
    float nearLightDepth; \
    float nearCascadeWeight; \
    bool isInsideNearCascade = BuildNearLightCoordinates(worldPosition, \
                                                         nearLightUV, \
                                                         nearLightDepth, \
                                                         nearCascadeWeight); \
    float nearShadow = 0.0f; \
    if (isInsideNearCascade) \
    { \
        nearShadow = NEAR_SAMPLE_FUNCTION(nearLightUV, nearLightDepth); \
    } \
    if (!g_farCascadeEnabled) \
    { \
        return nearShadow; \
    } \
    float2 farLightUV; \
    float farLightDepth; \
    float farShadow = 0.0f; \
    if (BuildFarLightCoordinates(worldPosition, farLightUV, farLightDepth)) \
    { \
        farShadow = FAR_SAMPLE_FUNCTION(farLightUV, farLightDepth); \
    } \
    if (!isInsideNearCascade) \
    { \
        nearCascadeWeight = 0.0f; \
    } \
    return lerp(farShadow, nearShadow, saturate(nearCascadeWeight)); \
}

DEFINE_DIRECT_SHADOW_EVALUATOR(EvaluateDirectShadowFixed1, SampleNearShadowFixed1, SampleFarShadowFixed1)
DEFINE_DIRECT_SHADOW_EVALUATOR(EvaluateDirectShadowFixed3, SampleNearShadowFixed3, SampleFarShadowFixed3)
DEFINE_DIRECT_SHADOW_EVALUATOR(EvaluateDirectShadowFixed5, SampleNearShadowFixed5, SampleFarShadowFixed5)
DEFINE_DIRECT_SHADOW_EVALUATOR(EvaluateDirectShadowFixed7, SampleNearShadowFixed7, SampleFarShadowFixed7)
DEFINE_DIRECT_SHADOW_EVALUATOR(EvaluateDirectShadowFixed9, SampleNearShadowFixed9, SampleFarShadowFixed9)
DEFINE_DIRECT_SHADOW_EVALUATOR(EvaluateDirectShadowFixed11, SampleNearShadowFixed11, SampleFarShadowFixed11)

#define DEFINE_DIRECT_COMPOSITE_ONE(FUNCTION_NAME, EVALUATE_FUNCTION) \
float4 FUNCTION_NAME(float4 inPos : POSITION, float2 inUV : TEXCOORD0) : COLOR0 \
{ \
    float2 uv = inUV + float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH); \
    float4 baseColor = tex2D(samplerBase, uv); \
    float sceneDepth = tex2D(samplerSceneDepth, uv).r; \
    float4 encodedNormal = tex2D(samplerSceneNormal, uv); \
    float shadowPresence = EVALUATE_FUNCTION(uv, encodedNormal.a, sceneDepth); \
    float shadowAmount = saturate(shadowPresence * g_shadowIntensity); \
    float3 shadowedColor = lerp(baseColor.rgb, float3(0.0f, 0.0f, 0.0f), shadowAmount); \
    float saturationAmount = lerp(1.0f, 1.0f + g_shadowSaturationBoost, saturate(shadowPresence)); \
    return float4(IncreaseSaturation(shadowedColor, saturationAmount), baseColor.a); \
}

#define DEFINE_DIRECT_COMPOSITE_FILTERED(FUNCTION_NAME, EVALUATE_FUNCTION, RADIUS) \
float4 FUNCTION_NAME(float4 inPos : POSITION, float2 inUV : TEXCOORD0) : COLOR0 \
{ \
    float2 uv = inUV + float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH); \
    float4 baseColor = tex2D(samplerBase, uv); \
    float centerDepth = tex2D(samplerSceneDepth, uv).r; \
    float4 centerEncodedNormal = tex2D(samplerSceneNormal, uv); \
    float3 centerNormalValue = centerEncodedNormal.rgb * 2.0f - 1.0f; \
    float3 centerNormal = centerNormalValue * rsqrt(max(dot(centerNormalValue, centerNormalValue), 0.00000001f)); \
    float shadowSum = 0.0f; \
    float sampleCount = 0.0f; \
    [loop] \
    for (int y = -(RADIUS); y <= (RADIUS); ++y) \
    { \
        [loop] \
        for (int x = -(RADIUS); x <= (RADIUS); ++x) \
        { \
            float2 sampleUV = uv + float2((float)x * g_compositeTexelW, \
                                          (float)y * g_compositeTexelH); \
            float sampleDepth = tex2Dlod(samplerSceneDepth, float4(sampleUV, 0.0f, 0.0f)).r; \
            float4 sampleEncodedNormal = tex2Dlod(samplerSceneNormal, float4(sampleUV, 0.0f, 0.0f)); \
            float3 sampleNormalValue = sampleEncodedNormal.rgb * 2.0f - 1.0f; \
            float3 sampleNormal = sampleNormalValue * rsqrt(max(dot(sampleNormalValue, sampleNormalValue), 0.00000001f)); \
            float insideWeight = step(0.0f, sampleUV.x) * step(sampleUV.x, 1.0f); \
            insideWeight *= step(0.0f, sampleUV.y) * step(sampleUV.y, 1.0f); \
            float depthWeight = 1.0f - step(g_edgeDepthThreshold, abs(sampleDepth - centerDepth)); \
            float normalWeight = step(g_edgeNormalThreshold, dot(centerNormal, sampleNormal)); \
            float sampleWeight = insideWeight * depthWeight * normalWeight; \
            shadowSum += EVALUATE_FUNCTION(sampleUV, sampleEncodedNormal.a, sampleDepth) * sampleWeight; \
            sampleCount += sampleWeight; \
        } \
    } \
    float shadowPresence = FinalizeShadowAmount(shadowSum, sampleCount); \
    float shadowAmount = saturate(shadowPresence * g_shadowIntensity); \
    float3 shadowedColor = lerp(baseColor.rgb, float3(0.0f, 0.0f, 0.0f), shadowAmount); \
    float saturationAmount = lerp(1.0f, 1.0f + g_shadowSaturationBoost, saturate(shadowPresence)); \
    return float4(IncreaseSaturation(shadowedColor, saturationAmount), baseColor.a); \
}

DEFINE_DIRECT_COMPOSITE_ONE(PS_DirectP1C1, EvaluateDirectShadowFixed1)
DEFINE_DIRECT_COMPOSITE_ONE(PS_DirectP3C1, EvaluateDirectShadowFixed3)
DEFINE_DIRECT_COMPOSITE_ONE(PS_DirectP5C1, EvaluateDirectShadowFixed5)
DEFINE_DIRECT_COMPOSITE_ONE(PS_DirectP7C1, EvaluateDirectShadowFixed7)
DEFINE_DIRECT_COMPOSITE_ONE(PS_DirectP9C1, EvaluateDirectShadowFixed9)
DEFINE_DIRECT_COMPOSITE_ONE(PS_DirectP11C1, EvaluateDirectShadowFixed11)

#define DEFINE_DIRECT_COMPOSITE_SET(PCF_NAME, EVALUATE_FUNCTION) \
DEFINE_DIRECT_COMPOSITE_FILTERED(PS_Direct##PCF_NAME##C3, EVALUATE_FUNCTION, 1) \
DEFINE_DIRECT_COMPOSITE_FILTERED(PS_Direct##PCF_NAME##C5, EVALUATE_FUNCTION, 2) \
DEFINE_DIRECT_COMPOSITE_FILTERED(PS_Direct##PCF_NAME##C7, EVALUATE_FUNCTION, 3) \
DEFINE_DIRECT_COMPOSITE_FILTERED(PS_Direct##PCF_NAME##C9, EVALUATE_FUNCTION, 4) \
DEFINE_DIRECT_COMPOSITE_FILTERED(PS_Direct##PCF_NAME##C11, EVALUATE_FUNCTION, 5)

DEFINE_DIRECT_COMPOSITE_SET(P1, EvaluateDirectShadowFixed1)
DEFINE_DIRECT_COMPOSITE_SET(P3, EvaluateDirectShadowFixed3)
DEFINE_DIRECT_COMPOSITE_SET(P5, EvaluateDirectShadowFixed5)
DEFINE_DIRECT_COMPOSITE_SET(P7, EvaluateDirectShadowFixed7)
DEFINE_DIRECT_COMPOSITE_SET(P9, EvaluateDirectShadowFixed9)
DEFINE_DIRECT_COMPOSITE_SET(P11, EvaluateDirectShadowFixed11)

float SampleNearShadowDirect(float2 lightUV, float lightDepth)
{
    int radius = clamp((g_shadowPcfTapCount - 1) / 2, 0, 5);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;
    [loop]
    for (int y = -radius; y <= radius; ++y)
    {
        [loop]
        for (int x = -radius; x <= radius; ++x)
        {
            float2 sampleUV = lightUV + float2((float)x * g_shadowTexelW,
                                               (float)y * g_shadowTexelH);
            if (any(sampleUV < 0.0f) || any(sampleUV > 1.0f))
            {
                continue;
            }
            float shadowDepth = tex2Dlod(samplerLightZ, float4(sampleUV, 0.0f, 0.0f)).r;
            if (shadowDepth < (lightDepth - g_shadowBias))
            {
                shadowSum += 1.0f;
            }
            sampleCount += 1.0f;
        }
    }
    return FinalizeShadowAmount(shadowSum, sampleCount);
}

float SampleFarShadowDirect(float2 lightUV, float lightDepth)
{
    int radius = clamp((g_shadowPcfTapCount - 1) / 2, 0, 5);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;
    [loop]
    for (int y = -radius; y <= radius; ++y)
    {
        [loop]
        for (int x = -radius; x <= radius; ++x)
        {
            float2 sampleUV = lightUV + float2((float)x * g_shadowFarTexelW,
                                               (float)y * g_shadowFarTexelH);
            if (any(sampleUV < 0.0f) || any(sampleUV > 1.0f))
            {
                continue;
            }
            float shadowDepth = tex2Dlod(samplerLightZFar, float4(sampleUV, 0.0f, 0.0f)).r;
            if (shadowDepth < (lightDepth - g_shadowBiasFar))
            {
                shadowSum += 1.0f;
            }
            sampleCount += 1.0f;
        }
    }
    return FinalizeShadowAmount(shadowSum, sampleCount);
}

float EvaluateDirectShadow(float2 uv, float receiverMask, float sceneEncodedDepth)
{
    float3 worldPosition;
    if (!ReconstructDirectShadowWorldPosition(uv,
                                              receiverMask,
                                              sceneEncodedDepth,
                                              worldPosition))
    {
        return 0.0f;
    }

    float2 nearLightUV;
    float nearLightDepth;
    float nearCascadeWeight;
    bool isInsideNearCascade = BuildNearLightCoordinates(worldPosition,
                                                         nearLightUV,
                                                         nearLightDepth,
                                                         nearCascadeWeight);
    float nearShadow = 0.0f;
    if (isInsideNearCascade)
    {
        nearShadow = SampleNearShadowDirect(nearLightUV, nearLightDepth);
    }

    if (!g_farCascadeEnabled)
    {
        return nearShadow;
    }

    float2 farLightUV;
    float farLightDepth;
    float farShadow = 0.0f;
    if (BuildFarLightCoordinates(worldPosition, farLightUV, farLightDepth))
    {
        farShadow = SampleFarShadowDirect(farLightUV, farLightDepth);
    }

    if (!isInsideNearCascade)
    {
        nearCascadeWeight = 0.0f;
    }
    return lerp(farShadow, nearShadow, saturate(nearCascadeWeight));
}

float SampleFarShadow1Direct(float2 lightUV, float lightDepth)
{
    float shadowDepth = tex2Dlod(samplerLightZFar, float4(lightUV, 0.0f, 0.0f)).r;
    if (shadowDepth < (lightDepth - g_shadowBiasFar))
    {
        return 1.0f;
    }
    return 0.0f;
}

float EvaluateDirectShadow1(float2 uv, float receiverMask, float sceneEncodedDepth)
{
    float3 worldPosition;
    if (!ReconstructDirectShadowWorldPosition(uv,
                                              receiverMask,
                                              sceneEncodedDepth,
                                              worldPosition))
    {
        return 0.0f;
    }

    float2 nearLightUV;
    float nearLightDepth;
    float nearCascadeWeight;
    bool isInsideNearCascade = BuildNearLightCoordinates(worldPosition,
                                                         nearLightUV,
                                                         nearLightDepth,
                                                         nearCascadeWeight);
    float nearShadow = 0.0f;
    if (isInsideNearCascade)
    {
        nearShadow = SampleShadowAmount1(nearLightUV, nearLightDepth);
    }

    if (!g_farCascadeEnabled)
    {
        return nearShadow;
    }

    float2 farLightUV;
    float farLightDepth;
    float farShadow = 0.0f;
    if (BuildFarLightCoordinates(worldPosition, farLightUV, farLightDepth))
    {
        farShadow = SampleFarShadow1Direct(farLightUV, farLightDepth);
    }

    if (!isInsideNearCascade)
    {
        nearCascadeWeight = 0.0f;
    }
    return lerp(farShadow, nearShadow, saturate(nearCascadeWeight));
}

float4 PS_DirectComposite1(float4 inPos : POSITION, float2 inUV : TEXCOORD0) : COLOR0
{
    float2 uv = inUV + float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH);
    float4 baseColor = tex2D(samplerBase, uv);
    float sceneDepth = tex2D(samplerSceneDepth, uv).r;
    float4 encodedNormal = tex2D(samplerSceneNormal, uv);
    float shadowPresence = EvaluateDirectShadow1(uv, encodedNormal.a, sceneDepth);
    float shadowAmount = saturate(shadowPresence * g_shadowIntensity);
    float3 shadowedColor = lerp(baseColor.rgb, float3(0.0f, 0.0f, 0.0f), shadowAmount);
    float saturationAmount = lerp(1.0f,
                                  1.0f + g_shadowSaturationBoost,
                                  saturate(shadowPresence));
    return float4(IncreaseSaturation(shadowedColor, saturationAmount), baseColor.a);
}

float4 PS_DirectComposite(float4 inPos : POSITION, float2 inUV : TEXCOORD0) : COLOR0
{
    float2 uv = inUV + float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH);
    float4 baseColor = tex2D(samplerBase, uv);
    float centerDepth = tex2D(samplerSceneDepth, uv).r;
    float4 centerEncodedNormal = tex2D(samplerSceneNormal, uv);
    float3 centerNormal = DecodeWorldNormal(centerEncodedNormal.rgb);

    int radius = clamp((g_shadowCompositeTapCount - 1) / 2, 0, 5);
    float shadowSum = 0.0f;
    float sampleCount = 0.0f;
    [loop]
    for (int y = -radius; y <= radius; ++y)
    {
        [loop]
        for (int x = -radius; x <= radius; ++x)
        {
            float2 sampleUV = uv + float2((float)x * g_compositeTexelW,
                                          (float)y * g_compositeTexelH);
            if (any(sampleUV < 0.0f) || any(sampleUV > 1.0f))
            {
                continue;
            }

            float sampleDepth = tex2Dlod(samplerSceneDepth, float4(sampleUV, 0.0f, 0.0f)).r;
            if (abs(sampleDepth - centerDepth) > g_edgeDepthThreshold)
            {
                continue;
            }

            float4 sampleEncodedNormal = tex2Dlod(samplerSceneNormal,
                                                  float4(sampleUV, 0.0f, 0.0f));
            float3 sampleNormal = DecodeWorldNormal(sampleEncodedNormal.rgb);
            if (dot(centerNormal, sampleNormal) < g_edgeNormalThreshold)
            {
                continue;
            }

            shadowSum += EvaluateDirectShadow(sampleUV,
                                              sampleEncodedNormal.a,
                                              sampleDepth);
            sampleCount += 1.0f;
        }
    }

    float shadowPresence = FinalizeShadowAmount(shadowSum, sampleCount);
    float shadowAmount = saturate(shadowPresence * g_shadowIntensity);
    float3 shadowedColor = lerp(baseColor.rgb, float3(0.0f, 0.0f, 0.0f), shadowAmount);
    float saturationAmount = lerp(1.0f,
                                  1.0f + g_shadowSaturationBoost,
                                  saturate(shadowPresence));
    return float4(IncreaseSaturation(shadowedColor, saturationAmount), baseColor.a);
}

float4 SampleCombinedShadow(float2 uv)
{
    float4 nearShadow = tex2D(samplerShadow, uv);
    if (!g_farCascadeEnabled)
    {
        return nearShadow;
    }

    float2 farUv = uv;
    farUv -= float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH);
    farUv += float2(0.5f * g_compositeFarTexelW, 0.5f * g_compositeFarTexelH);

    float4 farShadow = tex2D(samplerShadowFar, farUv);
    float nearCascadeWeight = saturate(nearShadow.b);
    return lerp(farShadow, nearShadow, nearCascadeWeight);
}

void PS_Composite(in float4 inPos     : POSITION,
                  in float2 inUV      : TEXCOORD0,

                  out float4 outColor : COLOR0)
{
    float2 uv = inUV + float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH);
    float2 pixelCoord = floor(uv / float2(g_compositeTexelW, g_compositeTexelH));
    float4 vBaseColor = tex2D(samplerBase, uv);
    float4 vCenterShadowColor = SampleCombinedShadow(uv);
    float centerDepth = tex2D(samplerSceneDepth, uv).r;
    float3 centerNormal = DecodeWorldNormal(tex2D(samplerSceneNormal, uv).rgb);

    float4 vShadowColorSum = 0.0f;
    float totalWeight = 0.0f;
    const int FILTER_RADIUS_MAX = 5;
    int filterRadius = (g_shadowCompositeTapCount - 1) / 2;
    filterRadius = clamp(filterRadius, 0, FILTER_RADIUS_MAX);

    for (int y = -FILTER_RADIUS_MAX; y <= FILTER_RADIUS_MAX; ++y)
    {
        for (int x = -FILTER_RADIUS_MAX; x <= FILTER_RADIUS_MAX; ++x)
        {
            if (abs(x) > filterRadius || abs(y) > filterRadius)
            {
                continue;
            }

            float2 sampleUv = uv + float2((float)x * g_compositeTexelW, (float)y * g_compositeTexelH);

            if (any(sampleUv < 0.0f) || any(sampleUv > 1.0f))
            {
                continue;
            }

            float sampleDepth = tex2D(samplerSceneDepth, sampleUv).r;
            if (abs(sampleDepth - centerDepth) > g_edgeDepthThreshold)
            {
                continue;
            }

            float3 sampleNormal = DecodeWorldNormal(tex2D(samplerSceneNormal, sampleUv).rgb);
            if (dot(centerNormal, sampleNormal) < g_edgeNormalThreshold)
            {
                continue;
            }

            float weight = 1.0f;
            vShadowColorSum += SampleCombinedShadow(sampleUv) * weight;
            totalWeight += weight;
        }
    }

    float4 vShadowColor = vCenterShadowColor;
    if (totalWeight > 0.0f)
    {
        vShadowColor = vShadowColorSum / totalWeight;
    }

    float4 result = float4(0, 0, 0, 0);
    float shadowPresence = saturate(vShadowColor.r);
    float shadowAmount = saturate(vShadowColor.a);
    float3 shadowedColor = lerp(vBaseColor.rgb, float3(0.0f, 0.0f, 0.0f), shadowAmount);
    float saturationAmount = lerp(1.0f, 1.0f + g_shadowSaturationBoost, shadowPresence);
    result.rgb = IncreaseSaturation(shadowedColor, saturationAmount);

    if (false)
    {
        if (fmod(pixelCoord.x, 5.0f) == 0.0f || fmod(pixelCoord.y, 5.0f) == 0.0f)
        {
            result.rgb = float3(0.0f, 1.0f, 0.0f);
        }
    }

    result.a = vBaseColor.a;
    outColor = result;
}

void AccumulateCompositeSampleFixed(float2 uv,
                                    float centerDepth,
                                    float3 centerNormal,
                                    int x,
                                    int y,
                                    inout float4 vShadowColorSum,
                                    inout float totalWeight)
{
    float2 sampleUv = uv + float2((float)x * g_compositeTexelW, (float)y * g_compositeTexelH);

    if (any(sampleUv < 0.0f) || any(sampleUv > 1.0f))
    {
        return;
    }

    float sampleDepth = tex2D(samplerSceneDepth, sampleUv).r;
    if (abs(sampleDepth - centerDepth) > g_edgeDepthThreshold)
    {
        return;
    }

    float3 sampleNormal = DecodeWorldNormal(tex2D(samplerSceneNormal, sampleUv).rgb);
    if (dot(centerNormal, sampleNormal) < g_edgeNormalThreshold)
    {
        return;
    }

    float weight = 1.0f;
    vShadowColorSum += SampleCombinedShadow(sampleUv) * weight;
    totalWeight += weight;
}

float4 FinalizeCompositeColorFixed(float2 uv,
                                   float2 pixelCoord,
                                   float4 vBaseColor,
                                   float4 vCenterShadowColor,
                                   float4 vShadowColorSum,
                                   float totalWeight)
{
    float4 vShadowColor = vCenterShadowColor;
    if (totalWeight > 0.0f)
    {
        vShadowColor = vShadowColorSum / totalWeight;
    }

    float4 result = float4(0, 0, 0, 0);
    float shadowPresence = saturate(vShadowColor.r);
    float shadowAmount = saturate(vShadowColor.a);
    float3 shadowedColor = lerp(vBaseColor.rgb, float3(0.0f, 0.0f, 0.0f), shadowAmount);
    float saturationAmount = lerp(1.0f, 1.0f + g_shadowSaturationBoost, shadowPresence);
    result.rgb = IncreaseSaturation(shadowedColor, saturationAmount);

    if (false)
    {
        if (fmod(pixelCoord.x, 5.0f) == 0.0f || fmod(pixelCoord.y, 5.0f) == 0.0f)
        {
            result.rgb = float3(0.0f, 1.0f, 0.0f);
        }
    }

    result.a = vBaseColor.a;
    return result;
}

float4 BuildCompositeColorFixed(float2 inUV, int filterRadius)
{
    float2 uv = inUV + float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH);
    float2 pixelCoord = floor(uv / float2(g_compositeTexelW, g_compositeTexelH));
    float4 vBaseColor = tex2D(samplerBase, uv);
    float4 vCenterShadowColor = SampleCombinedShadow(uv);
    float centerDepth = tex2D(samplerSceneDepth, uv).r;
    float3 centerNormal = DecodeWorldNormal(tex2D(samplerSceneNormal, uv).rgb);
    float4 vShadowColorSum = 0.0f;
    float totalWeight = 0.0f;

    if (filterRadius == 0)
    {
        AccumulateCompositeSampleFixed(uv, centerDepth, centerNormal, 0, 0, vShadowColorSum, totalWeight);
    }
    else if (filterRadius == 1)
    {
        [loop]
        for (int y = -1; y <= 1; ++y)
        {
            [loop]
            for (int x = -1; x <= 1; ++x)
            {
                AccumulateCompositeSampleFixed(uv, centerDepth, centerNormal, x, y, vShadowColorSum, totalWeight);
            }
        }
    }
    else if (filterRadius == 2)
    {
        [loop]
        for (int y = -2; y <= 2; ++y)
        {
            [loop]
            for (int x = -2; x <= 2; ++x)
            {
                AccumulateCompositeSampleFixed(uv, centerDepth, centerNormal, x, y, vShadowColorSum, totalWeight);
            }
        }
    }
    else if (filterRadius == 3)
    {
        [loop]
        for (int y = -3; y <= 3; ++y)
        {
            [loop]
            for (int x = -3; x <= 3; ++x)
            {
                AccumulateCompositeSampleFixed(uv, centerDepth, centerNormal, x, y, vShadowColorSum, totalWeight);
            }
        }
    }
    else if (filterRadius == 4)
    {
        [loop]
        for (int y = -4; y <= 4; ++y)
        {
            [loop]
            for (int x = -4; x <= 4; ++x)
            {
                AccumulateCompositeSampleFixed(uv, centerDepth, centerNormal, x, y, vShadowColorSum, totalWeight);
            }
        }
    }
    else
    {
        [loop]
        for (int y = -5; y <= 5; ++y)
        {
            [loop]
            for (int x = -5; x <= 5; ++x)
            {
                AccumulateCompositeSampleFixed(uv, centerDepth, centerNormal, x, y, vShadowColorSum, totalWeight);
            }
        }
    }

    return FinalizeCompositeColorFixed(uv,
                                       pixelCoord,
                                       vBaseColor,
                                       vCenterShadowColor,
                                       vShadowColorSum,
                                       totalWeight);
}

float4 PS_Composite1(in float4 inPos : POSITION,
                     in float2 inUV : TEXCOORD0) : COLOR0
{
    return BuildCompositeColorFixed(inUV, 0);
}

float4 PS_Composite3(in float4 inPos : POSITION,
                     in float2 inUV : TEXCOORD0) : COLOR0
{
    return BuildCompositeColorFixed(inUV, 1);
}

float4 PS_Composite5(in float4 inPos : POSITION,
                     in float2 inUV : TEXCOORD0) : COLOR0
{
    return BuildCompositeColorFixed(inUV, 2);
}

float4 PS_Composite7(in float4 inPos : POSITION,
                     in float2 inUV : TEXCOORD0) : COLOR0
{
    return BuildCompositeColorFixed(inUV, 3);
}

float4 PS_Composite9(in float4 inPos : POSITION,
                     in float2 inUV : TEXCOORD0) : COLOR0
{
    return BuildCompositeColorFixed(inUV, 4);
}

float4 PS_Composite11(in float4 inPos : POSITION,
                      in float2 inUV : TEXCOORD0) : COLOR0
{
    return BuildCompositeColorFixed(inUV, 5);
}

float4 PS_DebugLightZ(in float4 inPos : POSITION,
                      in float2 inUV  : TEXCOORD0) : COLOR0
{
    float depth = tex2D(samplerLightZ, inUV).r;
    return float4(depth, depth, depth, 1.0f);
}

// 光源から見た深度を描画するテクニック
technique TechniqueDepthFromLight
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = TRUE;
        ZWriteEnable = TRUE;
        ZFunc        = LESSEQUAL;
        VertexShader = compile vs_3_0 VS_DepthFromLight();
        PixelShader  = compile ps_3_0 PS_DepthFromLight();
    }
}

technique TechniqueDepthFromLightSkin
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = TRUE;
        ZWriteEnable = TRUE;
        ZFunc        = LESSEQUAL;
        VertexShader = (vsDepthSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_DepthFromLight();
    }
}

technique TechniqueShadowOccluderInstancing
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = TRUE;
        ZWriteEnable     = TRUE;
        AlphaBlendEnable = FALSE;
        VertexShader     = compile vs_3_0 VS_ShadowOccluderInstancing();
        PixelShader      = compile ps_3_0 PS_ShadowOccluderInstancing();
    }
}

// 光源から見た深度画像とカメラから見たワールド座標を使って、影を描画するテクニック
technique TechniqueWriteShadow
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow();
    }
}

technique TechniqueWriteShadowSkin
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow();
    }
}

// 二つの画像を合成するテクニック
technique TechniqueBuildShadowFromGBuffer1
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = FALSE;
        ZWriteEnable = FALSE;
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_BuildShadowFromGBuffer1();
    }
}

technique TechniqueBuildShadowFromGBuffer3
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = FALSE;
        ZWriteEnable = FALSE;
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_BuildShadowFromGBuffer3();
    }
}

technique TechniqueBuildShadowFromGBuffer5
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = FALSE;
        ZWriteEnable = FALSE;
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_BuildShadowFromGBuffer5();
    }
}

technique TechniqueBuildShadowFromGBuffer7
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = FALSE;
        ZWriteEnable = FALSE;
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_BuildShadowFromGBuffer7();
    }
}

technique TechniqueBuildShadowFromGBuffer9
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = FALSE;
        ZWriteEnable = FALSE;
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_BuildShadowFromGBuffer9();
    }
}

technique TechniqueBuildShadowFromGBuffer11
{
    pass P0
    {
        CullMode     = NONE;
        ZEnable      = FALSE;
        ZWriteEnable = FALSE;
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_BuildShadowFromGBuffer11();
    }
}

technique TechniqueDirectComposite1
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = FALSE;
        ZWriteEnable     = FALSE;
        AlphaBlendEnable = FALSE;
        VertexShader     = compile vs_3_0 VS_Composite();
        PixelShader      = compile ps_3_0 PS_DirectComposite1();
    }
}

#define DEFINE_DIRECT_TECHNIQUE(TECHNIQUE_NAME, PIXEL_SHADER_NAME) \
technique TECHNIQUE_NAME \
{ \
    pass P0 \
    { \
        CullMode         = NONE; \
        ZEnable          = FALSE; \
        ZWriteEnable     = FALSE; \
        AlphaBlendEnable = FALSE; \
        VertexShader     = compile vs_3_0 VS_Composite(); \
        PixelShader      = compile ps_3_0 PIXEL_SHADER_NAME(); \
    } \
}

DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP1C1, PS_DirectP1C1)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP1C3, PS_DirectP1C3)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP1C5, PS_DirectP1C5)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP1C7, PS_DirectP1C7)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP1C9, PS_DirectP1C9)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP1C11, PS_DirectP1C11)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP3C1, PS_DirectP3C1)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP3C3, PS_DirectP3C3)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP3C5, PS_DirectP3C5)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP3C7, PS_DirectP3C7)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP3C9, PS_DirectP3C9)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP3C11, PS_DirectP3C11)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP5C1, PS_DirectP5C1)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP5C3, PS_DirectP5C3)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP5C5, PS_DirectP5C5)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP5C7, PS_DirectP5C7)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP5C9, PS_DirectP5C9)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP5C11, PS_DirectP5C11)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP7C1, PS_DirectP7C1)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP7C3, PS_DirectP7C3)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP7C5, PS_DirectP7C5)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP7C7, PS_DirectP7C7)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP7C9, PS_DirectP7C9)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP7C11, PS_DirectP7C11)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP9C1, PS_DirectP9C1)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP9C3, PS_DirectP9C3)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP9C5, PS_DirectP9C5)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP9C7, PS_DirectP9C7)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP9C9, PS_DirectP9C9)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP9C11, PS_DirectP9C11)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP11C1, PS_DirectP11C1)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP11C3, PS_DirectP11C3)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP11C5, PS_DirectP11C5)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP11C7, PS_DirectP11C7)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP11C9, PS_DirectP11C9)
DEFINE_DIRECT_TECHNIQUE(TechniqueDirectP11C11, PS_DirectP11C11)

technique TechniqueComposite
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_Composite();
    }
}

technique TechniqueWriteShadow1
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow1();
    }
}

technique TechniqueWriteShadow3
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow3();
    }
}

technique TechniqueWriteShadow5
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow5();
    }
}

technique TechniqueWriteShadow7
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow7();
    }
}

technique TechniqueWriteShadow9
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow9();
    }
}

technique TechniqueWriteShadow11
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow11();
    }
}

technique TechniqueWriteShadowSkin1
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow1();
    }
}

technique TechniqueWriteShadowSkin3
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow3();
    }
}

technique TechniqueWriteShadowSkin5
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow5();
    }
}

technique TechniqueWriteShadowSkin7
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow7();
    }
}

technique TechniqueWriteShadowSkin9
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow9();
    }
}

technique TechniqueWriteShadowSkin11
{
    pass P0
    {
        CullMode    = NONE;
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow11();
    }
}

technique TechniqueComposite1
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_Composite1();
    }
}

technique TechniqueComposite3
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_Composite3();
    }
}

technique TechniqueComposite5
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_Composite5();
    }
}

technique TechniqueComposite7
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_Composite7();
    }
}

technique TechniqueComposite9
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_Composite9();
    }
}

technique TechniqueComposite11
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_Composite11();
    }
}

technique TechniqueDebugLightZ
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Composite();
        PixelShader  = compile ps_3_0 PS_DebugLightZ();
    }
}


