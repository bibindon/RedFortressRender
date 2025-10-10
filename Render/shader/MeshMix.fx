
float4x4 g_matWorld;
float4x4 g_matViewProj;
float4x4 g_matWorldViewProj;

float4 g_lightNormal = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_lightPos = { -10.f, 10.f, -10.f, 0.0f };

float4 g_cameraPos = { 10.f, 5.f, 10.f, 0.0f };

float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };

float4 g_specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// スペキュラ光の鋭さ
float g_specularPower = 128.0f;

// スペキュラ光の強さ
float g_specularIntensity = 1.0f;

// 距離フォグの色
float4 g_fogDistanceColor = { 0.5f, 0.5f, 1.0f, 1.0f };

// 距離フォグの強さ
float g_fogDistanceDensity = 0.01f;

// 高さフォグの色
float4 g_fogHeightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// 高さフォグの強さ
float g_fogHeightDensity = 0.01f;

texture g_texture;
sampler g_textureSampler = sampler_state
{
    Texture = (g_texture);
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MaxAnisotropy = 8;
};

// 環境マップ
textureCUBE g_texCubeMap;

samplerCUBE g_texCubeMapSampler = sampler_state
{
    Texture = <g_texCubeMap>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;

    // どれくらいぼかすか
    // 数字が大きいほどぼかされる
    MaxMipLevel = 7;
};

texture g_texNormalMap;
sampler g_texNormalMapSampler = sampler_state
{
    Texture = (g_texNormalMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

float g_time = 0.0f;

//---------------------------------------------------------
// 揺らしエフェクト用パラメータ
//---------------------------------------------------------
bool  g_swayEnable = false;
float g_swayAmount = 0.5f;
float g_swaySpeed  = 2.0f;
float g_swayHeight = 3.0f;

// 視差マッピングは「1パス目では実施せず、2パス目で実装する」というようなことはできない
//
void VertexShader1(in  float4 inPosition     : POSITION,
                   in  float4 inNormal       : NORMAL0,
                   in  float4 inTangent      : TANGENT0,
                   in  float4 inBinormal     : BINORMAL0,
                   in  float4 inTexCood      : TEXCOORD0,

                   out float4 outPosition    : POSITION,
                   out float3 outPosWorld    : TEXCOORD0,
                   out float3 outNormalWorld : TEXCOORD1,
                   out float2 outTexCood     : TEXCOORD2,
                   out float3 outTangent     : TEXCOORD3,
                   out float3 outBinorm      : TEXCOORD4
)
{
    // ゆらぎ効果（草とか）
    if (g_swayEnable)
    {
        float4 pos = inPosition;
    
        // 揺らしエフェクトを適用
        // Y座標の高さに基づいて揺らしの強度を変える（上にいくほど大きく揺れる）
        float heightFactor = (pos.y + 1.0) / 3.0; // 円柱の高さに合わせて調整
        heightFactor = pow(heightFactor, 2.0);
        heightFactor = clamp(heightFactor, 0.0, 1.0);
    
        // 複数の波を組み合わせて自然な揺らしを作成
        float wave1 = sin(g_time * g_swaySpeed) * g_swayAmount;
        float wave2 = sin(g_time * g_swaySpeed * 0.7 + 1.5) * g_swayAmount * 0.5;
        float wave3 = cos(g_time * g_swaySpeed * 1.3 + 2.0) * g_swayAmount * 0.3;
    
        // X軸とZ軸の両方向に揺らしを適用
        float swayX = (wave1 + wave2 + wave3) * heightFactor;
        float swayZ = (sin(g_time * g_swaySpeed * 0.8 + 0.5) * g_swayAmount * 0.7 +
                   cos(g_time * g_swaySpeed * 1.1 + 1.0) * g_swayAmount * 0.4) * heightFactor;
    
        pos.x += swayX;
        pos.z += swayZ;
        inPosition = pos;
    }
    
    outPosition = mul(inPosition, g_matWorldViewProj);

    outPosWorld = mul(inPosition, g_matWorld).xyz;
    outNormalWorld = mul(inNormal, g_matWorld).xyz;
    outTexCood = inTexCood.xy;

    outTangent = normalize(mul(inTangent, g_matWorld)).xyz;
    outBinorm = normalize(mul(inBinormal, g_matWorld)).xyz;
}

//-------------------------------------------------------------
// Pass 0
//-------------------------------------------------------------
void PixelShader1(in float4 inPosition    : POSITION,
                  in float3 inPosWorld    : TEXCOORD0,
                  in float3 inNormalWorld : TEXCOORD1,
                  in float2 inTexCood     : TEXCOORD2,
                  in float3 inTangent     : TEXCOORD3,
                  in float3 inBinorm      : TEXCOORD4,

                  out float4 outColor     : COLOR)
{
    // 正規化はピクセルシェーダーでやらないといけない
    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightNormal.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);

    // 法線マッピングでNdotLを調節
    float NdotL = 0.f;
    float NdotH = 0.f;
    {
        float3 normalInTangent = float3(0, 0, 0);
        normalInTangent.x = tex2D(g_texNormalMapSampler, inTexCood).r * 2.0 - 1.0;
        normalInTangent.y = tex2D(g_texNormalMapSampler, inTexCood).g * 2.0 - 1.0;
        normalInTangent.z = tex2D(g_texNormalMapSampler, inTexCood).b * 2.0 - 1.0;
        normalInTangent.x *= -1;
        normalInTangent = normalize(normalInTangent);

        // TBN（Tangent, Binormal, Normal）でワールドへ
        float3x3 tangentToWorld = float3x3(-inTangent, -inBinorm, inNormalWorld);
        float3 normalInWorld = normalize(mul(normalInTangent, tangentToWorld));

        // Lambert 拡散（光線方向）
        lightDir = normalize(g_lightNormal.xyz);
        NdotL = saturate(dot(normalInWorld, lightDir));
        NdotH = saturate(dot(normalInWorld, halfVector));
    }
    
    float3 albedo = tex2D(g_textureSampler, inTexCood).rgb * g_diffuse.rgb;

    float3 lambert = albedo * NdotL;
    float3 ambient = albedo * g_ambient.rgb;

    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz;

    float3 finalColor = ambient.rgb + lambert + specular;

    outColor = saturate(float4(finalColor, 1.f));
}

//-------------------------------------------------------------
// Pass 1
//-------------------------------------------------------------
void PixelShaderCubeMapping(in float4 inPosition     : POSITION,
                            in float3 inPosWorld     : TEXCOORD0,
                            in float3 inNormalWorld  : TEXCOORD1,
                            in float2 inTexCood      : TEXCOORD2,

                            out float4 outColor      : COLOR)
{
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 reflectWorld = reflect(-cameraDir, normalize(inNormalWorld));

    outColor = float4(texCUBE(g_texCubeMapSampler, reflectWorld).rgb, 0.2);
}

//-------------------------------------------------------------
// Pass 2
//-------------------------------------------------------------
// 霧の減衰関数（やわらか）
float FogAmountExp(float distance, float density)
{
    return 1 - exp(-density * distance);
}

// 霧の減衰関数（リアル）
float FogAmountExp2(float distance, float density)
{
    float x = density * distance;
    return 1 - exp(-x * x);
}

void PixelShaderFog(in float4 inPosition     : POSITION,
                    in float3 inPosWorld     : TEXCOORD0,
                    in float3 inNormalWorld  : TEXCOORD1,
                    in float2 inTexCood      : TEXCOORD2,

                    out float4 outColor      : COLOR)
{
    outColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    //--------------------------------------------------------
    // 距離フォグ
    //--------------------------------------------------------
    float cameraDistance = distance(g_cameraPos.xyz, inPosWorld);
    float fogDensity = FogAmountExp(cameraDistance, g_fogDistanceDensity);

    outColor += float4(g_fogDistanceColor.rgb, fogDensity);

    //--------------------------------------------------------
    // 高さフォグ
    //--------------------------------------------------------
    // 高さフォグは高さだけでなく距離も影響する必要がある。
    // まず距離を見て、そのあと、高さによって薄くする
    float fogDensityHeight = FogAmountExp(cameraDistance, g_fogDistanceDensity);
    
    // 高度が高くなるほど低くなる数値
    float fogHeightDensity = FogAmountExp(-inPosWorld.y, g_fogHeightDensity);

    fogDensityHeight *= fogHeightDensity;

    outColor += float4(g_fogHeightColor.rgb, fogDensityHeight);
}

technique Technique1
{
    pass Pass1
    {
        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShader1();
    }

    pass PassCubeMapping
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = ONE;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderCubeMapping();
    }

    pass PassFog
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderFog();
    }
}
