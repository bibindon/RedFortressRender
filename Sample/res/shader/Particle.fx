float4x4 g_matWorldViewProj;

texture g_texture0;
sampler ParticleSampler = sampler_state
{
    Texture = <g_texture0>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VS_INPUT
{
    float4 position : POSITION0;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 position : POSITION0;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

VS_OUTPUT ParticleVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.position = mul(input.position, g_matWorldViewProj);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

float4 ParticlePS(VS_OUTPUT input) : COLOR0
{
    return tex2D(ParticleSampler, input.uv) * input.color;
}

technique ParticleAlphaTechnique
{
    pass P0
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = TRUE;
        ZWriteEnable = FALSE;
        CullMode = NONE;
        Lighting = FALSE;
        AlphaTestEnable = FALSE;
        VertexShader = compile vs_2_0 ParticleVS();
        PixelShader = compile ps_2_0 ParticlePS();
    }
}

technique ParticleAdditiveTechnique
{
    pass P0
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = ONE;
        ZEnable = TRUE;
        ZWriteEnable = FALSE;
        CullMode = NONE;
        Lighting = FALSE;
        AlphaTestEnable = FALSE;
        VertexShader = compile vs_2_0 ParticleVS();
        PixelShader = compile ps_2_0 ParticlePS();
    }
}
