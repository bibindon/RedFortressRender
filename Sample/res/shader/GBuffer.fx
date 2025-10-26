
float4x4 g_matWorld;
float4x4 g_matView;
float4x4 g_matProj;

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
