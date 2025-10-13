
float4x4 gWorld;
float4x4 gView;
float4x4 gProj;

float2 gInvTexSize = float2(1.0 / 1600.0, 1.0 / 900.0);

float3 gLightDirW = normalize(float3(0.6, 0.7, 0.2));
float3 gLightColor = float3(1.0, 1.0, 1.0);
float3 gAmbient = float3(0.25, 0.25, 0.25);

texture g_texture;
texture g_texZFront;
texture g_texZBack;

sampler2D g_texSampler = sampler_state
{
    Texture = <g_texture>;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MipFilter = LINEAR;
    MaxAnisotropy = 8;
    AddressU = Wrap;
    AddressV = Wrap;
};

sampler2D g_texSamplerFront = sampler_state
{
    Texture = <g_texZFront>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler2D g_texSamplerBack = sampler_state
{
    Texture = <g_texZBack>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};

void VertexShader1(float3 inPos                 : POSITION0,
                   float3 inNormal              : NORMAL0,
                   float2 inUV                  : TEXCOORD0,

                   out float4 outPos            : POSITION0,
                   out float2 outUV             : TEXCOORD0,
                   out float3 outNormalWorld    : TEXCOORD1)
{
    float4 posWorld = mul(float4(inPos, 1.0f), gWorld);
    float4 posView = mul(posWorld, gView);
    float4 posProj = mul(posView, gProj);
    outPos = posProj;

    outUV = inUV;
    outNormalWorld = normalize(mul(inNormal, (float3x3) gWorld));
}

void PixelShader1(float2 inUV         : TEXCOORD0,
                  float3 normalWorld  : TEXCOORD1,

                  out float4 outColor : COLOR0)
{
    float3 albedo = tex2D(g_texSampler, inUV).rgb;

    float3 normalizedNormalWorld = normalize(normalWorld);
    float3 lightDirW = normalize(gLightDirW);

    float NdotL = saturate(dot(normalizedNormalWorld, lightDirW));
    float3 lit = albedo * (gAmbient + gLightColor * NdotL);

    outColor = float4(lit, 1.0f);
}

void VertexShaderScreenUV(float3 inPos : POSITION0,
                          float2 inUV : TEXCOORD0,

                          out float4 outPos : POSITION0,
                          out float2 outUV : TEXCOORD0,
                          out float outEyeZ : TEXCOORD1,
                          out float2 outFogUV : TEXCOORD2)
{
    float4 posW = mul(float4(inPos, 1.0), gWorld);
    float4 posV = mul(posW, gView);
    float4 posH = mul(posV, gProj);

    outEyeZ = posV.z;

    float2 screenUV = posH.xy / posH.w;
    screenUV = screenUV * float2(0.5, -0.5) + 0.5;
    screenUV += 0.5 * gInvTexSize;
    outUV = screenUV;

    outFogUV = inUV;

    outPos = posH;
}

// ------------------------------------------------------------
// Pass 2: write front depth
// ------------------------------------------------------------
void PS_WriteFrontZ(float2 uv : TEXCOORD0,
                    float eyeZ : TEXCOORD1,

                    out float4 outColor : COLOR0)
{
    outColor = float4(eyeZ, 0, 0, 1);
}

// ------------------------------------------------------------
// Pass 3: write back depth
// ------------------------------------------------------------
void PS_WriteBackZ(float2 uv : TEXCOORD0,
                   float eyeZ : TEXCOORD1,

                    out float4 outColor : COLOR0)
{
    outColor = float4(eyeZ, 0, 0, 1);
}

// ============================================================
// Pass 4: fog composite (thin parts brighter)
// alpha_sss = pow(exp(-sigmaT * thickness), sssPow)
// color = fogColor * tint * alpha_sss  (premultiplied)
// ============================================================
float gSigmaT = 2.0;
float3 gFogColor = float3(1.0, 1.0, 1.0);
float gSSSPow = 1.4;
float gTexStrength = 0.3;

float4 PS_FogComposite
(
    float2 uv : TEXCOORD0,
    float eyeZ : TEXCOORD1,
    float2 fogUV : TEXCOORD2
) : COLOR0
{
    float frontZ = tex2D(g_texSamplerFront, uv).r;
    float backZ = tex2D(g_texSamplerBack, uv).r;

    if (frontZ <= 0.0 || backZ <= 0.0)
    {
        discard;
    }

    float thickness = backZ - frontZ;

    if (thickness < 0.0)
    {
        discard;
    }

    float alphaSSS = saturate(exp(-gSigmaT * thickness));
    alphaSSS = pow(alphaSSS, gSSSPow);
    //alphaSSS *= 1.5f;

    float3 tint = 1.f - gTexStrength;

    float3 color = gFogColor * tint * alphaSSS;

    return float4(color, alphaSSS);
}

technique TechniquePass0
{
    pass P0
    {
        CullMode = CCW;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        ZFunc = LESSEQUAL;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
    }
}

technique Technique_FrontDepth
{
    pass P0
    {
        CullMode = CCW;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        ZFunc = LESSEQUAL;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VertexShaderScreenUV();
        PixelShader = compile ps_3_0 PS_WriteFrontZ();
    }
}

technique Technique_BackDepth
{
    pass P0
    {
        CullMode = CW;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        ZFunc = GREATER;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VertexShaderScreenUV();
        PixelShader = compile ps_3_0 PS_WriteBackZ();
    }
}

technique Technique_FogComposite
{
    pass P0
    {
        CullMode = CCW;
        ZEnable = TRUE;
        ZWriteEnable = FALSE;
        ZFunc = LESSEQUAL;

        AlphaBlendEnable = TRUE;
        SrcBlend = ONE;
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShaderScreenUV();
        PixelShader = compile ps_3_0 PS_FogComposite();
    }
}
