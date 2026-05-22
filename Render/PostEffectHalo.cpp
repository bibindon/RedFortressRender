#include "PostEffectHalo.h"

#include <algorithm>

#include "Util.h"

namespace NSRender
{

void PostEffectHalo::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectHalo.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
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

int PostEffectHalo::ComputeTextureWidth() const
{
    return (std::max)(1, Common::ScreenW() / HALO_TEXTURE_DIVISOR);
}

int PostEffectHalo::ComputeTextureHeight() const
{
    return (std::max)(1, Common::ScreenH() / HALO_TEXTURE_DIVISOR);
}

void PostEffectHalo::CreateTexture()
{
    ReleaseTextures();

    D3DXCreateTexture(Common::D3DDevice(),
                      ComputeTextureWidth(),
                      ComputeTextureHeight(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBright);

    D3DXCreateTexture(Common::D3DDevice(),
                      ComputeTextureWidth(),
                      ComputeTextureHeight(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texHalo);
}

void PostEffectHalo::Draw(LPDIRECT3DTEXTURE9 texSource,
                          LPDIRECT3DTEXTURE9 texTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->SetFloat("g_Threshold", m_threshold);
    DrawFullscreenQuad(texSource, m_texBright, "BrightPass");
    DrawFullscreenQuad(m_texBright, m_texHalo, "HaloPass");
    DrawCombineQuad(texSource, texTarget);
}

void PostEffectHalo::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }

    ReleaseTextures();
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectHalo::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                        LPDIRECT3DTEXTURE9 texTarget,
                                        const std::string& technique)
{
    if (texSource == NULL || texTarget == NULL)
    {
        return;
    }

    D3DSURFACE_DESC sourceDesc { };
    D3DSURFACE_DESC targetDesc { };
    texSource->GetLevelDesc(0, &sourceDesc);
    texTarget->GetLevelDesc(0, &targetDesc);

    D3DVIEWPORT9 oldViewport { };
    Common::D3DDevice()->GetViewport(&oldViewport);
    D3DVIEWPORT9 targetViewport { };
    targetViewport.X = 0;
    targetViewport.Y = 0;
    targetViewport.Width = targetDesc.Width;
    targetViewport.Height = targetDesc.Height;
    targetViewport.MinZ = 0.0f;
    targetViewport.MaxZ = 1.0f;

    LPDIRECT3DSURFACE9 rt = NULL;
    texTarget->GetSurfaceLevel(0, &rt);
    Common::D3DDevice()->SetRenderTarget(0, rt);
    Common::D3DDevice()->SetViewport(&targetViewport);
    SAFE_RELEASE(rt);

    Common::D3DDevice()->SetVertexShader(NULL);
    Common::D3DDevice()->SetVertexDeclaration(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);
    const float texelSize[2] =
    {
        1.0f / static_cast<float>(sourceDesc.Width),
        1.0f / static_cast<float>(sourceDesc.Height)
    };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] { };
    quad[0] = { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { -0.5f + static_cast<float>(targetDesc.Width), -0.5f, 0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { -0.5f, -0.5f + static_cast<float>(targetDesc.Height), 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { -0.5f + static_cast<float>(targetDesc.Width), -0.5f + static_cast<float>(targetDesc.Height), 0.0f, 1.0f, 1.0f, 1.0f };

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
    Common::D3DDevice()->SetViewport(&oldViewport);
}

void PostEffectHalo::DrawCombineQuad(LPDIRECT3DTEXTURE9 texScene,
                                     LPDIRECT3DTEXTURE9 texTarget)
{
    if (texScene == NULL || texTarget == NULL)
    {
        return;
    }

    D3DSURFACE_DESC targetDesc { };
    texTarget->GetLevelDesc(0, &targetDesc);

    D3DVIEWPORT9 oldViewport { };
    Common::D3DDevice()->GetViewport(&oldViewport);
    D3DVIEWPORT9 targetViewport { };
    targetViewport.X = 0;
    targetViewport.Y = 0;
    targetViewport.Width = targetDesc.Width;
    targetViewport.Height = targetDesc.Height;
    targetViewport.MinZ = 0.0f;
    targetViewport.MaxZ = 1.0f;

    LPDIRECT3DSURFACE9 rt = NULL;
    texTarget->GetSurfaceLevel(0, &rt);
    Common::D3DDevice()->SetRenderTarget(0, rt);
    Common::D3DDevice()->SetViewport(&targetViewport);
    SAFE_RELEASE(rt);

    Common::D3DDevice()->SetVertexShader(NULL);
    Common::D3DDevice()->SetVertexDeclaration(NULL);

    m_d3dEffect->SetTechnique("Combine");
    m_d3dEffect->SetTexture("g_SceneTex", texScene);
    m_d3dEffect->SetTexture("g_HaloTex", m_texHalo);
    m_d3dEffect->SetFloat("g_HaloIntensity", m_intensity);

    ScreenVertex quad[4] { };
    quad[0] = { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { -0.5f + static_cast<float>(targetDesc.Width), -0.5f, 0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { -0.5f, -0.5f + static_cast<float>(targetDesc.Height), 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { -0.5f + static_cast<float>(targetDesc.Width), -0.5f + static_cast<float>(targetDesc.Height), 0.0f, 1.0f, 1.0f, 1.0f };

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
    Common::D3DDevice()->SetViewport(&oldViewport);
}

void PostEffectHalo::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnLostDevice();
    ReleaseTextures();
}

void PostEffectHalo::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnResetDevice();
    CreateTexture();
}

void PostEffectHalo::ReleaseTextures()
{
    SAFE_RELEASE(m_texBright);
    SAFE_RELEASE(m_texHalo);
}

}
