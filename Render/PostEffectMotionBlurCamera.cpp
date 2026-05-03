#include "PostEffectMotionBlurCamera.h"

#include <algorithm>

#include "Camera.h"

namespace NSRender
{

namespace
{
int ClampMotionBlurQuality(const int quality)
{
    return (std::max)(1, (std::min)(quality, 8));
}
}

void PostEffectMotionBlurCamera::Initialize()
{
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               L"../x64/Debug/PostEffectMotionBlurCamera.cso",
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_d3dEffect,
                                               NULL);
    assert(SUCCEEDED(hResult));

    D3DXMatrixIdentity(&m_prevViewProj);
    CreateTexture();
    Common::AddDeviceLostResource(this);
}

void PostEffectMotionBlurCamera::Finalize()
{
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectMotionBlurCamera::CreateTexture()
{
    HRESULT hResult = D3DXCreateTexture(Common::D3DDevice(),
                                        Common::ScreenW(),
                                        Common::ScreenH(),
                                        1,
                                        D3DUSAGE_RENDERTARGET,
                                        D3DFMT_A16B16G16R16F,
                                        D3DPOOL_DEFAULT,
                                        &m_texWork);
    assert(SUCCEEDED(hResult));
}

LPDIRECT3DTEXTURE9 PostEffectMotionBlurCamera::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                                    LPDIRECT3DTEXTURE9 depthTexture)
{
    DrawFullscreenQuad(renderTarget, depthTexture, m_texWork);
    UpdateFrameMatrices();
    return m_texWork;
}

void PostEffectMotionBlurCamera::UpdateFrameMatrices()
{
    m_prevViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_hasPrevViewProj = true;
}

void PostEffectMotionBlurCamera::SetQuality(const int quality)
{
    m_quality = ClampMotionBlurQuality(quality);
}

int PostEffectMotionBlurCamera::GetQuality() const
{
    return m_quality;
}

void PostEffectMotionBlurCamera::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                                    LPDIRECT3DTEXTURE9 depthTexture,
                                                    LPDIRECT3DTEXTURE9 texTarget)
{
    D3DXMATRIX currentViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    D3DXMATRIX invCurrentViewProj { };
    D3DXMATRIX identity { };
    D3DXMatrixIdentity(&identity);
    if (D3DXMatrixInverse(&invCurrentViewProj, NULL, &currentViewProj) == NULL)
    {
        invCurrentViewProj = identity;
    }

    if (!m_hasPrevViewProj)
    {
        m_prevViewProj = currentViewProj;
    }

    const D3DXVECTOR4 texelSize(1.0f / static_cast<float>(Common::ScreenW()),
                                1.0f / static_cast<float>(Common::ScreenH()),
                                static_cast<float>(Common::ScreenW()),
                                static_cast<float>(Common::ScreenH()));

    const float blurScale = 0.5f + static_cast<float>(m_quality) * 0.25f;
    const float maxBlurPixels = 4.0f + static_cast<float>(m_quality) * 4.0f;
    const int sampleCount = 2 + m_quality * 2;

    m_d3dEffect->SetTexture("texture1", texSource);
    m_d3dEffect->SetTexture("depthTexture", depthTexture);
    m_d3dEffect->SetMatrix("g_matInvCurrentViewProj", &invCurrentViewProj);
    m_d3dEffect->SetMatrix("g_matPrevViewProj", &m_prevViewProj);
    m_d3dEffect->SetFloat("g_fBlurScale", blurScale);
    m_d3dEffect->SetFloat("g_fMaxBlurPixels", maxBlurPixels);
    m_d3dEffect->SetInt("g_iSampleCount", sampleCount);
    m_d3dEffect->SetVector("g_vTexelSize", &texelSize);
    m_d3dEffect->SetFloat("g_fNear", Camera::GetNear());
    m_d3dEffect->SetFloat("g_fFar", Camera::GetFar());

    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);
    m_d3dEffect->SetTechnique("Technique1");

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

void PostEffectMotionBlurCamera::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
    SAFE_RELEASE(m_texWork);
}

void PostEffectMotionBlurCamera::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();
    CreateTexture();
}

}
