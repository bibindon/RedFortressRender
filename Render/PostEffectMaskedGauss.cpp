#include "PostEffectMaskedGauss.h"

#include <algorithm>
#include <cmath>
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

    const int availableStageCount = GAUSSIAN_LEVEL_COUNT + 2;
    const float stagePosition = static_cast<float>(ComputeBlurStrength())
                              / static_cast<float>(GAUSSIAN_BLUR_STRENGTH_MAX)
                              * static_cast<float>(availableStageCount - 1);
    int weakStage = static_cast<int>(std::floor(stagePosition));
    weakStage = (std::max)(0, (std::min)(weakStage, availableStageCount - 1));

    int strongStage = weakStage + 1;
    strongStage = (std::min)(strongStage, availableStageCount - 1);

    const float blend = (strongStage == weakStage) ? 1.0f
                                                  : stagePosition - static_cast<float>(weakStage);
    DrawStageToTexture(renderTarget, weakStage, m_texWeak);
    DrawStageToTexture(renderTarget, strongStage, m_texStrong);
    DrawBlendQuad(m_texWeak, m_texStrong, m_texBlurResult, blend);
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

void PostEffectMaskedGauss::SetIntensity(const float intensity)
{
    m_intensity = (std::max)(0.0f, (std::min)(intensity, 1.0f));
}

void PostEffectMaskedGauss::SetMaskPath(const std::wstring& maskPath)
{
    m_maskPath = maskPath;
    if (m_isInitialized)
    {
        LoadMaskTexture();
    }
}

void PostEffectMaskedGauss::SetMaskScaleToBaseResolution(const bool enabled)
{
    m_maskScaleToBaseResolution = enabled;
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
                                        &m_texWeak);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_texStrong);
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

    for (int i = 0; i < GAUSSIAN_LEVEL_COUNT; ++i)
    {
        const int width = ComputeLevelWidth(i);
        const int height = ComputeLevelHeight(i);
        hResult = D3DXCreateTexture(Common::D3DDevice(),
                                    width,
                                    height,
                                    1,
                                    D3DUSAGE_RENDERTARGET,
                                    D3DFMT_A16B16G16R16F,
                                    D3DPOOL_DEFAULT,
                                    &m_texDown[i]);
        assert(SUCCEEDED(hResult));

        hResult = D3DXCreateTexture(Common::D3DDevice(),
                                    width,
                                    height,
                                    1,
                                    D3DUSAGE_RENDERTARGET,
                                    D3DFMT_A16B16G16R16F,
                                    D3DPOOL_DEFAULT,
                                    &m_texUp[i]);
        assert(SUCCEEDED(hResult));
    }
}

void PostEffectMaskedGauss::ReleaseWorkTextures()
{
    SAFE_RELEASE(m_texWeak);
    SAFE_RELEASE(m_texStrong);
    SAFE_RELEASE(m_texBlurResult);
    for (int i = 0; i < GAUSSIAN_LEVEL_COUNT; ++i)
    {
        SAFE_RELEASE(m_texDown[i]);
        SAFE_RELEASE(m_texUp[i]);
    }
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
                                               const std::string& technique,
                                               float filterSpacing)
{
    if (texSource == nullptr || texTarget == nullptr)
    {
        return;
    }

    LPDIRECT3DSURFACE9 renderTargetSurface = nullptr;
    texTarget->GetSurfaceLevel(0, &renderTargetSurface);
    D3DSURFACE_DESC targetDesc { };
    renderTargetSurface->GetDesc(&targetDesc);
    Common::D3DDevice()->SetRenderTarget(0, renderTargetSurface);
    SAFE_RELEASE(renderTargetSurface);

    Common::D3DDevice()->SetVertexShader(nullptr);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);

    D3DSURFACE_DESC sourceDesc { };
    texSource->GetLevelDesc(0, &sourceDesc);
    const float texelSize[2] = { 1.0f / static_cast<float>(sourceDesc.Width),
                                 1.0f / static_cast<float>(sourceDesc.Height) };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);
    m_d3dEffect->SetFloat("g_FilterSpacing", filterSpacing);

    ScreenVertex quad[4] { };

    quad[0].x = -0.5f;
    quad[0].y = -0.5f;
    quad[0].z = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u = 0.0f;
    quad[0].v = 0.0f;

    quad[1].x = -0.5f + static_cast<float>(targetDesc.Width);
    quad[1].y = -0.5f;
    quad[1].z = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u = 1.0f;
    quad[1].v = 0.0f;

    quad[2].x = -0.5f;
    quad[2].y = -0.5f + static_cast<float>(targetDesc.Height);
    quad[2].z = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u = 0.0f;
    quad[2].v = 1.0f;

    quad[3].x = -0.5f + static_cast<float>(targetDesc.Width);
    quad[3].y = -0.5f + static_cast<float>(targetDesc.Height);
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

void PostEffectMaskedGauss::DrawBlendQuad(LPDIRECT3DTEXTURE9 texBase,
                                          LPDIRECT3DTEXTURE9 texBlur,
                                          LPDIRECT3DTEXTURE9 texTarget,
                                          float blend)
{
    blend = (std::max)(0.0f, (std::min)(blend, 1.0f));
    m_d3dEffect->SetTexture("g_BlendTex", texBlur);
    m_d3dEffect->SetFloat("g_BlendAmount", blend);
    DrawFullscreenQuad(texBase, texTarget, "BlendTwo");
}

void PostEffectMaskedGauss::DrawStageToTexture(LPDIRECT3DTEXTURE9 texSource,
                                               int actualStage,
                                               LPDIRECT3DTEXTURE9 texTarget)
{
    if (actualStage <= 0)
    {
        DrawFullscreenQuad(texSource, texTarget, "Copy");
        return;
    }

    if (actualStage == 1)
    {
        DrawFullResolutionBlurTo(texSource, texTarget);
        return;
    }

    const int firstLevel = 0;
    const int lastLevel = actualStage - 2;
    BuildDownChain(texSource, firstLevel, lastLevel);
    BuildUpChain(firstLevel, lastLevel);
    DrawFullscreenQuad(m_texUp[firstLevel], texTarget, "UpsampleOnly3x3");
}

void PostEffectMaskedGauss::DrawFullResolutionBlurTo(LPDIRECT3DTEXTURE9 texSource,
                                                     LPDIRECT3DTEXTURE9 texTarget)
{
    DrawFullscreenQuad(texSource, texTarget, "UpsampleOnly3x3");
}

void PostEffectMaskedGauss::BuildDownChain(LPDIRECT3DTEXTURE9 texSource,
                                           int firstLevel,
                                           int lastLevel)
{
    firstLevel = (std::max)(0, firstLevel);
    lastLevel = (std::min)(lastLevel, GAUSSIAN_LEVEL_COUNT - 1);
    if (lastLevel < firstLevel)
    {
        return;
    }

    LPDIRECT3DTEXTURE9 source = texSource;
    for (int i = firstLevel; i <= lastLevel; ++i)
    {
        DrawFullscreenQuad(source, m_texDown[i], "Down3x3");
        source = m_texDown[i];
    }
}

void PostEffectMaskedGauss::BuildUpChain(int firstLevel,
                                         int lastLevel)
{
    firstLevel = (std::max)(0, firstLevel);
    lastLevel = (std::min)(lastLevel, GAUSSIAN_LEVEL_COUNT - 1);
    if (lastLevel < firstLevel)
    {
        return;
    }

    DrawFullscreenQuad(m_texDown[lastLevel], m_texUp[lastLevel], "Copy");
    for (int i = lastLevel - 1; i >= firstLevel; --i)
    {
        DrawFullscreenQuad(m_texUp[i + 1], m_texUp[i], "UpsampleOnly3x3");
    }
}

void PostEffectMaskedGauss::DrawCompositeQuad(LPDIRECT3DTEXTURE9 texBlurred,
                                              LPDIRECT3DTEXTURE9 texOriginal,
                                              LPDIRECT3DTEXTURE9 texTarget)
{
    LPDIRECT3DSURFACE9 renderTargetSurface = nullptr;
    texTarget->GetSurfaceLevel(0, &renderTargetSurface);
    D3DSURFACE_DESC targetDesc { };
    renderTargetSurface->GetDesc(&targetDesc);
    Common::D3DDevice()->SetRenderTarget(0, renderTargetSurface);
    SAFE_RELEASE(renderTargetSurface);

    Common::D3DDevice()->SetVertexShader(nullptr);

    m_d3dEffect->SetTechnique("CompositeMasked");
    m_d3dEffect->SetTexture("g_SrcTex", texBlurred);
    m_d3dEffect->SetTexture("g_SrcTex2", texOriginal);
    m_d3dEffect->SetTexture("g_MaskTex", m_texMask);

    const float texelSize[2] = { 1.0f / static_cast<float>(targetDesc.Width),
                                 1.0f / static_cast<float>(targetDesc.Height) };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    const float maskBaseSize[2] = { static_cast<float>(Common::BASE_W),
                                    static_cast<float>(Common::BASE_H) };
    const float maskScreenSize[2] = { static_cast<float>(targetDesc.Width),
                                      static_cast<float>(targetDesc.Height) };
    float maskOffset[2] = { 0.0f, 0.0f };
    if (m_maskScaleToBaseResolution)
    {
        maskOffset[0] = (maskScreenSize[0] - maskBaseSize[0]) * 0.5f;
        maskOffset[1] = (maskScreenSize[1] - maskBaseSize[1]) * 0.5f;
    }
    m_d3dEffect->SetFloatArray("g_MaskBaseSize", maskBaseSize, 2);
    m_d3dEffect->SetFloatArray("g_MaskScreenSize", maskScreenSize, 2);
    m_d3dEffect->SetFloatArray("g_MaskOffset", maskOffset, 2);

    ScreenVertex quad[4] { };

    quad[0].x = -0.5f;
    quad[0].y = -0.5f;
    quad[0].z = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u = 0.0f;
    quad[0].v = 0.0f;

    quad[1].x = -0.5f + static_cast<float>(targetDesc.Width);
    quad[1].y = -0.5f;
    quad[1].z = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u = 1.0f;
    quad[1].v = 0.0f;

    quad[2].x = -0.5f;
    quad[2].y = -0.5f + static_cast<float>(targetDesc.Height);
    quad[2].z = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u = 0.0f;
    quad[2].v = 1.0f;

    quad[3].x = -0.5f + static_cast<float>(targetDesc.Width);
    quad[3].y = -0.5f + static_cast<float>(targetDesc.Height);
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

int PostEffectMaskedGauss::ComputeBlurStrength() const
{
    const float normalized = (std::max)(0.0f, (std::min)(m_intensity, 1.0f));
    return static_cast<int>(std::lround(normalized * static_cast<float>(GAUSSIAN_BLUR_STRENGTH_MAX)));
}

int PostEffectMaskedGauss::ComputeLevelWidth(int level) const
{
    const int divisor = 1 << (GAUSSIAN_START_EXP + level * GAUSSIAN_LEVEL_EXP_STEP);
    return (std::max)(1, Common::ScreenW() / divisor);
}

int PostEffectMaskedGauss::ComputeLevelHeight(int level) const
{
    const int divisor = 1 << (GAUSSIAN_START_EXP + level * GAUSSIAN_LEVEL_EXP_STEP);
    return (std::max)(1, Common::ScreenH() / divisor);
}

}
