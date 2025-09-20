// PostEffectEnd.fx
texture g_SrcTex;
sampler2D SrcSamp = sampler_state
{
    Texture = <g_SrcTex>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VS_IN
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
};
struct VS_OUT
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUT VS_Copy(VS_IN i)
{
    VS_OUT o;
    // �N���b�v���(-1..1)�Ŏ󂯎�肻�̂܂ܒʂ�
    o.pos = i.pos;
    o.uv = i.uv;
    return o;
}

float4 PS_Copy(float2 uv : TEXCOORD0) : COLOR0
{
    return tex2D(SrcSamp, uv);
}

technique Copy
{
    pass p0
    {
        VertexShader = compile vs_2_0 VS_Copy();
        PixelShader = compile ps_2_0 PS_Copy();
    }
}
