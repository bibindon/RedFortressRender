#include "PostEffectBloom.h"

#include "Util.h"

namespace NSRender
{

void PostEffectBloom::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectBloom.cso";
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       effectPath.c_str(),
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_d3dEffect,
                                       NULL);
    assert(SUCCEEDED(hResult));

    CreateTexture();

    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }

    m_isInitialized = true;
}

void PostEffectBloom::CreateTexture()
{
    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texBright);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texBlurH);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texBlurH2);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texBlurV);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texBlurV2);
}

void PostEffectBloom::Draw(LPDIRECT3DTEXTURE9 renderSource,
                           LPDIRECT3DTEXTURE9 renderTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->SetInt("g_sampleSize", 101);
    m_d3dEffect->SetFloat("g_Threshold", m_threshold);

    // ------------------------------------------------------------
    // (1) BrightPass : 入力 = renderSource, 出力 = m_texWork
    // ------------------------------------------------------------
    {
        m_d3dEffect->SetTexture("g_SrcTex", renderSource);

        float texelSize[2] = { 1.0f / Common::ScreenW(), 1.0f / Common::ScreenH() };
        m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

        DrawFullscreenQuad(m_texBright, "BrightPass");
    }

    // ------------------------------------------------------------
    // (2) Horizontal Blur : 入力 = m_texBright, 出力 = m_texBlurH
    // ------------------------------------------------------------
    {
        // 横方向
        float dir[4] = { 1, 0, 0, 0 };
        m_d3dEffect->SetFloatArray("g_Direction", dir, 4);

        m_d3dEffect->SetTexture("g_SrcTex", m_texBright);
        DrawFullscreenQuad(m_texBlurH, "Blur");

        m_d3dEffect->SetTexture("g_SrcTex", m_texBlurH);
        DrawFullscreenQuad(m_texBlurH2, "Blur");
    }

    // ------------------------------------------------------------
    // (3) Vertical Blur : 入力 = m_texBlurH, 出力 = m_texBlurV
    // ------------------------------------------------------------
    {
        // 縦方向
        float dir[4] = { 0, 1, 0, 0 };
        m_d3dEffect->SetFloatArray("g_Direction", dir, 4);

        m_d3dEffect->SetTexture("g_SrcTex", m_texBlurH2);
        DrawFullscreenQuad(m_texBlurV, "Blur");

        m_d3dEffect->SetTexture("g_SrcTex", m_texBlurV);
        DrawFullscreenQuad(m_texBlurV2, "Blur");
    }

    // ------------------------------------------------------------
    // (4) Combine : (renderSource + m_texBlurV) → renderTarget
    // ------------------------------------------------------------
    {
        m_d3dEffect->SetTexture("g_SceneTex", renderSource);
        m_d3dEffect->SetTexture("g_BlurTex", m_texBlurV2);

        DrawFullscreenQuad(renderTarget, "Combine");
    }
}

void PostEffectBloom::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    SAFE_RELEASE(m_texBright);
    SAFE_RELEASE(m_texBlurH);
    SAFE_RELEASE(m_texBlurH2);
    SAFE_RELEASE(m_texBlurV);
    SAFE_RELEASE(m_texBlurV2);
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectBloom::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                                         const std::string& technique)
{
    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());

    ScreenVertex quad[4] { };

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

void PostEffectBloom::SetThreshold(const float arg)
{
    m_threshold = arg;
}

void PostEffectBloom::SetIntensity(const float arg)
{
    m_intensity = arg;
}

void PostEffectBloom::SetSize(const float arg)
{
    m_size = arg;
}

void PostEffectBloom::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnLostDevice();
    SAFE_RELEASE(m_texBright);
    SAFE_RELEASE(m_texBlurH);
    SAFE_RELEASE(m_texBlurH2);
    SAFE_RELEASE(m_texBlurV);
    SAFE_RELEASE(m_texBlurV2);
}

void PostEffectBloom::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnResetDevice();
    CreateTexture();
}

}
