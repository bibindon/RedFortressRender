#include "PostEffectHeightFog.h"
#include "Camera.h"

#include "Util.h"

namespace NSRender
{

void PostEffectHeightFog::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectHeightFog.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               effectPath.c_str(),
                                               nullptr,
                                               nullptr,
                                               D3DXSHADER_DEBUG,
                                               nullptr,
                                               &m_d3dEffect,
                                               nullptr);
    assert(SUCCEEDED(hResult));

    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }

    m_isInitialized = true;
}

void PostEffectHeightFog::Draw(LPDIRECT3DTEXTURE9 texSource,
                               LPDIRECT3DTEXTURE9 texTarget,
                               LPDIRECT3DTEXTURE9 texRenderTargetPos)
{
    if (!m_isInitialized || m_d3dEffect == nullptr)
    {
        return;
    }

    if (texSource == nullptr || texTarget == nullptr || texRenderTargetPos == nullptr)
    {
        return;
    }

    m_d3dEffect->SetFloat("g_IntensityHeight", m_intensity);
    m_d3dEffect->SetFloat("g_HeightStart", m_startHeight);
    m_d3dEffect->SetFloat("g_HeightMax", m_maxHeight);
    m_d3dEffect->SetFloat("g_DistanceStart", m_distanceStart);
    m_d3dEffect->SetFloat("g_DistanceMax", m_distanceMax);
    m_d3dEffect->SetFloat("g_PosRange", m_positionRange);
    const D3DXVECTOR3 eye = Camera::GetEyePos();
    const D3DXVECTOR4 cameraPos(eye.x, eye.y, eye.z, 1.0f);
    m_d3dEffect->SetVector("g_CameraPos", &cameraPos);
    m_d3dEffect->SetVector("g_FogColor", &m_fogColor);
    m_d3dEffect->SetTexture("g_PosTex", texRenderTargetPos);

    DrawFullscreenQuad(texSource, texTarget, "TechHeightFog");
}

void PostEffectHeightFog::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectHeightFog::SetIntensity(const float intensity)
{
    m_intensity = intensity;
}

void PostEffectHeightFog::SetStartHeight(const float startHeight)
{
    m_startHeight = startHeight;
}

void PostEffectHeightFog::SetMaxHeight(const float maxHeight)
{
    m_maxHeight = maxHeight;
}

void PostEffectHeightFog::SetDistanceStart(const float distanceStart)
{
    m_distanceStart = distanceStart;
}

void PostEffectHeightFog::SetDistanceMax(const float distanceMax)
{
    m_distanceMax = distanceMax;
}

void PostEffectHeightFog::SetFogColor(const D3DXCOLOR& color)
{
    m_fogColor = D3DXVECTOR4(color.r, color.g, color.b, color.a);
}

void PostEffectHeightFog::SetPositionRange(const float positionRange)
{
    m_positionRange = 1.0f;
    if (positionRange > 1.0f)
    {
        m_positionRange = positionRange;
    }
}

void PostEffectHeightFog::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == nullptr)
    {
        return;
    }

    if (m_d3dEffect != nullptr)
    {
        m_d3dEffect->OnLostDevice();
    }
}

void PostEffectHeightFog::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == nullptr)
    {
        return;
    }

    if (m_d3dEffect != nullptr)
    {
        m_d3dEffect->OnResetDevice();
    }
}

void PostEffectHeightFog::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                             LPDIRECT3DTEXTURE9 texTarget,
                                             const std::string& technique)
{
    LPDIRECT3DSURFACE9 pSceneRT = nullptr;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

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

}
