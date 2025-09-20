
// �ʓx�̃��x��
// 1.0f : ������
// 0.0f : �O���[�X�P�[��
// 2.0f : �ʓx2�{
float g_level = 1.0f;

texture texture1;
sampler textureSampler = sampler_state {
    Texture = (texture1);
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
};

void VertexShader1(in  float4 inPosition  : POSITION,
                   in  float2 inTexCood   : TEXCOORD0,

                   out float4 outPosition : POSITION,
                   out float2 outTexCood  : TEXCOORD0)
{
    outPosition = inPosition;
    outTexCood = inTexCood;
}

void PixelShader1(in float4 inPosition    : POSITION,
                  in float2 inTexCood     : TEXCOORD0,

                  out float4 outColor     : COLOR)
{
    float4 workColor = (float4)0;
    workColor = tex2D(textureSampler, inTexCood);

    float average = (workColor.r + workColor.g + workColor.b) / 3;

    // �ʓx���グ��������
    workColor.r = average + (workColor.r - average) * g_level;
    workColor.g = average + (workColor.g - average) * g_level;
    workColor.b = average + (workColor.b - average) * g_level;

    workColor = saturate(workColor);

    outColor = workColor;
    
}

technique Technique1
{
    pass Pass1
    {
        CullMode = NONE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
   }
}
