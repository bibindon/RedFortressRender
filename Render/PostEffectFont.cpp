#include "PostEffectFont.h"

#include "Util.h"

namespace NSRender
{

void PostEffectFont::Initialize()
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

void PostEffectFont::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }

    SAFE_RELEASE(m_blurTexture);
    SAFE_RELEASE(m_shadowTexture);
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectFont::BeginShadowPass()
{
    if (!m_isInitialized)
    {
        Initialize();
    }

    if (m_shadowTexture == NULL)
    {
        return;
    }

    LPDIRECT3DSURFACE9 renderTarget = NULL;
    m_shadowTexture->GetSurfaceLevel(0, &renderTarget);
    Common::D3DDevice()->SetRenderTarget(0, renderTarget);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
    SAFE_RELEASE(renderTarget);
}

void PostEffectFont::DrawBlurredShadow()
{
    if (!m_isInitialized || m_shadowTexture == NULL || m_blurTexture == NULL)
    {
        return;
    }

    LPDIRECT3DSURFACE9 blurTarget = NULL;
    m_blurTexture->GetSurfaceLevel(0, &blurTarget);
    DrawFullscreenQuad(m_shadowTexture, blurTarget, "GaussianH", false);
    SAFE_RELEASE(blurTarget);

    LPDIRECT3DSURFACE9 backBuffer = NULL;
    Common::D3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    DrawFullscreenQuad(m_blurTexture, backBuffer, "GaussianV", true);
    SAFE_RELEASE(backBuffer);
}

void PostEffectFont::SetGaussianSampleSize(const int sampleSize)
{
    m_gaussianSampleSize = NormalizeGaussianSampleSize(sampleSize);
}

void PostEffectFont::OnDeviceLost()
{
    SAFE_RELEASE(m_blurTexture);
    SAFE_RELEASE(m_shadowTexture);
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectFont::OnDeviceReset()
{
    CreateResources();
    m_isInitialized = true;
}

void PostEffectFont::CreateResources()
{
    HRESULT hResult = E_FAIL;

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectFont.cso";
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       effectPath.c_str(),
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &m_d3dEffect,
                                       NULL);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_shadowTexture);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_blurTexture);
    assert(SUCCEEDED(hResult));
}

void PostEffectFont::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                        LPDIRECT3DSURFACE9 renderTarget,
                                        const char* technique,
                                        const bool enableAlphaBlend)
{
    if (m_d3dEffect == NULL || renderTarget == NULL)
    {
        return;
    }

    Common::D3DDevice()->SetRenderTarget(0, renderTarget);
    Common::D3DDevice()->SetVertexShader(NULL);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, enableAlphaBlend ? TRUE : FALSE);
    Common::D3DDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    Common::D3DDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    if (!enableAlphaBlend)
    {
        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
    }

    m_d3dEffect->SetTechnique(technique);
    m_d3dEffect->SetTexture("g_SrcTex", texSource);
    m_d3dEffect->SetInt("g_sampleSize", m_gaussianSampleSize);

    float texelSize[2] =
    {
        1.0f / static_cast<float>(Common::ScreenW()),
        1.0f / static_cast<float>(Common::ScreenH())
    };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] { };

    quad[0] = { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { -0.5f + Common::ScreenW(), -0.5f, 0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { -0.5f, -0.5f + Common::ScreenH(), 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { -0.5f + Common::ScreenW(), -0.5f + Common::ScreenH(), 0.0f, 1.0f, 1.0f, 1.0f };

    Common::D3DDevice()->BeginScene();
    m_d3dEffect->Begin(NULL, 0);
    m_d3dEffect->BeginPass(0);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
    m_d3dEffect->EndPass();
    m_d3dEffect->End();
    Common::D3DDevice()->EndScene();

    Common::D3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}

int PostEffectFont::NormalizeGaussianSampleSize(const int sampleSize)
{
    return (std::max)(1, (std::min)(sampleSize, 21));
}

}
