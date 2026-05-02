#include "PostEffectDepthOfField.h"

#include <algorithm>
#include <cfloat>

#include "Camera.h"
#include "PostEffectSSAO.h"

namespace NSRender
{

namespace
{
constexpr float POSITION_RANGE = PostEffectSSAO::Z_RANGE;
}

void PostEffectDepthOfField::Initialize()
{
    HRESULT hr = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                          L"../x64/Debug/PostEffectDepthOfField.cso",
                                          NULL,
                                          NULL,
                                          D3DXSHADER_DEBUG,
                                          NULL,
                                          &m_d3dEffect,
                                          NULL);
    assert(SUCCEEDED(hr));

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A16B16G16R16F,
                           D3DPOOL_DEFAULT,
                           &m_texWork);
    assert(SUCCEEDED(hr));

    CreateReadbackSurface();

    Common::AddDeviceLostResource(this);
}

LPDIRECT3DTEXTURE9 PostEffectDepthOfField::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                                LPDIRECT3DTEXTURE9 texRenderTargetPos)
{
    if (renderTarget == NULL || texRenderTargetPos == NULL)
    {
        return renderTarget;
    }

    const D3DXVECTOR3 cameraPos = Camera::GetEyePos();
    const D3DXVECTOR4 cameraPos4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);

    m_d3dEffect->SetVector("g_cameraPos", &cameraPos4);
    m_d3dEffect->SetFloat("g_focalDistanceMeters", m_focalDistance);
    m_d3dEffect->SetFloat("g_maxBlurDistanceMeters", m_maxBlurDistance);
    m_d3dEffect->SetFloat("g_focusBandHalfWidthMeters", m_focusBandHalfWidth);
    m_d3dEffect->SetFloat("g_blurRadiusPixels", m_blurRadiusPixels);
    m_d3dEffect->SetFloat("g_positionRange", POSITION_RANGE);
    m_d3dEffect->SetFloat("g_dofBlend", m_blend);

    DrawFullscreenQuad(renderTarget, texRenderTargetPos, m_texWork, "Technique1");
    return m_texWork;
}

void PostEffectDepthOfField::Finalize()
{
    SAFE_RELEASE(m_surfacePositionReadback);
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectDepthOfField::SetFocalDistance(float focalDistance)
{
    m_focalDistance = focalDistance;
}

void PostEffectDepthOfField::SetMaxBlurDistance(float maxBlurDistance)
{
    m_maxBlurDistance = maxBlurDistance;
}

void PostEffectDepthOfField::SetFocusBandHalfWidth(float focusBandHalfWidth)
{
    m_focusBandHalfWidth = focusBandHalfWidth;
}

void PostEffectDepthOfField::SetBlurRadiusPixels(float blurRadiusPixels)
{
    m_blurRadiusPixels = blurRadiusPixels;
}

void PostEffectDepthOfField::SetAutoActivationDistance(float autoActivationDistance)
{
    m_autoActivationDistance = (std::max)(0.1f, autoActivationDistance);
}

void PostEffectDepthOfField::SetBlend(float blend)
{
    m_blend = (std::max)(0.0f, (std::min)(blend, 1.0f));
}

float PostEffectDepthOfField::GetBlend() const
{
    return m_blend;
}

void PostEffectDepthOfField::UpdateAutoBlend(LPDIRECT3DTEXTURE9 texRenderTargetPos)
{
    const float nearestDistance = MeasureCenterNearestDistance(texRenderTargetPos);
    const float targetBlend = (nearestDistance < m_autoActivationDistance) ? 1.0f : 0.0f;

    const DWORD currentTick = GetTickCount();
    float deltaTime = 0.0f;
    if (m_lastAutoBlendTick != 0)
    {
        deltaTime = static_cast<float>(currentTick - m_lastAutoBlendTick) * (1.0f / 1000.0f);
    }
    m_lastAutoBlendTick = currentTick;

    const float blendStep = m_autoBlendSpeed * deltaTime;
    if (m_blend < targetBlend)
    {
        m_blend = (std::min)(m_blend + blendStep, targetBlend);
    }
    else
    {
        m_blend = (std::max)(m_blend - blendStep, targetBlend);
    }
}

void PostEffectDepthOfField::OnDeviceLost()
{
    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnLostDevice();
    }

    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_surfacePositionReadback);
}

void PostEffectDepthOfField::OnDeviceReset()
{
    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnResetDevice();
    }

    HRESULT hr = D3DXCreateTexture(Common::D3DDevice(),
                                   Common::ScreenW(),
                                   Common::ScreenH(),
                                   1,
                                   D3DUSAGE_RENDERTARGET,
                                   D3DFMT_A16B16G16R16F,
                                   D3DPOOL_DEFAULT,
                                   &m_texWork);
    assert(SUCCEEDED(hr));

    CreateReadbackSurface();
}

void PostEffectDepthOfField::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                                LPDIRECT3DTEXTURE9 texPosition,
                                                LPDIRECT3DTEXTURE9 texTarget,
                                                const std::string& technique)
{
    LPDIRECT3DSURFACE9 sceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &sceneRT);
    Common::D3DDevice()->SetRenderTarget(0, sceneRT);
    SAFE_RELEASE(sceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);
    m_d3dEffect->SetTexture("g_PositionTex", texPosition);

    const float texelSize[2] =
    {
        1.0f / static_cast<float>(Common::ScreenW()),
        1.0f / static_cast<float>(Common::ScreenH())
    };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] { };
    quad[0] = { 0.f,                              0.f,                               0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { static_cast<float>(Common::ScreenW()), 0.f,                   0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { 0.f,                              static_cast<float>(Common::ScreenH()), 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { static_cast<float>(Common::ScreenW()), static_cast<float>(Common::ScreenH()), 0.0f, 1.0f, 1.0f, 1.0f };

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

float PostEffectDepthOfField::MeasureCenterNearestDistance(LPDIRECT3DTEXTURE9 texRenderTargetPos)
{
    if (texRenderTargetPos == NULL || m_surfacePositionReadback == NULL)
    {
        return FLT_MAX;
    }

    LPDIRECT3DSURFACE9 sourceSurface = NULL;
    if (FAILED(texRenderTargetPos->GetSurfaceLevel(0, &sourceSurface)))
    {
        return FLT_MAX;
    }

    const HRESULT copyResult = Common::D3DDevice()->GetRenderTargetData(sourceSurface, m_surfacePositionReadback);
    SAFE_RELEASE(sourceSurface);
    if (FAILED(copyResult))
    {
        return FLT_MAX;
    }

    D3DLOCKED_RECT lockedRect { };
    if (FAILED(m_surfacePositionReadback->LockRect(&lockedRect, NULL, D3DLOCK_READONLY)))
    {
        return FLT_MAX;
    }

    const int screenW = Common::ScreenW();
    const int screenH = Common::ScreenH();
    const float centerX = static_cast<float>(screenW) * 0.5f;
    const float centerY = static_cast<float>(screenH) * 0.5f;
    const float radiusPxX = (static_cast<float>(screenW) * 0.5f) * m_autoCenterRadiusNdc;
    const float radiusPxY = (static_cast<float>(screenH) * 0.5f) * m_autoCenterRadiusNdc;
    const int minX = (std::max)(0, static_cast<int>(centerX - radiusPxX));
    const int maxX = (std::min)(screenW - 1, static_cast<int>(centerX + radiusPxX));
    const int minY = (std::max)(0, static_cast<int>(centerY - radiusPxY));
    const int maxY = (std::min)(screenH - 1, static_cast<int>(centerY + radiusPxY));
    const int sampleStep = 8;
    const D3DXVECTOR3 cameraPos = Camera::GetEyePos();

    float nearestDistance = FLT_MAX;

    for (int y = minY; y <= maxY; y += sampleStep)
    {
        const float normalizedY = (static_cast<float>(y) - centerY) / (std::max)(radiusPxY, 0.0001f);
        const BYTE* row = static_cast<const BYTE*>(lockedRect.pBits) + (lockedRect.Pitch * y);

        for (int x = minX; x <= maxX; x += sampleStep)
        {
            const float normalizedX = (static_cast<float>(x) - centerX) / (std::max)(radiusPxX, 0.0001f);
            if ((normalizedX * normalizedX) + (normalizedY * normalizedY) > 1.0f)
            {
                continue;
            }

            const auto* encodedPosition = reinterpret_cast<const D3DXFLOAT16*>(row + (x * sizeof(D3DXFLOAT16) * 4));
            FLOAT decodedPosition[4] { };
            D3DXFloat16To32Array(decodedPosition, encodedPosition, 4);
            if (decodedPosition[3] <= 0.0f)
            {
                continue;
            }

            const D3DXVECTOR3 worldPos(((decodedPosition[0] * 2.0f) - 1.0f) * POSITION_RANGE,
                                       ((decodedPosition[1] * 2.0f) - 1.0f) * POSITION_RANGE,
                                       ((decodedPosition[2] * 2.0f) - 1.0f) * POSITION_RANGE);
            const D3DXVECTOR3 toObject = worldPos - cameraPos;
            nearestDistance = (std::min)(nearestDistance, D3DXVec3Length(&toObject));
        }
    }

    m_surfacePositionReadback->UnlockRect();
    return nearestDistance;
}

void PostEffectDepthOfField::CreateReadbackSurface()
{
    SAFE_RELEASE(m_surfacePositionReadback);

    const HRESULT hr = Common::D3DDevice()->CreateOffscreenPlainSurface(Common::ScreenW(),
                                                                        Common::ScreenH(),
                                                                        D3DFMT_A16B16G16R16F,
                                                                        D3DPOOL_SYSTEMMEM,
                                                                        &m_surfacePositionReadback,
                                                                        NULL);
    assert(SUCCEEDED(hr));
}

}
