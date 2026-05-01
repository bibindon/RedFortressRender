
float4x4 g_matWorld;
float4x4 g_matView;
float4x4 g_matProj;

static const int MAX_MATRICES = 8;
float4x3 g_matWorldArray[MAX_MATRICES];
int g_currentBoneIndex;

float g_fNear  = 1.0f;
float g_fFar   = 1000.0f;

float g_posRange = 50.0f;

struct VS_INPUT
{
    float4 positionObject : POSITION0;
    float3 normalObject   : NORMAL0;
};

struct VS_OUTPUT
{
    float4 positionClip   : POSITION0;
    float  viewSpaceZ     : TEXCOORD0;
    float3 positionWorld  : TEXCOORD1;
    float3 normalWorld    : TEXCOORD2;
};

VS_OUTPUT VS_GBuffer(VS_INPUT inputData)
{
    VS_OUTPUT outputData;

    float4 positionWorld4 = mul(inputData.positionObject, g_matWorld);
    float4 positionView4  = mul(positionWorld4, g_matView);
    outputData.positionClip = mul(positionView4, g_matProj);

    outputData.viewSpaceZ = positionView4.z;
    outputData.positionWorld = positionWorld4.xyz;

    float3 nWS = mul(inputData.normalObject, (float3x3)g_matWorld);
    outputData.normalWorld = normalize(nWS);


    return outputData;
}

void VS_GBufferSkin(in  float4 inPosition     : POSITION,
                    in  float4 inBlendWeights : BLENDWEIGHT,
                    in  float4 inBlendIndices : BLENDINDICES,
                    in  float4 inNormal       : NORMAL,
                    out float4 outPosition    : POSITION0,
                    out float  outViewSpaceZ  : TEXCOORD0,
                    out float3 outWorldPos    : TEXCOORD1,
                    out float3 outWorldNormal : TEXCOORD2,
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
                    out float4 outPosition    : POSITION0,
                    out float  outViewSpaceZ  : TEXCOORD0,
                    out float3 outWorldPos    : TEXCOORD1,
                    out float3 outWorldNormal : TEXCOORD2,
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

    float4 positionView4 = mul(float4(position, 1.0f), g_matView);

    outPosition = mul(positionView4, g_matProj);
    outViewSpaceZ = positionView4.z;
    outWorldPos = position;
    outWorldNormal = normalize(normalPosition - position);
}

void PS_GBuffer(VS_OUTPUT inputData,
                 out float4 outRT0 : COLOR0,
                 out float4 outRT1 : COLOR1,
                 out float4 outRT2 : COLOR2)
{
    float linearZ = (inputData.viewSpaceZ - g_fNear) / (g_fFar - g_fNear);
    linearZ = saturate(linearZ);

    float zVisual = linearZ;

    outRT0 = float4(zVisual, zVisual, zVisual, linearZ);

    float3 normalized = inputData.positionWorld / g_posRange;
    float3 world01 = normalized * 0.5f + 0.5f;
    world01 = saturate(world01);
    
    outRT1 = float4(world01, 1.0f);

    // ---- RT2: WS 法線を 0..1 にエンコード ----
    float3 n01 = saturate(inputData.normalWorld * 0.5f + 0.5f);
    outRT2 = float4(n01, 1.0f);
}

// バックフェイスの線形深度だけを出力する（厚み情報用）
void PS_GBufferBackFace(VS_OUTPUT inputData,
                        out float4 outRT0 : COLOR0)
{
    float linearZ = (inputData.viewSpaceZ - g_fNear) / (g_fFar - g_fNear);
    linearZ = saturate(linearZ);
    outRT0 = float4(linearZ, linearZ, linearZ, linearZ);
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
        PixelShader  = compile ps_3_0 PS_GBuffer();
    }
}
