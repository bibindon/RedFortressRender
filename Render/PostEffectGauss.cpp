#include "PostEffectGauss.h"
#include "PostEffectBloom.h"

#include <algorithm>
#include <cmath>

#include "Util.h"

namespace NSRender
{

void PostEffectGauss::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectGaussian.cso";
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       effectPath.c_str(),
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &m_d3dEffect,
                                       NULL);
    assert(SUCCEEDED(hResult));
    CreateWorkTextures();
    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }
    m_isInitialized = true;
}

void PostEffectGauss::DrawHorizontal(LPDIRECT3DTEXTURE9 texSource,
                                     LPDIRECT3DTEXTURE9 texTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->SetBool("g_bFilterON", TRUE);
    m_d3dEffect->SetInt("g_sampleSize", m_sampleSize);
    DrawFullscreenQuad(texSource, texTarget, "GaussianH");
}

void PostEffectGauss::DrawVertical(LPDIRECT3DTEXTURE9 texSource,
                                   LPDIRECT3DTEXTURE9 texTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->SetBool("g_bFilterON", TRUE);
    m_d3dEffect->SetInt("g_sampleSize", m_sampleSize);
    DrawFullscreenQuad(texSource, texTarget, "GaussianV");
}

void PostEffectGauss::Draw(LPDIRECT3DTEXTURE9 texSource,
                           LPDIRECT3DTEXTURE9 texTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    if (texSource == NULL || texTarget == NULL)
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
    DrawStageToTexture(texSource, weakStage, m_texWeak);
    DrawStageToTexture(texSource, strongStage, m_texTemp);
    DrawBlendQuad(m_texWeak, m_texTemp, texTarget, blend);
}

void PostEffectGauss::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    ReleaseWorkTextures();
    SAFE_RELEASE(m_d3dEffect);
    m_isInitialized = false;
}

void PostEffectGauss::SetSampleSize(const int sampleSize)
{
    m_sampleSize = sampleSize;
}

void PostEffectGauss::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                         LPDIRECT3DTEXTURE9 texTarget,
                                         const std::string& technique,
                                         float filterSpacing)
{
    if (texSource == NULL || texTarget == NULL)
    {
        return;
    }

    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    D3DSURFACE_DESC targetDesc { };
    pSceneRT->GetDesc(&targetDesc);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);

    D3DSURFACE_DESC sourceDesc { };
    texSource->GetLevelDesc(0, &sourceDesc);
    float texelSize[2] = { 1.0f / static_cast<float>(sourceDesc.Width),
                           1.0f / static_cast<float>(sourceDesc.Height) };

    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);
    m_d3dEffect->SetFloat("g_FilterSpacing", filterSpacing);

    ScreenVertex quad[4] { };

    quad[0].x   = -0.5f;
    quad[0].y   = -0.5f;
    quad[0].z   = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u   = 0.0f;
    quad[0].v   = 0.0f;

    quad[1].x   = -0.5f + static_cast<float>(targetDesc.Width);
    quad[1].y   = -0.5f;
    quad[1].z   = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u   = 1.0f;
    quad[1].v   = 0.0f;

    quad[2].x   = -0.5f;
    quad[2].y   = -0.5f + static_cast<float>(targetDesc.Height);
    quad[2].z   = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u   = 0.0f;
    quad[2].v   = 1.0f;

    quad[3].x   = -0.5f + static_cast<float>(targetDesc.Width);
    quad[3].y   = -0.5f + static_cast<float>(targetDesc.Height);
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

void PostEffectGauss::SetIntensity(const float arg)
{
    m_intensity = (std::max)(0.0f, (std::min)(arg, 1.0f));
}

void PostEffectGauss::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnLostDevice();
    ReleaseWorkTextures();
}

void PostEffectGauss::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnResetDevice();
    CreateWorkTextures();
}

void PostEffectGauss::DrawBlendQuad(LPDIRECT3DTEXTURE9 texBase,
                                    LPDIRECT3DTEXTURE9 texBlur,
                                    LPDIRECT3DTEXTURE9 texTarget,
                                    float blend)
{
    blend = (std::max)(0.0f, (std::min)(blend, 1.0f));
    m_d3dEffect->SetTexture("g_BlendTex", texBlur);
    m_d3dEffect->SetFloat("g_BlendAmount", blend);
    DrawFullscreenQuad(texBase, texTarget, "BlendTwo");
}

void PostEffectGauss::DrawFullResolutionBlurTo(LPDIRECT3DTEXTURE9 texSource,
                                               LPDIRECT3DTEXTURE9 texTarget)
{
    DrawFullscreenQuad(texSource, texTarget, "UpsampleOnly3x3");
}

void PostEffectGauss::BuildDownChain(LPDIRECT3DTEXTURE9 texSource,
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

void PostEffectGauss::BuildUpChain(int firstLevel,
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

void PostEffectGauss::DrawStageToTexture(LPDIRECT3DTEXTURE9 texSource,
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

void PostEffectGauss::CreateWorkTextures()
{
    ReleaseWorkTextures();

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texTemp);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texWeak);

    for (int i = 0; i < GAUSSIAN_LEVEL_COUNT; ++i)
    {
        const int width = ComputeLevelWidth(i);
        const int height = ComputeLevelHeight(i);
        D3DXCreateTexture(Common::D3DDevice(),
                          width,
                          height,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A16B16G16R16F,
                          D3DPOOL_DEFAULT,
                          &m_texDown[i]);
        D3DXCreateTexture(Common::D3DDevice(),
                          width,
                          height,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A16B16G16R16F,
                          D3DPOOL_DEFAULT,
                          &m_texUp[i]);
    }
}

void PostEffectGauss::ReleaseWorkTextures()
{
    SAFE_RELEASE(m_texTemp);
    SAFE_RELEASE(m_texWeak);
    for (int i = 0; i < GAUSSIAN_LEVEL_COUNT; ++i)
    {
        SAFE_RELEASE(m_texDown[i]);
        SAFE_RELEASE(m_texUp[i]);
    }
}

int PostEffectGauss::ComputeBlurStrength() const
{
    const float normalized = (std::max)(0.0f, (std::min)(m_intensity, 1.0f));
    return static_cast<int>(std::lround(normalized * static_cast<float>(GAUSSIAN_BLUR_STRENGTH_MAX)));
}

int PostEffectGauss::ComputeLevelWidth(int level) const
{
    const int divisor = 1 << (GAUSSIAN_START_EXP + level * GAUSSIAN_LEVEL_EXP_STEP);
    return (std::max)(1, Common::ScreenW() / divisor);
}

int PostEffectGauss::ComputeLevelHeight(int level) const
{
    const int divisor = 1 << (GAUSSIAN_START_EXP + level * GAUSSIAN_LEVEL_EXP_STEP);
    return (std::max)(1, Common::ScreenH() / divisor);
}

}
