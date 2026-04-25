float4x4 g_matWorldViewProj;

texture g_texture;
sampler g_textureSampler = sampler_state
{
    Texture   = (g_texture);
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MaxAnisotropy = 8;
};

void VertexShader1(in  float3 inPos       : POSITION,
                   in  float3 inNormal    : NORMAL0,
                   in  float2 inTexCoord  : TEXCOORD0,
                   out float4 outPos      : POSITION,
                   out float2 outTexCoord : TEXCOORD0)
{
    outPos = mul(float4(inPos, 1.0f), g_matWorldViewProj);
    outTexCoord = inTexCoord;
}

void PixelShader1(in  float2 inTexCoord   : TEXCOORD0,
                  out float4 outVecColor  : COLOR0)
{
    outVecColor = tex2D(g_textureSampler, inTexCoord);
}

technique Technique1
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader  = compile ps_3_0 PixelShader1();
    }
}
