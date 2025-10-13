// ============================================================
// Volumetric Fog + SSS-like thin-bright look (DX9 / SM3.0)
// - Opaque objects are Lambert shaded
// - Fog is drawn on top: thinner parts appear whiter
// ============================================================

float4x4 gWorld;
float4x4 gView;
float4x4 gProj;

float2 gInvTexSize = float2(1.0 / 1600.0, 1.0 / 900.0);

// ------------------------------------------------------------
// Light for opaque pass
// ------------------------------------------------------------
float3 gLightDirW = normalize(float3(0.6, 0.7, 0.2));
float3 gLightColor = float3(1.0, 1.0, 1.0);
float3 gAmbient = float3(0.25, 0.25, 0.25);

// ------------------------------------------------------------
// Textures
// ------------------------------------------------------------
texture gDiffuse;
texture gFrontDepthTex;
texture gBackDepthTex;
texture gFogTex;

sampler2D SDiffuse = sampler_state
{
    Texture = <gDiffuse>;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MipFilter = LINEAR;
    MaxAnisotropy = 8;
    AddressU = Wrap;
    AddressV = Wrap;
};

sampler2D SFront = sampler_state
{
    Texture = <gFrontDepthTex>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler2D SBack = sampler_state
{
    Texture = <gBackDepthTex>;
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler2D SFog = sampler_state
{
    Texture = <gFogTex>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

// ============================================================
// OPAQUE (Lambert)
// ============================================================
float4 VS_Opaque
(
    float3 inPos : POSITION0,
    float3 inNrm : NORMAL0,
    float2 inUV : TEXCOORD0,

    out float2 outUV : TEXCOORD0,
    out float3 outNrmW : TEXCOORD1
) : POSITION0
{
    float4 posW = mul(float4(inPos, 1.0), gWorld);
    float4 posV = mul(posW, gView);
    float4 posH = mul(posV, gProj);

    outUV = inUV;
    outNrmW = normalize(mul(inNrm, (float3x3) gWorld));
    return posH;
}

float4 PS_Opaque
(
    float2 uv : TEXCOORD0,
    float3 nrmW : TEXCOORD1
) : COLOR0
{
    float3 albedo = tex2D(SDiffuse, uv).rgb;

    float3 normalW = normalize(nrmW);
    float3 lightDirW = normalize(gLightDirW);

    float ndotl = saturate(dot(normalW, lightDirW));
    float3 lit = albedo * (gAmbient + gLightColor * ndotl);

    return float4(lit, 1.0);
}

technique Technique_Opaque
{
    pass P0
    {
        CullMode = CCW;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        ZFunc = LESSEQUAL;
        AlphaBlendEnable = FALSE;

        VertexShader = compile vs_3_0 VS_Opaque();
        PixelShader = compile ps_3_0 PS_Opaque();
    }
}

// ============================================================
// Shared VS for fog passes
// - outputs screen UV and object UV
// - eye-space Z is used as linear depth
// ============================================================
float4 VS_ScreenUV
(
    float3 inPos : POSITION0,
    float2 inUV : TEXCOORD0,

    out float2 outUV : TEXCOORD0,
    out float outEyeZ : TEXCOORD1,
    out float2 outFogUV : TEXCOORD2
) : POSITION0
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

    return posH;
}

// ------------------------------------------------------------
// Pass 2: write front depth
// ------------------------------------------------------------
float4 PS_WriteFrontZ(float2 uv : TEXCOORD0, float eyeZ : TEXCOORD1) : COLOR0
{
    return float4(eyeZ, 0, 0, 1);
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

        VertexShader = compile vs_3_0 VS_ScreenUV();
        PixelShader = compile ps_3_0 PS_WriteFrontZ();
    }
}

// ------------------------------------------------------------
// Pass 3: write back depth
// ------------------------------------------------------------
float4 PS_WriteBackZ(float2 uv : TEXCOORD0, float eyeZ : TEXCOORD1) : COLOR0
{
    return float4(eyeZ, 0, 0, 1);
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

        VertexShader = compile vs_3_0 VS_ScreenUV();
        PixelShader = compile ps_3_0 PS_WriteBackZ();
    }
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
    float frontZ = tex2D(SFront, uv).r;
    float backZ = tex2D(SBack, uv).r;

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

    float3 fogTex = tex2D(SFog, fogUV).rgb;
    float3 tint = lerp(float3(1.0, 1.0, 1.0), fogTex, saturate(gTexStrength));

    float3 color = gFogColor * tint * alphaSSS;

    return float4(color, alphaSSS);
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

        VertexShader = compile vs_3_0 VS_ScreenUV();
        PixelShader = compile ps_3_0 PS_FogComposite();
    }
}
