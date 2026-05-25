


float4x4 g_matWorld;
float4x4 g_matWorldViewProj;

float4x4 g_matLightView;
float    g_lightNear;
float    g_lightFar;
float4x4 g_matLightViewProj;
float    g_instancingAlphaClipThreshold;
float    g_meshAlphaClipThreshold;
int      g_swayMode;
float    g_time;
bool     g_useMeshAlphaCutout;

static const int MAX_MATRICES = 8;
float4x3 g_matWorldArray[MAX_MATRICES];
int g_currentBoneIndex;

float g_shadowTexelW;
float g_shadowTexelH;
float g_compositeTexelW;
float g_compositeTexelH;

// 影の端に表示されるギザギザを抑制。0.002～0.005 で調整
float g_shadowBias;

// 影の濃さ(0 ~ 1)
float g_shadowIntensity;
float g_shadowSaturationBoost;
float g_edgeDepthThreshold;
float g_edgeNormalThreshold;
int g_shadowPcfTapCount;
int g_shadowCompositeTapCount;

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
    float  depthLinear = (vPosLightView.z - g_lightNear) / (g_lightFar - g_lightNear);
    vout.fDepth = saturate(depthLinear);
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
    outDepth = saturate((posLightView.z - g_lightNear) / (g_lightFar - g_lightNear));
    outUV = inUV;
}

float4 PS_DepthFromLight(VSOutDepth pin) : COLOR0
{
    if (g_useMeshAlphaCutout)
    {
        clip(tex2D(sampMeshAlpha, pin.uv).a - g_meshAlphaClipThreshold);
    }

    float d = pin.fDepth;
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

    outColor.rgb = nShadowColor.xxx;
    outColor.a = nShadowColor * g_shadowIntensity;
    return outColor;
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
void PS_Composite(in float4 inPos     : POSITION,
                  in float2 inUV      : TEXCOORD0,

                  out float4 outColor : COLOR0)
{
    float2 uv = inUV + float2(0.5f * g_compositeTexelW, 0.5f * g_compositeTexelH);
    float2 pixelCoord = floor(uv / float2(g_compositeTexelW, g_compositeTexelH));
    float4 vBaseColor = tex2D(samplerBase, uv);
    float4 vCenterShadowColor = tex2D(samplerShadow, uv);
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
            vShadowColorSum += tex2D(samplerShadow, sampleUv) * weight;
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
    vShadowColorSum += tex2D(samplerShadow, sampleUv) * weight;
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
    float4 vCenterShadowColor = tex2D(samplerShadow, uv);
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
        VertexShader = compile vs_3_0 VS_DepthFromLight();
        PixelShader  = compile ps_3_0 PS_DepthFromLight();
    }
}

technique TechniqueDepthFromLightSkin
{
    pass P0
    {
        CullMode     = NONE;
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
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow();
    }
}

technique TechniqueWriteShadowSkin
{
    pass P0
    {
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow();
    }
}

// 二つの画像を合成するテクニック
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
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow1();
    }
}

technique TechniqueWriteShadow3
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow3();
    }
}

technique TechniqueWriteShadow5
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow5();
    }
}

technique TechniqueWriteShadow7
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow7();
    }
}

technique TechniqueWriteShadow9
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow9();
    }
}

technique TechniqueWriteShadow11
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_Base();
        PixelShader  = compile ps_3_0 PS_WriteShadow11();
    }
}

technique TechniqueWriteShadowSkin1
{
    pass P0
    {
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow1();
    }
}

technique TechniqueWriteShadowSkin3
{
    pass P0
    {
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow3();
    }
}

technique TechniqueWriteShadowSkin5
{
    pass P0
    {
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow5();
    }
}

technique TechniqueWriteShadowSkin7
{
    pass P0
    {
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow7();
    }
}

technique TechniqueWriteShadowSkin9
{
    pass P0
    {
        VertexShader = (vsBaseSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_WriteShadow9();
    }
}

technique TechniqueWriteShadowSkin11
{
    pass P0
    {
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


