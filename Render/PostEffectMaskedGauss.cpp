#include "PostEffectMaskedGauss.h"

#include <Shlwapi.h>

#include "Util.h"

#pragma comment(lib, "Shlwapi.lib")

namespace NSRender
{

void PostEffectMaskedGauss::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectMaskedGaussian.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               effectPath.c_str(),
                                               nullptr,
                                               nullptr,
                                               D3DXSHADER_DEBUG,
                                               nullptr,
                                               &m_d3dEffect,
                                               nullptr);
    assert(SUCCEEDED(hResult));

    CreateWorkTextures();
    LoadMaskTexture();
    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }
    m_isInitialized = true;
}

void PostEffectMaskedGauss::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                 LPDIRECT3DTEXTURE9 texTarget)
{
    if (!m_isInitialized || m_d3dEffect == nullptr)
    {
        return;
    }

    if (renderTarget == nullptr || texTarget == nullptr || m_texMask == nullptr)
    {
        return;
    }

    m_d3dEffect->SetBool("g_bFilterON", TRUE);
    m_d3dEffect->SetInt("g_sampleSize", m_sampleSize);

    DrawFullscreenQuad(renderTarget, m_texBlurWork, "GaussianH");
    DrawFullscreenQuad(m_texBlurWork, m_texBlurResult, "GaussianH");
    DrawFullscreenQuad(m_texBlurResult, m_texBlurWork, "GaussianV");
    DrawFullscreenQuad(m_texBlurWork, m_texBlurResult, "GaussianV");
    DrawCompositeQuad(m_texBlurResult, renderTarget, texTarget);
}

void PostEffectMaskedGauss::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    SAFE_RELEASE(m_texMask);
    ReleaseWorkTextures();
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectMaskedGauss::SetSampleSize(const int sampleSize)
{
    m_sampleSize = sampleSize;
}

void PostEffectMaskedGauss::SetMaskPath(const std::wstring& maskPath)
{
    m_maskPath = maskPath;
    if (m_isInitialized)
    {
        LoadMaskTexture();
    }
}

void PostEffectMaskedGauss::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == nullptr)
    {
        return;
    }

    if (m_d3dEffect != nullptr)
    {
        m_d3dEffect->OnLostDevice();
    }
    ReleaseWorkTextures();
}

void PostEffectMaskedGauss::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == nullptr)
    {
        return;
    }

    if (m_d3dEffect != nullptr)
    {
        m_d3dEffect->OnResetDevice();
    }
    CreateWorkTextures();
    LoadMaskTexture();
}

void PostEffectMaskedGauss::CreateWorkTextures()
{
    ReleaseWorkTextures();

    HRESULT hResult = D3DXCreateTexture(Common::D3DDevice(),
                                        Common::ScreenW(),
                                        Common::ScreenH(),
                                        1,
                                        D3DUSAGE_RENDERTARGET,
                                        D3DFMT_A16B16G16R16F,
                                        D3DPOOL_DEFAULT,
                                        &m_texBlurWork);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_texBlurResult);
    assert(SUCCEEDED(hResult));
}

void PostEffectMaskedGauss::ReleaseWorkTextures()
{
    SAFE_RELEASE(m_texBlurWork);
    SAFE_RELEASE(m_texBlurResult);
}

void PostEffectMaskedGauss::LoadMaskTexture()
{
    SAFE_RELEASE(m_texMask);

    if (m_maskPath.empty())
    {
        return;
    }

    std::wstring resolvedPath = m_maskPath;
    if (PathIsRelativeW(resolvedPath.c_str()))
    {
        resolvedPath = Util::GetExeDir() + resolvedPath;
    }

    if (!PathFileExistsW(resolvedPath.c_str()))
    {
        return;
    }

    const HRESULT hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                      resolvedPath.c_str(),
                                                      &m_texMask);
    if (FAILED(hResult))
    {
        SAFE_RELEASE(m_texMask);
    }
}

void PostEffectMaskedGauss::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                               LPDIRECT3DTEXTURE9 texTarget,
                                               const std::string& technique)
{
    LPDIRECT3DSURFACE9 renderTargetSurface = nullptr;
    texTarget->GetSurfaceLevel(0, &renderTargetSurface);
    Common::D3DDevice()->SetRenderTarget(0, renderTargetSurface);
    SAFE_RELEASE(renderTargetSurface);

    Common::D3DDevice()->SetVertexShader(nullptr);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);

    const float texelSize[2] = { 1.0f / Common::ScreenW(), 1.0f / Common::ScreenH() };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] { };

    quad[0].x = -0.5f;
    quad[0].y = -0.5f;
    quad[0].z = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u = 0.0f;
    quad[0].v = 0.0f;

    quad[1].x = -0.5f + Common::ScreenW();
    quad[1].y = -0.5f;
    quad[1].z = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u = 1.0f;
    quad[1].v = 0.0f;

    quad[2].x = -0.5f;
    quad[2].y = -0.5f + Common::ScreenH();
    quad[2].z = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u = 0.0f;
    quad[2].v = 1.0f;

    quad[3].x = -0.5f + Common::ScreenW();
    quad[3].y = -0.5f + Common::ScreenH();
    quad[3].z = 0.0f;
    quad[3].rhw = 1.0f;
    quad[3].u = 1.0f;
    quad[3].v = 1.0f;

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    Common::D3DDevice()->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
    Common::D3DDevice()->BeginScene();
    m_d3dEffect->Begin(nullptr, 0);
    m_d3dEffect->BeginPass(0);

    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));

    m_d3dEffect->EndPass();
    m_d3dEffect->End();
    Common::D3DDevice()->EndScene();

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}

void PostEffectMaskedGauss::DrawCompositeQuad(LPDIRECT3DTEXTURE9 texBlurred,
                                              LPDIRECT3DTEXTURE9 texOriginal,
                                              LPDIRECT3DTEXTURE9 texTarget)
{
    LPDIRECT3DSURFACE9 renderTargetSurface = nullptr;
    texTarget->GetSurfaceLevel(0, &renderTargetSurface);
    Common::D3DDevice()->SetRenderTarget(0, renderTargetSurface);
    SAFE_RELEASE(renderTargetSurface);

    Common::D3DDevice()->SetVertexShader(nullptr);

    m_d3dEffect->SetTechnique("CompositeMasked");
    m_d3dEffect->SetTexture("g_SrcTex", texBlurred);
    m_d3dEffect->SetTexture("g_SrcTex2", texOriginal);
    m_d3dEffect->SetTexture("g_MaskTex", m_texMask);

    const float texelSize[2] = { 1.0f / Common::ScreenW(), 1.0f / Common::ScreenH() };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] { };

    quad[0].x = -0.5f;
    quad[0].y = -0.5f;
    quad[0].z = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u = 0.0f;
    quad[0].v = 0.0f;

    quad[1].x = -0.5f + Common::ScreenW();
    quad[1].y = -0.5f;
    quad[1].z = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u = 1.0f;
    quad[1].v = 0.0f;

    quad[2].x = -0.5f;
    quad[2].y = -0.5f + Common::ScreenH();
    quad[2].z = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u = 0.0f;
    quad[2].v = 1.0f;

    quad[3].x = -0.5f + Common::ScreenW();
    quad[3].y = -0.5f + Common::ScreenH();
    quad[3].z = 0.0f;
    quad[3].rhw = 1.0f;
    quad[3].u = 1.0f;
    quad[3].v = 1.0f;

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    Common::D3DDevice()->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
    Common::D3DDevice()->BeginScene();
    m_d3dEffect->Begin(nullptr, 0);
    m_d3dEffect->BeginPass(0);

    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));

    m_d3dEffect->EndPass();
    m_d3dEffect->End();
    Common::D3DDevice()->EndScene();

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}

}
