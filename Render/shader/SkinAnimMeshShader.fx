float4 g_lightNormal;
float g_lightBrightness;
float4 g_diffuse;
float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };

static const int MAX_MATRICES = 8;
float4x3 g_matWorldArray[MAX_MATRICES];
float4x4 g_matViewProj;

texture g_texture;
sampler g_sampler = sampler_state
{
    Texture   = (g_texture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

void VertexShader1(in float4 in_position : POSITION,
                   in float4 in_blend_weights : BLENDWEIGHT,
                   in float4 in_blend_indices : BLENDINDICES,
                   in float4 in_normal : NORMAL,
                   in float3 in_texcoord0 : TEXCOORD0,

                   out float4 out_position : POSITION,
                   out float4 out_diffuse : COLOR,
                   out float2 out_texcoord0 : TEXCOORD0,
                   uniform int bone_number);

int g_currentBoneIndex;
VertexShader vsArray[4] =
{
    compile vs_3_0 VertexShader1(1),
    compile vs_3_0 VertexShader1(2),
    compile vs_3_0 VertexShader1(3),
    compile vs_3_0 VertexShader1(4)
};

void VertexShader1(in  float4  in_position      : POSITION,
                   in  float4  in_blend_weights : BLENDWEIGHT,
                   in  float4  in_blend_indices : BLENDINDICES,
                   in  float4  in_normal        : NORMAL,
                   in  float3  in_texcoord0     : TEXCOORD0,

                   out float4  out_position     : POSITION,
                   out float4  out_diffuse      : COLOR,
                   out float2  out_texcoord0    : TEXCOORD0,
                   uniform int bone_number)
{
    float3 position    = 0.0f;
    float3 normal      = 0.0f;
    float  last_weight = 0.0f;

    int4 index_vector = (int4)in_blend_indices;

    float blend_weights_array[4] = (float[4])in_blend_weights;
    int   index_array[4]         = (int[4])index_vector;

    [unroll] for (int i = 0; i < bone_number - 1; ++i)
    {
        last_weight = last_weight + blend_weights_array[i];

        position += mul(in_position, g_matWorldArray[index_array[i]]) * blend_weights_array[i];
        normal += mul(in_normal, g_matWorldArray[index_array[i]]) * blend_weights_array[i];
    }

    last_weight = 1.0f - last_weight;

    position += (mul(in_position, g_matWorldArray[index_array[bone_number - 1]]) * last_weight);
    normal += (mul(in_normal, g_matWorldArray[index_array[bone_number - 1]]) * last_weight);

    out_position = mul(float4(position.xyz, 1.0f), g_matViewProj);

    normal -= position;
    float4 normal4 = normalize(float4(normal.xyz, 1.0f));


    float light_intensity = g_lightBrightness * dot(normal4, g_lightNormal);
    out_diffuse = g_diffuse * max(0, light_intensity) + g_ambient;
    out_diffuse.a = 1.0f;

    out_texcoord0 = in_texcoord0.xy;
}

void PixelShader1(in  float4 in_diffuse  : COLOR0,
                  in  float2 in_texcood  : TEXCOORD0,

                  out float4 out_diffuse : COLOR0)
{
    float4 color_result = (float4)0;

    color_result = tex2D(g_sampler, in_texcood);

    out_diffuse = (in_diffuse * color_result);
}

technique Technique1
{
    pass Pass0
    {
        // タテガミがうまく表示できないが、このコメントアウトを外すと
        // 逆にタテガミだけ正しく表示できるようになる。
        // ZEnable = FALSE;
        // ZWriteEnable = FALSE;
        // ZFunc        = Always;

        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = (vsArray[g_currentBoneIndex]);
        PixelShader = compile ps_3_0 PixelShader1();
    }
}
