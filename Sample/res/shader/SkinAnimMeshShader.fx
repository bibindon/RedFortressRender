float4 g_light_normal;
float g_light_brightness;
//float4 g_light_diffuse = {1, 0, 0, 0}; // TODO
float4 g_diffuse;
float4 g_ambient = { 0.2f, 0.2f, 0.5f, 0.0f };

// Matrix Palette 
static const int MAX_MATRICES = 26;
float4x3 g_world_matrix_array[MAX_MATRICES];
float4x4 g_view_projection;

void vertex_shader(in  float4  in_position      : POSITION,
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

    int4 index_vector = D3DCOLORtoUBYTE4(in_blend_indices);

    float blend_weights_array[4] = (float[4])in_blend_weights;
    int   index_array[4]         = (int[4])index_vector;

    [unroll] for (int i = 0; i < bone_number - 1; ++i)
    {
        last_weight = last_weight + blend_weights_array[i];

        position += mul(in_position, g_world_matrix_array[index_array[i]]) * blend_weights_array[i];
        normal += mul(in_normal, g_world_matrix_array[index_array[i]]) * blend_weights_array[i];
    }

    last_weight = 1.0f - last_weight;

    position += (mul(in_position, g_world_matrix_array[index_array[bone_number - 1]]) * last_weight);
    normal += (mul(in_normal, g_world_matrix_array[index_array[bone_number - 1]]) * last_weight);

    out_position = mul(float4(position.xyz, 1.0f), g_view_projection);

    normal -= position;
    float4 normal4 = normalize(float4(normal.xyz, 1.0f));


    float light_intensity = g_light_brightness * dot(normal4, g_light_normal);
    out_diffuse = g_diffuse * max(0, light_intensity) + g_ambient;
    out_diffuse.a = 1.0f;

    out_texcoord0 = in_texcoord0.xy;
}

texture g_mesh_texture;
sampler mesh_texture_sampler = sampler_state {
    Texture   = (g_mesh_texture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

void pixel_shader(
    in  float4 in_diffuse  : COLOR0,
    in  float2 in_texcood  : TEXCOORD0,
    out float4 out_diffuse : COLOR0)
{
    float4 color_result = (float4)0;

    color_result = tex2D(mesh_texture_sampler, in_texcood);

    out_diffuse = (in_diffuse * color_result);
    out_diffuse.a = 1.0f;
}

int current_bone_numbers;
VertexShader vsArray[4] = {
    compile vs_2_0 vertex_shader(1),
    compile vs_2_0 vertex_shader(2),
    compile vs_2_0 vertex_shader(3),
    compile vs_2_0 vertex_shader(4)
};

technique technique_
{
    pass pass_
    {
//        AlphaBlendEnable = TRUE;
//        SrcBlend = SRCALPHA;
//        DestBlend = INVSRCALPHA;

        VertexShader = (vsArray[current_bone_numbers]);
        PixelShader = compile ps_2_0 pixel_shader();
    }
}
