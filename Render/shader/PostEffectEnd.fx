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

float4 PS_Copy(float2 uv : TEXCOORD0) : COLOR0
{
    return tex2D(SrcSamp, uv);
}

technique Copy
{
    pass p0
    {
        // VSは書かない（固定機能 or 既存のVSでもOK）
        PixelShader = compile ps_3_0 PS_Copy();
    }
}
