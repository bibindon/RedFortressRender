#include "PostEffectTAA.h"

#include "Util.h"

namespace NSRender
{

void PostEffectTAA::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    CreateResources();

    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }
    m_isInitialized = true;
}

void PostEffectTAA::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }

    ReleaseResources();
    m_isInitialized = false;
    m_hasHistory = false;
}

void PostEffectTAA::CreateResources()
{
    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectTAA.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               effectPath.c_str(),
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_d3dEffect,
                                               NULL);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_historyTexture);
    assert(SUCCEEDED(hResult));
}

void PostEffectTAA::ReleaseResources()
{
    SAFE_RELEASE(m_historyTexture);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectTAA::Draw(LPDIRECT3DTEXTURE9 texSource,
                         LPDIRECT3DTEXTURE9 texTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL || m_historyTexture == NULL)
    {
        return;
    }

    DrawFullscreenQuad(texSource, texTarget);
    CopyTexture(texTarget, m_historyTexture);
    m_hasHistory = true;
}

void PostEffectTAA::ResetHistory()
{
    m_hasHistory = false;
}

void PostEffectTAA::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                       LPDIRECT3DTEXTURE9 texTarget)
{
    LPDIRECT3DSURFACE9 renderTargetSurface = NULL;
    texTarget->GetSurfaceLevel(0, &renderTargetSurface);
    Common::D3DDevice()->SetRenderTarget(0, renderTargetSurface);
    SAFE_RELEASE(renderTargetSurface);

    float texelSize[2] =
    {
        1.0f / static_cast<float>(Common::ScreenW()),
        1.0f / static_cast<float>(Common::ScreenH())
    };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);
    m_d3dEffect->SetBool("g_HistoryValid", m_hasHistory ? TRUE : FALSE);
    m_d3dEffect->SetTexture("g_CurrentTex", texSource);
    m_d3dEffect->SetTexture("g_HistoryTex", m_historyTexture);
    m_d3dEffect->SetTechnique("Technique1");

    ScreenVertex quad[4] { };
    quad[0] = { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { -0.5f + Common::ScreenW(), -0.5f, 0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { -0.5f, -0.5f + Common::ScreenH(), 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { -0.5f + Common::ScreenW(), -0.5f + Common::ScreenH(), 0.0f, 1.0f, 1.0f, 1.0f };

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

void PostEffectTAA::CopyTexture(LPDIRECT3DTEXTURE9 texSource,
                                LPDIRECT3DTEXTURE9 texTarget)
{
    LPDIRECT3DSURFACE9 sourceSurface = NULL;
    LPDIRECT3DSURFACE9 targetSurface = NULL;

    texSource->GetSurfaceLevel(0, &sourceSurface);
    texTarget->GetSurfaceLevel(0, &targetSurface);
    const HRESULT hResult = Common::D3DDevice()->StretchRect(sourceSurface,
                                                             NULL,
                                                             targetSurface,
                                                             NULL,
                                                             D3DTEXF_NONE);
    assert(SUCCEEDED(hResult));

    SAFE_RELEASE(targetSurface);
    SAFE_RELEASE(sourceSurface);
}

void PostEffectTAA::OnDeviceLost()
{
    if (!m_isInitialized)
    {
        return;
    }

    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnLostDevice();
    }
    ReleaseResources();
    m_hasHistory = false;
}

void PostEffectTAA::OnDeviceReset()
{
    if (!m_isInitialized)
    {
        return;
    }

    CreateResources();
    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnResetDevice();
    }
    m_hasHistory = false;
}

}
