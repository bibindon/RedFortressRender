#include "PostEffectFog.h"
#include "PostEffectBloom.h"

namespace NSRender
{

void PostEffectFog::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       // L"res\\shader\\PostEffectFog.fx",
                                       L"../x64/Debug/PostEffectFog.cso",
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_d3dEffect,
                                       NULL);
    assert(SUCCEEDED(hResult));

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texWork);

    m_d3dEffect->SetVector("g_FogColor", &m_fogColor);

    Common::AddDeviceLostResource(this);
}

LPDIRECT3DTEXTURE9 PostEffectFog::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                       LPDIRECT3DTEXTURE9 texRenderTargetZ,
                                       LPDIRECT3DTEXTURE9 texRenderTargetPos,
                                       const bool enableZ,
                                       const bool enableHeight)
{
    // 入力チェック（必要テクスチャが無ければパススルー）
    if (enableZ && texRenderTargetZ == NULL)
    {
        return renderTarget;
    }

    if (enableHeight && texRenderTargetPos == NULL)
    {
        return renderTarget;
    }

    // エフェクトパラメータ設定
    // 霧の強度等
    BOOL enableZBool = FALSE;
    if (enableZ)
    {
        enableZBool = TRUE;
    }
    BOOL enableHeightBool = FALSE;
    if (enableHeight)
    {
        enableHeightBool = TRUE;
    }
    m_d3dEffect->SetBool("g_EnableZ", enableZBool);
    m_d3dEffect->SetBool("g_EnableHeight", enableHeightBool);

    m_d3dEffect->SetFloat("g_IntensityZ", m_intensityZ);
    m_d3dEffect->SetFloat("g_IntensityHeight", m_intensityHeight);
    m_d3dEffect->SetFloat("g_HeightStart", m_heightStart);
    m_d3dEffect->SetFloat("g_PosRange", m_positionRange);
    m_d3dEffect->SetFloat("g_DepthDecodeNear", m_depthDecodeNear);
    m_d3dEffect->SetFloat("g_DepthDecodeFar", m_depthDecodeFar);
    m_d3dEffect->SetFloat("g_FogNear", m_fogNear);
    m_d3dEffect->SetFloat("g_FogFar", m_fogFar);

    // 霧色（必要なら setter を後で追加してください）
    D3DXVECTOR4 fogColor(0.72f, 0.78f, 0.86f, 1.0f);
    m_d3dEffect->SetVector("g_FogColor", &fogColor);

    // 参照テクスチャ
    m_d3dEffect->SetTexture("g_ZTex", texRenderTargetZ);
    m_d3dEffect->SetTexture("g_PosTex", texRenderTargetPos);

    // フルスクリーンクアッドで描画
    DrawFullscreenQuad(renderTarget, m_texWork, "TechFog");

    return m_texWork;
}

void PostEffectFog::Finalize()
{
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectFog::SetIntensityZ(const float arg)
{
    m_intensityZ = arg;
}

void PostEffectFog::SetIntensityHeight(const float arg)
{
    m_intensityHeight = arg;
}

void PostEffectFog::SetHeightStart(const float arg)
{
    m_heightStart = arg;
}

void PostEffectFog::SetPositionRange(const float positionRange)
{
    m_positionRange = 1.0f;
    if (positionRange > 1.0f)
    {
        m_positionRange = positionRange;
    }
}

void PostEffectFog::SetDepthDecodeRange(const float nearPlane, const float farPlane)
{
    m_depthDecodeNear = nearPlane;
    m_depthDecodeFar = farPlane;
}

void PostEffectFog::SetFogDepthRange(const float nearPlane, const float farPlane)
{
    m_fogNear = nearPlane;
    m_fogFar = farPlane;
}

void PostEffectFog::SetFogColor(const D3DXCOLOR& color)
{
    m_fogColor = D3DXVECTOR4(color.r, color.g, color.b, color.a);
}

void PostEffectFog::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                       LPDIRECT3DTEXTURE9 texTarget,
                                       const std::string& technique)
{
    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);

    float texelSize[2] = { 1.0f / Common::ScreenW(), 1.0f / Common::ScreenH() };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] = { };

    quad[0].x   = -0.5f;
    quad[0].y   = -0.5f;
    quad[0].z   = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u   = 0.0f;
    quad[0].v   = 0.0f;

    quad[1].x   = -0.5f + Common::ScreenW();
    quad[1].y   = -0.5f;
    quad[1].z   = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u   = 1.0f;
    quad[1].v   = 0.0f;

    quad[2].x   = -0.5f;
    quad[2].y   = -0.5f + Common::ScreenH();
    quad[2].z   = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u   = 0.0f;
    quad[2].v   = 1.0f;

    quad[3].x   = -0.5f + Common::ScreenW();
    quad[3].y   = -0.5f + Common::ScreenH();
    quad[3].z   = 0.0f;
    quad[3].rhw = 1.0f;
    quad[3].u   = 1.0f;
    quad[3].v   = 1.0f;

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    Common::D3DDevice()->BeginScene();
    m_d3dEffect->Begin(NULL, 0);
    m_d3dEffect->BeginPass(0);

    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));

    m_d3dEffect->EndPass();
    m_d3dEffect->End();
    Common::D3DDevice()->EndScene();

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}

void PostEffectFog::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
    SAFE_RELEASE(m_texWork);
}

void PostEffectFog::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texWork);
}

}
