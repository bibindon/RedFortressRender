
float4x4 g_matWorldViewProj;

float4x4 g_matWorld;
float4x4 g_matViewProj;

float4 g_lightNormal = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_lightPos = { -10.f, 10.f, -10.f, 0.0f };

float4 g_cameraPos = { 10.f, 5.f, 10.f, 0.0f };

float4 g_ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };

float4 g_specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// スペキュラ光の鋭さ
float g_specularPower = 128.0f;

// スペキュラ光の強さ
float g_specularIntensity = 1.0f;

texture g_texture;
sampler g_textureSampler = sampler_state
{
    Texture = (g_texture);
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MaxAnisotropy = 8;
};

void VertexShader1(in  float4 inPosition  : POSITION,
                   in  float4 inNormal    : NORMAL0,
                   in  float4 inTexCood   : TEXCOORD0,

                   out float4 outPosition : POSITION,
                   out float3 outPosLocal : TEXCOORD0,
                   out float3 outNormal   : TEXCOORD1,
                   out float2 outTexCood  : TEXCOORD2)
{
    outPosition = mul(inPosition, g_matWorldViewProj);

    // TODO matWorldのような行列を作ってかけないとワールド座標にならない。
    outPosLocal = inPosition.xyz;
    outNormal = inNormal.xyz;
    outTexCood = inTexCood;
}

void PixelShader1(in float4 inPosition : POSITION,
                  in float3 inPosLocal : TEXCOORD0,
                  in float3 inNormal   : TEXCOORD1,
                  in float2 inTexCood  : TEXCOORD2,

                  out float4 outColor  : COLOR)
{
    // 正規化はピクセルシェーダーでやらないといけない
    float3 normal = normalize(inNormal);
    float3 lightDir = normalize(g_lightPos.xyz - inPosLocal);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosLocal);
    float3 halfVector = normalize(lightDir + cameraDir);

    float NdotL = saturate(dot(normal, lightDir));
    float NdotH = saturate(dot(normal, halfVector));
    
    float3 albedo = tex2D(g_textureSampler, inTexCood).rgb * g_diffuse.rgb;

    float3 lambert = albedo * NdotL;

    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor;

    float3 finalColor = g_ambient.rgb + lambert + specular;

    outColor = saturate(float4(finalColor, 1.f));
}

technique Technique1
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
    }
}
