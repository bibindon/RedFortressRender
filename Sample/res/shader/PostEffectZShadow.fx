


float4x4 g_matWorld;
float4x4 g_matWorldViewProj;

float4x4 g_matLightView;
float    g_lightNear;
float    g_lightFar;
float4x4 g_matLightViewProj;

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

float3 IncreaseSaturation(float3 color, float amount)
{
    float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
    return saturate(lerp(luminance.xxx, color, amount));
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

float SampleShadowAmount(float2 uvLightView, float fDepthLightView)
{
    float2 uvTexel = float2(g_shadowTexelW, g_shadowTexelH);
    const int FILTER_RADIUS = 5;

    float shadowSum = 0.0f;
    float sampleCount = 0.0f;

    for (int y = -FILTER_RADIUS; y <= FILTER_RADIUS; ++y)
    {
        for (int x = -FILTER_RADIUS; x <= FILTER_RADIUS; ++x)
        {
            float2 sampleUv = uvLightView + float2((float)x, (float)y) * uvTexel;
            if (any(sampleUv < 0.0f) || any(sampleUv > 1.0f))
            {
                continue;
            }

            float shadowDepth = tex2Dlod(samplerLightZ, float4(sampleUv, 0, 0)).r;
            if (shadowDepth < (fDepthLightView - g_shadowBias))
            {
                shadowSum += 1.0f;
            }

            sampleCount += 1.0f;
        }
    }

    if (sampleCount <= 0.0f)
    {
        return 0.0f;
    }

    return shadowSum / sampleCount;
}

// 変数名の末尾のOSはローカル座標の意味
// 変数名の末尾のWSはグローバル座標の意味

//-------------------------------------------------------------------------
// Technique 1
//-------------------------------------------------------------------------

struct VSInDepth
{
    float4 vPosOS  : POSITION0;
};

struct VSOutDepth
{
    float4 vPos    : POSITION0;
    float  fDepth  : TEXCOORD0;
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

    return vout;
}

void VS_DepthFromLightSkin(in  float4 inPosition     : POSITION,
                           in  float4 inBlendWeights : BLENDWEIGHT,
                           in  float4 inBlendIndices : BLENDINDICES,
                           out float4 outPosition    : POSITION0,
                           out float  outDepth       : TEXCOORD0,
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
                           out float4 outPosition    : POSITION0,
                           out float  outDepth       : TEXCOORD0,
                           uniform int boneNumber)
{
    float3 worldPos = SkinPosition(inPosition, inBlendWeights, inBlendIndices, boneNumber);
    float4 posLightView = mul(float4(worldPos, 1.0f), g_matLightView);

    outPosition = mul(float4(worldPos, 1.0f), g_matLightViewProj);
    outDepth = saturate((posLightView.z - g_lightNear) / (g_lightFar - g_lightNear));
}

float4 PS_DepthFromLight(VSOutDepth pin) : COLOR0
{
    float d = pin.fDepth;
    return float4(d, d, d, 1.0f);
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

void PS_WriteShadow(in float4 inPos       : POSITION0,
                    in float2 inUV        : TEXCOORD0,
                    in float3 inWorldPos  : TEXCOORD1,

                    out float4 outColor   : COLOR0)
{
    outColor = float4(0, 0, 0, 0);
    
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
        return;
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
        return;
    }

    float nShadowColor = SampleShadowAmount(uvLightView, fDepthLightView);

    outColor.rgb = nShadowColor.xxx;
    outColor.a = nShadowColor * g_shadowIntensity;
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
    float centerDepth = tex2D(samplerSceneDepth, uv).a;
    float3 centerNormal = DecodeWorldNormal(tex2D(samplerSceneNormal, uv).rgb);

    float4 vShadowColorSum = 0.0f;
    float totalWeight = 0.0f;
    const int FILTER_RADIUS = 5;

    for (int y = -FILTER_RADIUS; y <= FILTER_RADIUS; ++y)
    {
        for (int x = -FILTER_RADIUS; x <= FILTER_RADIUS; ++x)
        {
            float2 sampleUv = uv + float2((float)x * g_compositeTexelW, (float)y * g_compositeTexelH);

            if (any(sampleUv < 0.0f) || any(sampleUv > 1.0f))
            {
                continue;
            }

            float sampleDepth = tex2D(samplerSceneDepth, sampleUv).a;
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

    result.a = 1.f;
    outColor = result;
}

// 光源から見た深度を描画するテクニック
technique TechniqueDepthFromLight
{
    pass P0
    {
        VertexShader = compile vs_3_0 VS_DepthFromLight();
        PixelShader  = compile ps_3_0 PS_DepthFromLight();
    }
}

technique TechniqueDepthFromLightSkin
{
    pass P0
    {
        VertexShader = (vsDepthSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_DepthFromLight();
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


