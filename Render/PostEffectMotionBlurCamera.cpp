#include "PostEffectMotionBlurCamera.h"

#include <algorithm>
#include <cmath>

#include "Camera.h"

#include "Util.h"

namespace NSRender
{

namespace
{
int ClampMotionBlurQuality(const int quality)
{
    return (std::max)(1, (std::min)(quality, 8));
}

float ClampMotionBlurMaxBlurPixels(const float maxBlurPixels)
{
    return (std::max)(1.0f, (std::min)(maxBlurPixels, 64.0f));
}

int ClampMotionBlurSampleCount(const int sampleCount)
{
    return (std::max)(2, (std::min)(sampleCount, 21));
}
}

void PostEffectMotionBlurCamera::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectMotionBlurCamera.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               effectPath.c_str(),
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_d3dEffect,
                                               NULL);
    assert(SUCCEEDED(hResult));

    D3DXMatrixIdentity(&m_prevViewProj);
    m_prevFrameTick = GetTickCount64();
    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }
    m_isInitialized = true;
}

void PostEffectMotionBlurCamera::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectMotionBlurCamera::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                      LPDIRECT3DTEXTURE9 depthTexture,
                                      LPDIRECT3DTEXTURE9 texTarget,
                                      bool& applied)
{
    applied = false;

    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    const D3DXMATRIX currentViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_frameMotionScale = UpdateFrameMotionScale();
    if (!ShouldApplyMotionBlur(currentViewProj))
    {
        UpdateFrameMatrices();
        return;
    }

    DrawFullscreenQuad(renderTarget, depthTexture, texTarget);
    UpdateFrameMatrices();
    applied = true;
}

void PostEffectMotionBlurCamera::UpdateFrameMatrices()
{
    m_prevViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_hasPrevViewProj = true;
}

void PostEffectMotionBlurCamera::SetQuality(const int quality)
{
    m_quality = ClampMotionBlurQuality(quality);
    m_maxBlurPixels = static_cast<float>(m_quality) * 6.0f;
    m_sampleCount = ClampMotionBlurSampleCount(5 + m_quality * 2);
}

int PostEffectMotionBlurCamera::GetQuality() const
{
    return m_quality;
}

void PostEffectMotionBlurCamera::SetMaxBlurPixels(const float maxBlurPixels)
{
    m_maxBlurPixels = ClampMotionBlurMaxBlurPixels(maxBlurPixels);
}

float PostEffectMotionBlurCamera::GetMaxBlurPixels() const
{
    return m_maxBlurPixels;
}

void PostEffectMotionBlurCamera::SetSampleCount(const int sampleCount)
{
    m_sampleCount = ClampMotionBlurSampleCount(sampleCount);
}

int PostEffectMotionBlurCamera::GetSampleCount() const
{
    return m_sampleCount;
}

bool PostEffectMotionBlurCamera::ShouldApplyMotionBlur(const D3DXMATRIX& currentViewProj)
{
    if (!m_hasPrevViewProj)
    {
        m_prevViewProj = currentViewProj;
        return false;
    }

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            if (fabsf(currentViewProj(row, col) - m_prevViewProj(row, col)) > 0.0001f)
            {
                return true;
            }
        }
    }

    return false;
}

float PostEffectMotionBlurCamera::UpdateFrameMotionScale()
{
    const ULONGLONG currentTick = GetTickCount64();
    float deltaSeconds = static_cast<float>(currentTick - m_prevFrameTick) / 1000.0f;
    m_prevFrameTick = currentTick;

    if (deltaSeconds > 0.1f)
    {
        deltaSeconds = 0.1f;
    }

    static const float kMotionVectorFrameSeconds = 1.0f / 60.0f;
    return (std::min)(1.0f, kMotionVectorFrameSeconds / (std::max)(deltaSeconds, 0.0001f));
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

    m_d3dEffect->SetTexture("texture1", texSource);
    m_d3dEffect->SetTexture("depthTexture", depthTexture);
    m_d3dEffect->SetMatrix("g_matInvCurrentViewProj", &invCurrentViewProj);
    m_d3dEffect->SetMatrix("g_matPrevViewProj", &m_prevViewProj);
    m_d3dEffect->SetFloat("g_fBlurScale", 2.0f);
    m_d3dEffect->SetFloat("g_fMaxBlurPixels", m_maxBlurPixels);
    m_d3dEffect->SetInt("g_iSampleCount", m_sampleCount);
    m_d3dEffect->SetInt("g_iMotionBlurEnabled", 1);
    m_d3dEffect->SetInt("g_iDebugGridEnabled", 0);
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
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnLostDevice();
}

void PostEffectMotionBlurCamera::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnResetDevice();
}

}
