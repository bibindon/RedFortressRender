#include "Common.fx"

float4x4 g_matWorld;
float4x4 g_matView;
float4x4 g_matProj;
float4x4 g_matWorldViewProjParticle;

float4x3 g_matWorldArray[MAX_MATRICES];
int g_currentBoneIndex;

float g_fNear  = 1.0f;
float g_fFar   = 1000.0f;
float g_fogNear = 1.0f;
float g_fogFar = 1000.0f;

float g_posRange = 50.0f;
int g_swayMode = 0;
float g_time = 0.0f;

texture g_texFrontDepth;
sampler sampFrontDepth = sampler_state
{
    Texture = (g_texFrontDepth);
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_texInstancingAlpha;
sampler sampInstancingAlpha = sampler_state
{
    Texture = (g_texInstancingAlpha);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_texParticleAlpha;
sampler sampParticleAlpha = sampler_state
{
    Texture = (g_texParticleAlpha);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

texture g_texSkinAlpha;
sampler sampSkinAlpha = sampler_state
{
    Texture = (g_texSkinAlpha);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

bool g_useSkinAlphaCutout = false;

struct VS_INPUT
{
    float4 positionObject : POSITION0;
    float3 normalObject   : NORMAL0;
};

struct VS_INPUT_INSTANCING
{
    float4 positionObject : POSITION0;
    float3 normalObject   : NORMAL0;
    float2 texCoord       : TEXCOORD0;
    float4 instancePosRot : TEXCOORD1;
    float4 instanceScale  : TEXCOORD2;
};

struct VS_OUTPUT
{
    float4 positionClip   : POSITION0;
    float  viewSpaceZ     : TEXCOORD0;
    float3 positionWorld  : TEXCOORD1;
    float3 normalWorld    : TEXCOORD2;
    float2 screenUV       : TEXCOORD3;
    float2 alphaUV        : TEXCOORD4;
};

struct VS_INPUT_PARTICLE
{
    float4 positionObject : POSITION0;
    float4 color          : COLOR0;
    float2 texCoord       : TEXCOORD0;
};

VS_OUTPUT VS_GBuffer(VS_INPUT inputData)
{
    VS_OUTPUT outputData;

    float4 positionWorld4 = mul(inputData.positionObject, g_matWorld);
    float4 positionView4  = mul(positionWorld4, g_matView);
    outputData.positionClip = mul(positionView4, g_matProj);
    float2 ndc = outputData.positionClip.xy / outputData.positionClip.w;
    outputData.screenUV = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);

    outputData.viewSpaceZ = positionView4.z;
    outputData.positionWorld = positionWorld4.xyz;

    float3 nWS = mul(inputData.normalObject, (float3x3)g_matWorld);
    outputData.normalWorld = normalize(nWS);
    outputData.alphaUV = 0.0f;


    return outputData;
}

VS_OUTPUT VS_GBufferInstancing(VS_INPUT_INSTANCING inputData)
{
    VS_OUTPUT outputData;

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
    float4 positionView4 = mul(float4(worldPos, 1.0f), g_matView);
    outputData.positionClip = mul(positionView4, g_matProj);
    float2 ndc = outputData.positionClip.xy / outputData.positionClip.w;
    outputData.screenUV = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);

    outputData.viewSpaceZ = positionView4.z;
    outputData.positionWorld = worldPos;

    float3 rotatedNormal;
    rotatedNormal.x = (inputData.normalObject.x * cosY) + (inputData.normalObject.z * sinY);
    rotatedNormal.y = inputData.normalObject.y;
    rotatedNormal.z = (-inputData.normalObject.x * sinY) + (inputData.normalObject.z * cosY);
    outputData.normalWorld = normalize(rotatedNormal);
    outputData.alphaUV = inputData.texCoord;

    return outputData;
}

VS_OUTPUT VS_GBufferParticle(VS_INPUT_PARTICLE inputData)
{
    VS_OUTPUT outputData;

    float4 positionView4 = mul(inputData.positionObject, g_matView);
    outputData.positionClip = mul(inputData.positionObject, g_matWorldViewProjParticle);
    float2 ndc = outputData.positionClip.xy / outputData.positionClip.w;
    outputData.screenUV = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);

    outputData.viewSpaceZ = positionView4.z;
    outputData.positionWorld = inputData.positionObject.xyz;
    outputData.normalWorld = float3(0.0f, 1.0f, 0.0f);
    outputData.alphaUV = inputData.texCoord;

    return outputData;
}

void VS_GBufferSkin(in  float4 inPosition     : POSITION,
                    in  float4 inBlendWeights : BLENDWEIGHT,
                    in  float4 inBlendIndices : BLENDINDICES,
                    in  float4 inNormal       : NORMAL,
                    in  float3 inTexCoord     : TEXCOORD0,
                    out float4 outPosition    : POSITION0,
                    out float  outViewSpaceZ  : TEXCOORD0,
                    out float3 outWorldPos    : TEXCOORD1,
                    out float3 outWorldNormal : TEXCOORD2,
                    out float2 outScreenUV    : TEXCOORD3,
                    out float2 outAlphaUV     : TEXCOORD4,
                    uniform int boneNumber);

VertexShader vsSkinArray[4] =
{
    compile vs_3_0 VS_GBufferSkin(1),
    compile vs_3_0 VS_GBufferSkin(2),
    compile vs_3_0 VS_GBufferSkin(3),
    compile vs_3_0 VS_GBufferSkin(4)
};

void VS_GBufferSkin(in  float4 inPosition     : POSITION,
                    in  float4 inBlendWeights : BLENDWEIGHT,
                    in  float4 inBlendIndices : BLENDINDICES,
                    in  float4 inNormal       : NORMAL,
                    in  float3 inTexCoord     : TEXCOORD0,
                    out float4 outPosition    : POSITION0,
                    out float  outViewSpaceZ  : TEXCOORD0,
                    out float3 outWorldPos    : TEXCOORD1,
                    out float3 outWorldNormal : TEXCOORD2,
                    out float2 outScreenUV    : TEXCOORD3,
                    out float2 outAlphaUV     : TEXCOORD4,
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

    float4 positionView4 = mul(float4(position, 1.0f), g_matView);

    outPosition = mul(positionView4, g_matProj);
    float2 ndc = outPosition.xy / outPosition.w;
    outScreenUV = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);
    outViewSpaceZ = positionView4.z;
    outWorldPos = position;
    outWorldNormal = normal;
    outAlphaUV = inTexCoord.xy;
}

void PS_GBuffer(VS_OUTPUT inputData,
                 out float4 outRT0 : COLOR0,
                 out float4 outRT1 : COLOR1,
                 out float4 outRT2 : COLOR2,
                 out float4 outRT3 : COLOR3)
{
    float linearZ = (inputData.viewSpaceZ - g_fNear) / (g_fFar - g_fNear);
    linearZ = saturate(linearZ);

    float fogLinearZ = (inputData.viewSpaceZ - g_fogNear) / (g_fogFar - g_fogNear);
    fogLinearZ = saturate(fogLinearZ);

    outRT0 = float4(linearZ, 0.0f, 0.0f, 1.0f);
    outRT3 = float4(fogLinearZ, 0.0f, 0.0f, 1.0f);

    float3 normalized = inputData.positionWorld / g_posRange;
    float3 world01 = normalized * 0.5f + 0.5f;
    world01 = saturate(world01);
    
    outRT1 = float4(world01, 1.0f);

    // ---- RT2: WS 法線を 0..1 にエンコード ----
    float3 n01 = saturate(inputData.normalWorld * 0.5f + 0.5f);
    outRT2 = float4(n01, 1.0f);
}

// バックフェイスの線形深度だけを出力する（厚み情報用）
void PS_GBufferInstancing(VS_OUTPUT inputData,
                          out float4 outRT0 : COLOR0,
                          out float4 outRT1 : COLOR1,
                          out float4 outRT2 : COLOR2,
                          out float4 outRT3 : COLOR3)
{
    clip(tex2D(sampInstancingAlpha, inputData.alphaUV).a - 0.1f);
    PS_GBuffer(inputData, outRT0, outRT1, outRT2, outRT3);
}

void PS_GBufferSkin(float  viewSpaceZ  : TEXCOORD0,
                    float3 worldPos    : TEXCOORD1,
                    float3 worldNormal : TEXCOORD2,
                    float2 screenUV    : TEXCOORD3,
                    float2 alphaUV     : TEXCOORD4,
                    out float4 outRT0  : COLOR0,
                    out float4 outRT1  : COLOR1,
                    out float4 outRT2  : COLOR2,
                    out float4 outRT3  : COLOR3)
{
    if (g_useSkinAlphaCutout)
    {
        clip(tex2D(sampSkinAlpha, alphaUV).a - 0.5f);
    }

    VS_OUTPUT inputData;
    inputData.positionClip = 0.0f;
    inputData.viewSpaceZ = viewSpaceZ;
    inputData.positionWorld = worldPos;
    inputData.normalWorld = worldNormal;
    inputData.screenUV = screenUV;
    inputData.alphaUV = alphaUV;
    PS_GBuffer(inputData, outRT0, outRT1, outRT2, outRT3);
}

void PS_GBufferParticle(VS_OUTPUT inputData,
                        out float4 outRT0 : COLOR0,
                        out float4 outRT1 : COLOR1,
                        out float4 outRT2 : COLOR2,
                        out float4 outRT3 : COLOR3)
{
    clip(tex2D(sampParticleAlpha, inputData.alphaUV).a - 0.1f);
    PS_GBuffer(inputData, outRT0, outRT1, outRT2, outRT3);
}

void PS_GBufferBackFace(VS_OUTPUT inputData,
                        out float4 outRT0 : COLOR0,
                        out float4 outRT1 : COLOR1)
{
    float frontLinearZ = tex2D(sampFrontDepth, inputData.screenUV).r;
    float frontViewZ = frontLinearZ * (g_fFar - g_fNear) + g_fNear;
    float thickness = max(inputData.viewSpaceZ - frontViewZ, 0.0f);
    float backLinearZ = saturate((inputData.viewSpaceZ - g_fNear) / (g_fFar - g_fNear));
    outRT0 = float4(thickness, thickness, thickness, thickness);
    outRT1 = float4(backLinearZ, 0.0f, 0.0f, 1.0f);
}

void PS_GBufferSkinBackFace(float  viewSpaceZ : TEXCOORD0,
                            float2 screenUV   : TEXCOORD3,
                            float2 alphaUV    : TEXCOORD4,
                            out float4 outRT0 : COLOR0,
                            out float4 outRT1 : COLOR1)
{
    if (g_useSkinAlphaCutout)
    {
        clip(tex2D(sampSkinAlpha, alphaUV).a - 0.5f);
    }

    VS_OUTPUT inputData;
    inputData.positionClip = 0.0f;
    inputData.viewSpaceZ = viewSpaceZ;
    inputData.positionWorld = 0.0f;
    inputData.normalWorld = 0.0f;
    inputData.screenUV = screenUV;
    inputData.alphaUV = alphaUV;
    PS_GBufferBackFace(inputData, outRT0, outRT1);
}

technique TechniqueGBuffer
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = TRUE;
        ZWriteEnable     = TRUE;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VS_GBuffer();
        PixelShader  = compile ps_3_0 PS_GBuffer();
    }
}

// バックフェイスのみ描画して背面深度を取得（厚み = 背面深度 - 前面深度）
technique TechniqueGBufferInstancing
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = TRUE;
        ZWriteEnable     = TRUE;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VS_GBufferInstancing();
        PixelShader  = compile ps_3_0 PS_GBufferInstancing();
    }
}

technique TechniqueGBufferParticle
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = TRUE;
        ZWriteEnable     = TRUE;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VS_GBufferParticle();
        PixelShader  = compile ps_3_0 PS_GBufferParticle();
    }
}

technique TechniqueGBufferBackFace
{
    pass P0
    {
        CullMode         = CW;
        ZEnable          = TRUE;
        ZWriteEnable     = TRUE;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VS_GBuffer();
        PixelShader  = compile ps_3_0 PS_GBufferBackFace();
    }
}

technique TechniqueGBufferSkin
{
    pass P0
    {
        CullMode         = NONE;
        ZEnable          = TRUE;
        ZWriteEnable     = TRUE;
        AlphaBlendEnable = FALSE;

        VertexShader = (vsSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_GBufferSkin();
    }
}

technique TechniqueGBufferSkinBackFace
{
    pass P0
    {
        CullMode         = CW;
        ZEnable          = TRUE;
        ZWriteEnable     = TRUE;
        AlphaBlendEnable = FALSE;

        VertexShader = (vsSkinArray[g_currentBoneIndex]);
        PixelShader  = compile ps_3_0 PS_GBufferSkinBackFace();
    }
}
