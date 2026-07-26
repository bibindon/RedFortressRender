#include "PostEffectDepthOfField.h"

#include <algorithm>
#include <stdexcept>

#include "Camera.h"

#include "Util.h"

namespace NSRender
{

namespace
{
const int kAutoDepthWidth = 80;
const int kAutoDepthHeight = 45;
const int kAutoReductionScale = 4;
}

void PostEffectDepthOfField::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectDepthOfField.cso";
    HRESULT hr = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                          effectPath.c_str(),
                                          NULL,
                                          NULL,
                                          D3DXSHADER_DEBUG,
                                          NULL,
                                          &m_d3dEffect,
                                          NULL);
    assert(SUCCEEDED(hr));

    CreateAutoResources();

    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }

    m_isInitialized = true;
}

void PostEffectDepthOfField::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                  LPDIRECT3DTEXTURE9 texRenderTargetPos,
                                  LPDIRECT3DTEXTURE9 texTarget,
                                  const bool useAutoBlendTexture)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    if (renderTarget == NULL || texRenderTargetPos == NULL || texTarget == NULL)
    {
        return;
    }

    const D3DXVECTOR3 cameraPos = Camera::GetEyePos();
    const D3DXVECTOR4 cameraPos4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);

    m_d3dEffect->SetVector("g_cameraPos", &cameraPos4);
    m_d3dEffect->SetFloat("g_focalDistanceMeters", m_focalDistance);
    m_d3dEffect->SetFloat("g_startNearMeters", m_startNear);
    m_d3dEffect->SetFloat("g_maxBlurDistanceMeters", m_maxBlurDistance);
    m_d3dEffect->SetFloat("g_focusBandHalfWidthMeters", m_focusBandHalfWidth);
    m_d3dEffect->SetFloat("g_blurRadiusPixels", m_blurRadiusPixels);
    m_d3dEffect->SetFloat("g_positionRange", m_positionRange);
    m_d3dEffect->SetFloat("g_dofBlend", m_blend);
    m_d3dEffect->SetBool("g_useAutoBlendTexture", useAutoBlendTexture);
    m_d3dEffect->SetTexture("g_AutoBlendTex", m_autoBlendTextures[m_autoBlendTextureIndex]);

    DrawFullscreenQuad(renderTarget, texRenderTargetPos, texTarget, "Technique1");
}

void PostEffectDepthOfField::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    ReleaseAutoResources();
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectDepthOfField::SetFocalDistance(float focalDistance)
{
    m_focalDistance = focalDistance;
}

void PostEffectDepthOfField::SetStartNear(float startNear)
{
    m_startNear = (std::max)(0.0f, startNear);
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

void PostEffectDepthOfField::SetPositionRange(float positionRange)
{
    m_positionRange = (std::max)(1.0f, positionRange);
}

void PostEffectDepthOfField::UpdateAutoBlend(LPDIRECT3DTEXTURE9 texCameraDepth,
                                             const float nearPlane,
                                             const float farPlane,
                                             const float deltaTime)
{
    if (!m_isInitialized ||
        m_d3dEffect == NULL ||
        texCameraDepth == NULL ||
        m_autoDepthLevels.empty())
    {
        return;
    }

    LPDIRECT3DSURFACE9 previousRenderTarget = NULL;
    HRESULT hr = Common::D3DDevice()->GetRenderTarget(0, &previousRenderTarget);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to get the render target before DOF auto depth reduction.");
    }

    D3DVIEWPORT9 previousViewport { };
    hr = Common::D3DDevice()->GetViewport(&previousViewport);
    if (FAILED(hr))
    {
        SAFE_RELEASE(previousRenderTarget);
        throw std::runtime_error("Failed to get the viewport before DOF auto depth reduction.");
    }

    m_d3dEffect->SetFloat("g_autoCenterRadiusNdc", m_autoCenterRadiusNdc);
    DrawAutoPass(texCameraDepth,
                 m_autoDepthLevels.front().texture,
                 m_autoDepthLevels.front().width,
                 m_autoDepthLevels.front().height,
                 "TechniqueAutoExtract");

    for (std::size_t levelIndex = 1; levelIndex < m_autoDepthLevels.size(); ++levelIndex)
    {
        const AutoDepthLevel& sourceLevel = m_autoDepthLevels.at(levelIndex - 1);
        const AutoDepthLevel& targetLevel = m_autoDepthLevels.at(levelIndex);
        const float sourceTexelSize[2] =
        {
            1.0f / static_cast<float>(sourceLevel.width),
            1.0f / static_cast<float>(sourceLevel.height)
        };
        const float targetSize[2] =
        {
            static_cast<float>(targetLevel.width),
            static_cast<float>(targetLevel.height)
        };
        m_d3dEffect->SetFloatArray("g_AutoTexelSize", sourceTexelSize, 2);
        m_d3dEffect->SetFloatArray("g_AutoTargetSize", targetSize, 2);
        DrawAutoPass(sourceLevel.texture,
                     targetLevel.texture,
                     targetLevel.width,
                     targetLevel.height,
                     "TechniqueAutoReduce");
    }

    int nextBlendTextureIndex = 0;
    if (m_autoBlendTextureIndex == 0)
    {
        nextBlendTextureIndex = 1;
    }
    m_d3dEffect->SetTexture("g_AutoPreviousBlendTex", m_autoBlendTextures[m_autoBlendTextureIndex]);
    m_d3dEffect->SetFloat("g_autoNearPlane", nearPlane);
    m_d3dEffect->SetFloat("g_autoFarPlane", farPlane);
    m_d3dEffect->SetFloat("g_autoActivationDistanceMeters", m_autoActivationDistance);
    m_d3dEffect->SetFloat("g_autoBlendSpeed", m_autoBlendSpeed);
    m_d3dEffect->SetFloat("g_autoDeltaTime", (std::max)(0.0f, deltaTime));
    DrawAutoPass(m_autoDepthLevels.back().texture,
                 m_autoBlendTextures[nextBlendTextureIndex],
                 1,
                 1,
                 "TechniqueAutoBlend");
    m_autoBlendTextureIndex = nextBlendTextureIndex;

    hr = Common::D3DDevice()->SetRenderTarget(0, previousRenderTarget);
    SAFE_RELEASE(previousRenderTarget);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to restore the render target after DOF auto depth reduction.");
    }
    hr = Common::D3DDevice()->SetViewport(&previousViewport);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to restore the viewport after DOF auto depth reduction.");
    }
}

void PostEffectDepthOfField::ResetAutoBlend()
{
    m_autoBlendTextureIndex = 0;
    for (LPDIRECT3DTEXTURE9 texture : m_autoBlendTextures)
    {
        if (texture == NULL)
        {
            continue;
        }

        LPDIRECT3DSURFACE9 surface = NULL;
        const HRESULT surfaceResult = texture->GetSurfaceLevel(0, &surface);
        if (FAILED(surfaceResult))
        {
            throw std::runtime_error("Failed to get a DOF auto blend surface.");
        }
        const HRESULT fillResult = Common::D3DDevice()->ColorFill(surface, NULL, D3DCOLOR_ARGB(0, 0, 0, 0));
        SAFE_RELEASE(surface);
        if (FAILED(fillResult))
        {
            throw std::runtime_error("Failed to clear a DOF auto blend surface.");
        }
    }
}

void PostEffectDepthOfField::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnLostDevice();
    }

    ReleaseAutoResources();
}

void PostEffectDepthOfField::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnResetDevice();
    }

    CreateAutoResources();
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

void PostEffectDepthOfField::DrawAutoPass(LPDIRECT3DTEXTURE9 texSource,
                                          LPDIRECT3DTEXTURE9 texTarget,
                                          const int targetWidth,
                                          const int targetHeight,
                                          const std::string& technique)
{
    if (texSource == NULL || texTarget == NULL || targetWidth <= 0 || targetHeight <= 0)
    {
        throw std::runtime_error("Invalid texture or size for a DOF auto pass.");
    }

    for (DWORD samplerIndex = 0; samplerIndex < 16; ++samplerIndex)
    {
        Common::D3DDevice()->SetTexture(samplerIndex, NULL);
    }

    LPDIRECT3DSURFACE9 targetSurface = NULL;
    HRESULT hr = texTarget->GetSurfaceLevel(0, &targetSurface);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to get a DOF auto render-target surface.");
    }
    hr = Common::D3DDevice()->SetRenderTarget(0, targetSurface);
    SAFE_RELEASE(targetSurface);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to set a DOF auto render target.");
    }

    D3DVIEWPORT9 viewport { };
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = static_cast<DWORD>(targetWidth);
    viewport.Height = static_cast<DWORD>(targetHeight);
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    hr = Common::D3DDevice()->SetViewport(&viewport);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to set a DOF auto viewport.");
    }

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_AutoSourceTex", texSource);

    ScreenVertex quad[4] { };
    quad[0] = { 0.0f,                            0.0f,                             0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { static_cast<float>(targetWidth), 0.0f,                             0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { 0.0f,                            static_cast<float>(targetHeight), 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { static_cast<float>(targetWidth), static_cast<float>(targetHeight), 0.0f, 1.0f, 1.0f, 1.0f };

    Common::D3DDevice()->SetVertexShader(NULL);
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    hr = Common::D3DDevice()->BeginScene();
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to begin a DOF auto scene.");
    }
    m_d3dEffect->Begin(NULL, 0);
    m_d3dEffect->BeginPass(0);
    hr = Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
    m_d3dEffect->EndPass();
    m_d3dEffect->End();
    Common::D3DDevice()->EndScene();
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to draw a DOF auto pass.");
    }
}

void PostEffectDepthOfField::CreateAutoResources()
{
    ReleaseAutoResources();

    int width = kAutoDepthWidth;
    int height = kAutoDepthHeight;
    while (true)
    {
        AutoDepthLevel level;
        level.width = width;
        level.height = height;
        const HRESULT createResult = D3DXCreateTexture(Common::D3DDevice(),
                                                       static_cast<UINT>(width),
                                                       static_cast<UINT>(height),
                                                       1,
                                                       D3DUSAGE_RENDERTARGET,
                                                       D3DFMT_G16R16F,
                                                       D3DPOOL_DEFAULT,
                                                       &level.texture);
        if (FAILED(createResult))
        {
            ReleaseAutoResources();
            throw std::runtime_error("Failed to create a DOF auto depth texture.");
        }
        m_autoDepthLevels.push_back(level);

        if (width == 1 && height == 1)
        {
            break;
        }

        width = (width + kAutoReductionScale - 1) / kAutoReductionScale;
        height = (height + kAutoReductionScale - 1) / kAutoReductionScale;
    }

    for (LPDIRECT3DTEXTURE9& texture : m_autoBlendTextures)
    {
        const HRESULT createResult = D3DXCreateTexture(Common::D3DDevice(),
                                                       1,
                                                       1,
                                                       1,
                                                       D3DUSAGE_RENDERTARGET,
                                                       D3DFMT_G16R16F,
                                                       D3DPOOL_DEFAULT,
                                                       &texture);
        if (FAILED(createResult))
        {
            ReleaseAutoResources();
            throw std::runtime_error("Failed to create a DOF auto blend texture.");
        }
    }
    ResetAutoBlend();
}

void PostEffectDepthOfField::ReleaseAutoResources()
{
    for (AutoDepthLevel& level : m_autoDepthLevels)
    {
        SAFE_RELEASE(level.texture);
    }
    m_autoDepthLevels.clear();
    for (LPDIRECT3DTEXTURE9& texture : m_autoBlendTextures)
    {
        SAFE_RELEASE(texture);
    }
    m_autoBlendTextureIndex = 0;
}

}
