
float4x4 g_matWorld;
float4x4 g_matViewProj;
float4x4 g_matWorldViewProj;

float4 g_lightDir = { 0.3f, 1.0f, 0.5f, 0.0f };
float4 g_lightPos = { -10.f, 10.f, -10.f, 0.0f };

float4 g_cameraPos = { 10.f, 5.f, 10.f, 0.0f };

float4 g_ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
float4 g_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };

float4 g_specularColor = { 1.0f, 1.0f, 1.0f, 1.0f };
float2 g_screenSize = { 1600.0f, 900.0f };

// スペキュラ光の鋭さ
float g_specularPower = 16.0f;
// float g_specularPower = 128.0f;
//float g_specularPower = 0.0f;

// スペキュラ光の強さ
float g_specularIntensity = 0.2f;
//float g_specularIntensity = 0.0f;

// 距離フォグの色
float4 g_fogDistanceColor = { 0.5f, 0.5f, 1.0f, 1.0f };

// 距離フォグの強さ
float g_fogDistanceDensity = 0.01f;

// 高さフォグの色
float4 g_fogHeightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

// 高さフォグの強さ
float g_fogHeightDensity = 0.01f;

// 空間の明るさ
// 0なら洞窟、0.1なら夜、1なら快晴、0.5なら室内、という感じ
// 1.0を超えると彩度が上がり、逆に暗くなるようにすると面白い気がする。
float g_fSunLightIntensity = 0.2f;

texture g_texture;
sampler g_textureSampler = sampler_state
{
    Texture = (g_texture);
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    MaxAnisotropy = 8;

    AddressU = Wrap;
    AddressV = Wrap;

    MaxMipLevel = 1;
};

// 環境マップ
textureCUBE g_texCubeMap;

samplerCUBE g_cubeMapSampler = sampler_state
{
    Texture = <g_texCubeMap>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;

    // どれくらいぼかすか
    // 数字が大きいほどぼかされる
    //MaxMipLevel = 7;
    MaxMipLevel = 1;
};

// 法線マップ
texture g_texNormalMap;
sampler g_normalMapSampler = sampler_state
{
    Texture = (g_texNormalMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = Wrap;
    AddressV = Wrap;

    MaxMipLevel = 1;
};

//------------------------------------------------------
// 視差遮蔽マッピング関連
//------------------------------------------------------

bool g_bPOM = false;
bool g_bNormalMapping = false;

// 高さ 0.0 ~ 1.0
float g_fHeightMapScale = 0.1f;

// サンプリング数（最小）
int g_nMinSamples = 50;

// サンプリング数（最大）
int g_nMaxSamples = 100;

// 高さマップ
texture g_texHeightMap;
sampler g_heightMapSampler = sampler_state
{
    Texture = (g_texHeightMap);
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

//---------------------------------------------------------
// ポイントライト
//---------------------------------------------------------
float3 g_pointLightPos[16];
float  g_pointLightBrightness[16];
float3 g_pointLightColor[16];

//---------------------------------------------------------
// 頂点シェーダー
// 視差マッピングは「1パス目では実施せず、2パス目で実装する」というようなことはできない
//
// WS ... WorldSpace
// TS ... TangentSpace
// OS ... ObjectSpace(Local coordinate)
//---------------------------------------------------------
void VertexShader1(in  float4 inPosition     : POSITION,
                   in  float4 inNormal       : NORMAL0,
                   in  float4 inTangent      : TANGENT0,
                   in  float4 inBinormal     : BINORMAL0,
                   in  float4 inTexCoord     : TEXCOORD0,

                   out float4 outPosition    : POSITION,
                   out float3 outPosWorld    : TEXCOORD0,
                   out float3 outNormalWorld : TEXCOORD1,
                   out float2 outTexCood     : TEXCOORD2,
                   out float3 outTangent     : TEXCOORD3,
                   out float3 outBinorm      : TEXCOORD4,
                   out float3 outvViewWS     : TEXCOORD5,
                   out float3 outvLightTS    : TEXCOORD6,
                   out float3 outvViewTS     : TEXCOORD7,
                   out float2 outvParallaxOffsetTS    : TEXCOORD8)
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

    // outPosWorldでは4x4を使いoutNormalWorldでは3x3の変換行列を使っている
    // こうしないと環境マップがおかしくなる
    outPosWorld = mul(inPosition, g_matWorld).xyz;

    float3x3 world3x3 = (float3x3) g_matWorld;
    outNormalWorld = mul(inNormal.xyz, world3x3);

    outTexCood = inTexCoord.xy;

    outTangent = normalize(mul(inTangent.xyz, world3x3));
    outBinorm = normalize(mul(inBinormal.xyz, world3x3));

    float3 vViewWS = g_cameraPos.xyz - outPosWorld.xyz;
    outvViewWS = vViewWS;

    // 光源ベクトル（正規化しない）
    float3 vLightWS = g_lightDir.xyz;

    // 光源ベクトル・カメラ方向ベクトルを接空間へ変換
    float3x3 mWorldToTangent = float3x3(outTangent, outBinorm, outNormalWorld);

    outvLightTS = mul(mWorldToTangent, vLightWS);
    outvViewTS = mul(mWorldToTangent, vViewWS);

    // ズレ量
    // グレージング角なら沢山ズレるし、正面を向いてるならズレない。
    // それを表す数値
    outvParallaxOffsetTS = outvViewTS.xy / outvViewTS.z;

    outvParallaxOffsetTS *= g_fHeightMapScale;
}

float2 CalcUVCoordWithPOM(float3 inNormalizedNormalWS,
                          float2 inTexCoord,
                          float3 invViewWS,
                          float3 invViewTS,
                          float2 invParallaxOffsetTS);

//-------------------------------------------------------------
// Pass 0
//-------------------------------------------------------------
void PixelShader1(in float2 inScreenPos   : VPOS,
                  in float3 inPosWorld    : TEXCOORD0,
                  in float3 inNormalWorld : TEXCOORD1,
                  in float2 inTexCoord    : TEXCOORD2,
                  in float3 inTangent     : TEXCOORD3,
                  in float3 inBinorm      : TEXCOORD4,
                  in float3 invViewWS     : TEXCOORD5,
                  in float3 invLightTS    : TEXCOORD6,
                  in float3 invViewTS     : TEXCOORD7,
                  in float2 invParallaxOffsetTS  : TEXCOORD8,

                  out float4 outColor     : COLOR)
{
    // 正規化はピクセルシェーダーでやらないといけない
    float3 normal = normalize(inNormalWorld);
    float3 lightDir = normalize(g_lightDir.xyz);
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 halfVector = normalize(lightDir + cameraDir);

    // ?
    // どこかで必要なはず
    invLightTS = normalize(invLightTS);

    //----------------------------------------------------
    // 視差遮蔽マッピング
    //----------------------------------------------------
    if (g_bPOM)
    {
        inTexCoord = CalcUVCoordWithPOM(normal,
                                        inTexCoord,
                                        invViewWS,
                                        invViewTS,
                                        invParallaxOffsetTS);
    }
    
    float3 albedo = tex2D(g_textureSampler, inTexCoord).rgb * g_diffuse.rgb;

    //-----------------------------------------------------------------------
    // 法線マッピングでNdotLを調節
    //-----------------------------------------------------------------------
    float NdotL = 0.f;
    float NdotH = 0.f;

    // 法線マッピングを行うか
    if (g_bNormalMapping)
    {
        float3 normalInTangent = float3(0, 0, 0);
        normalInTangent.x = tex2D(g_normalMapSampler, inTexCoord).r * 2.0 - 1.0;
        normalInTangent.y = tex2D(g_normalMapSampler, inTexCoord).g * 2.0 - 1.0;
        normalInTangent.z = tex2D(g_normalMapSampler, inTexCoord).b * 2.0 - 1.0;
        normalInTangent.x *= -1;
        normalInTangent = normalize(normalInTangent);

        // TBN（Tangent, Binormal, Normal）でワールドへ
        float3x3 tangentToWorld = float3x3(-inTangent, -inBinorm, normal);
        float3 normalInWorld = normalize(mul(normalInTangent, tangentToWorld));

        // Lambert 拡散（光線方向）
        // saturate関数をここで実行するとマイナス成分が消える。
        NdotL = dot(normalInWorld, lightDir);
        NdotH = saturate(dot(normalInWorld, halfVector));
    }
    else
    {
        NdotL = dot(normal, lightDir);
        NdotH = saturate(dot(normal, halfVector));
    }


    float3 lambert = 0.f;
    
    // ハーフランバート
    // 深度バッファシャドウを実行すると、影が2重に表示されてしまう。
    // ハーフランバートならマシになる
    if (true)
    {
        NdotL = (NdotL + 1.0f) * 0.5f;

        // 0.5が0.7になるような補正をかける
        // 対数グラフのイメージ
        NdotL = pow(NdotL, 0.5);

    }

    lambert = albedo * NdotL * g_fSunLightIntensity;

    float3 ambient = float3(0.2, 0.2, 0.2) * albedo;

    // 陰の彩度を上げる
    // 要らないかもしれない
    if (false)
    {
        if (NdotL <= 0.0f)
        {
            // アルベドの彩度を強調した色をアンビエント色に設定する
            // 陰の彩度を上げたいが、これだと全体的に彩度が高くなってしまう。
            float3 workColor = albedo;

            float average = (workColor.r + workColor.g + workColor.b) / 3;

            // 彩度を上げ下げする
            workColor.r = average + (workColor.r - average) * 8.0f;
            workColor.g = average + (workColor.g - average) * 8.0f;
            workColor.b = average + (workColor.b - average) * 8.0f;
            lambert = workColor * 0.05f * -NdotL;
        }
    }

    float3 specular = (pow(NdotH, g_specularPower) * g_specularIntensity) * g_specularColor.xyz;

    float3 finalColor = ambient.rgb + lambert + specular;

    outColor = saturate(float4(finalColor, 1.f));

    if (false)
    {
        if (abs(inScreenPos.x - (g_screenSize.x * 0.5f)) <= 0.5f)
        {
            outColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
    }
}

//-------------------------------------------------------------
// Pass 1
//-------------------------------------------------------------
void PixelShaderCubeMapping(in float4 inPosition     : POSITION,
                            in float3 inPosWorld     : TEXCOORD0,
                            in float3 inNormalWorld  : TEXCOORD1,
                            in float2 inTexCoord      : TEXCOORD2,
                            in float3 inTangent      : TEXCOORD3,
                            in float3 inBinorm       : TEXCOORD4,

                            out float4 outColor      : COLOR)
{
    outColor = float4(0, 0, 0, 0);

    float3 normal = normalize(inNormalWorld);
    
    float3 normalInTangent = float3(0, 0, 0);
    normalInTangent.x = tex2D(g_normalMapSampler, inTexCoord).r * 2.0 - 1.0;
    normalInTangent.y = tex2D(g_normalMapSampler, inTexCoord).g * 2.0 - 1.0;
    normalInTangent.z = tex2D(g_normalMapSampler, inTexCoord).b * 2.0 - 1.0;
    normalInTangent.x *= -1;
    normalInTangent = normalize(normalInTangent);

    float3x3 tangentToWorld = float3x3(-inTangent, -inBinorm, normal);
    float3 normalInWorld = normalize(mul(normalInTangent, tangentToWorld));

    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 reflectWorld = reflect(-cameraDir, normalize(normalInWorld));

    outColor = float4(texCUBE(g_cubeMapSampler, reflectWorld).rgb, 0.1f);
}

//-------------------------------------------------------------
// Pass 2
//-------------------------------------------------------------
void PixelShaderGlass(in float4 inPosition     : POSITION,
                      in float3 inPosWorld     : TEXCOORD0,
                      in float3 inNormalWorld  : TEXCOORD1,
                      in float2 inTexCoord      : TEXCOORD2,
                      in float3 inTangent      : TEXCOORD3,
                      in float3 inBinorm       : TEXCOORD4,

                      out float4 outColor      : COLOR)
{
    outColor = float4(0, 0, 0, 0);

    float3 normal = normalize(inNormalWorld);
    
    float3 normalInTangent = float3(0, 0, 0);
    normalInTangent.x = tex2D(g_normalMapSampler, inTexCoord).r * 2.0 - 1.0;
    normalInTangent.y = tex2D(g_normalMapSampler, inTexCoord).g * 2.0 - 1.0;
    normalInTangent.z = tex2D(g_normalMapSampler, inTexCoord).b * 2.0 - 1.0;
    normalInTangent.x *= -1;
    normalInTangent = normalize(normalInTangent);

    // TBN（Tangent, Binormal, Normal）でワールドへ
    float3x3 tangentToWorld = float3x3(-inTangent, -inBinorm, normal);
    float3 normalInWorld = normalize(mul(normalInTangent, tangentToWorld));
    
    float3 cameraDir = normalize(g_cameraPos.xyz - inPosWorld);
    float3 refractWorld = refract(-cameraDir, normalize(normalInWorld), 1.f / 1.5f);

    outColor = float4(texCUBE(g_cubeMapSampler, refractWorld).rgb, 0.8f);
}

//-------------------------------------------------------------
// Pass 3
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

void PixelShaderPointLight(in  float4 inPosition            : POSITION,
                           in  float3 inPosWorld            : TEXCOORD0,
                           in  float3 inNormalWorld         : TEXCOORD1,
                           in  float2 inTexCoord            : TEXCOORD2,
                           in  float3 inTangent             : TEXCOORD3,
                           in  float3 inBinorm              : TEXCOORD4,
                           in  float3 invViewWS             : TEXCOORD5,
                           in  float3 invViewTS             : TEXCOORD7,
                           in  float2 invParallaxOffsetTS   : TEXCOORD8,
                           out float4 outColor              : COLOR)
{
    float3 normalWS = normalize(inNormalWorld);
    float3 cameraDirWS = normalize(g_cameraPos.xyz - inPosWorld);

    // POMでUV更新（必要なときだけ）
    float2 uv = inTexCoord;
    if (g_bPOM)
    {
        uv = CalcUVCoordWithPOM(normalWS,
                                inTexCoord,
                                invViewWS,
                                invViewTS,
                                invParallaxOffsetTS );
    }

    float3 N = normalWS;
    if (g_bNormalMapping)
    {
        // 法線マップがある場合だけ、接空間法線をワールド空間へ変換して使う
        float3 normalTS;
        float4 nTex = tex2D(g_normalMapSampler, uv);
        normalTS.x = nTex.r * 2.0 - 1.0;
        normalTS.y = nTex.g * 2.0 - 1.0;
        normalTS.z = nTex.b * 2.0 - 1.0;
        normalTS.x *= -1.0;
        normalTS = normalize(normalTS);

        float3x3 tbn = float3x3(-inTangent, -inBinorm, normalWS);
        N = normalize(mul(normalTS, tbn));
    }

    float3 accum = 0.0;

    float diffSum = 0.f;

    for (int i = 0; i < 16; ++i)
    {
        if (g_pointLightBrightness[i] <= 0.0f)
        {
            continue;
        }

        float3 Lvec   = g_pointLightPos[i] - inPosWorld;
        float  dist   = length(Lvec);
        float3 L      = Lvec / max(dist, 1e-6);

        float  NdotL  = saturate(dot(N, L));
        float3 H      = normalize(L + cameraDirWS);
        float  NdotH  = saturate(dot(N, H));

        // 減衰：簡易 1/dist （元の式を尊重）
        float  atten  = saturate(1.0 / max(dist, 1e-6));

        float  diff   = g_pointLightBrightness[i] * atten * NdotL;

        float  spec   = 0.0f;
        if (g_specularIntensity > 0.0f)
        {
            spec = pow(NdotH, g_specularPower) * g_specularIntensity
                 * g_pointLightBrightness[i] * atten;
        }

        // ★ RGB に「色 × 強度」を加算
        accum += g_pointLightColor[i] * diff;
        accum += g_pointLightColor[i] * spec;

        diffSum += diff;
    }

    // HDR蓄積：ここでクランプしない
    outColor = float4(accum, saturate(diffSum));
}

float2 CalcUVCoordWithPOM(float3 inNormalizedNormalWS,
                          float2 inTexCoord,
                          float3 invViewWS,
                          float3 invViewTS,
                          float2 invParallaxOffsetTS)
{
    invViewWS = normalize(invViewWS);
    invViewTS= normalize(invViewTS);

    // 視角に応じてサンプル数を変更。
    // グレージング角であるほどステップを細かくして精度を上げる。
    int nNumSteps = (int) lerp(g_nMaxSamples, g_nMinSamples, dot(invViewWS, inNormalizedNormalWS));

    float fStepSize = 1.0 / (float) nNumSteps;
    int nStepIndex = 0;

    float fCurrHeight = 0.0;

    float2 vTexOffsetPerStep = fStepSize * invParallaxOffsetTS;
    float2 vTexCurrentOffset = inTexCoord;

    // 今どの深さの層（Layer）までレイを進めたか
    float fCurrentLayer = 1.0;

    while (nStepIndex < nNumSteps)
    {
        vTexCurrentOffset -= vTexOffsetPerStep;

        // tex2Dgrad関数を使うとPIX For Windowsが落ちる
        // fCurrHeight = tex2Dgrad(g_heightMapSampler, vTexCurrentOffset, dx, dy ).r;
        fCurrHeight = tex2Dlod(g_heightMapSampler, float4(vTexCurrentOffset, 0.0f, 0.0f)).r;

        fCurrentLayer -= fStepSize;

        if (fCurrHeight > fCurrentLayer)
        {
            break;
        }

        nStepIndex++;
    }

    float2 vParallaxOffset = invParallaxOffsetTS * (1 - fCurrentLayer);

    // 疑似的に押し出された表面上の最終テクスチャ座標
    inTexCoord -= vParallaxOffset;
    return inTexCoord;
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
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderCubeMapping();
    }

    pass PassGlass
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderGlass();
    }

    pass PassPointLight
    {
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;

        VertexShader = compile vs_3_0 VertexShader1();
        PixelShader = compile ps_3_0 PixelShaderPointLight();
    }

}

