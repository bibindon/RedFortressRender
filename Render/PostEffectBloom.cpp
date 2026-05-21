#include "PostEffectBloom.h"

#include <algorithm>

#include "Util.h"

namespace NSRender
{

const int PostEffectBloom::BLOOM_LEVEL_DIVISORS[PostEffectBloom::BLOOM_LEVEL_COUNT] = { 2, 4, 8, 16, 32, 64 };

void PostEffectBloom::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectBloom.cso";
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
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

int PostEffectBloom::ComputeBloomTextureWidth(const int divisor) const
{
    return (std::max)(1, Common::ScreenW() / divisor);
}

int PostEffectBloom::ComputeBloomTextureHeight(const int divisor) const
{
    return (std::max)(1, Common::ScreenH() / divisor);
}

void PostEffectBloom::CreateTexture()
{
    ReleaseTextures();

    for (int i = 0; i < BLOOM_LEVEL_COUNT; ++i)
    {
        const int width = ComputeBloomTextureWidth(BLOOM_LEVEL_DIVISORS[i]);
        const int height = ComputeBloomTextureHeight(BLOOM_LEVEL_DIVISORS[i]);

        D3DXCreateTexture(Common::D3DDevice(),
                          width,
                          height,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A16B16G16R16F,
                          D3DPOOL_DEFAULT,
                          &m_texDownsample[i]);

        D3DXCreateTexture(Common::D3DDevice(),
                          width,
                          height,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A16B16G16R16F,
                          D3DPOOL_DEFAULT,
                          &m_texBlur[i]);
    }
}

void PostEffectBloom::Draw(LPDIRECT3DTEXTURE9 renderSource,
                           LPDIRECT3DTEXTURE9 renderTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->SetFloat("g_Threshold", m_threshold);

    const float baseWeights[BLOOM_LEVEL_COUNT] = { 0.28f, 0.24f, 0.18f, 0.14f, 0.10f, 0.06f };
    float bloomWeightsA[4] { };
    float bloomWeightsB[4] { };
    for (int i = 0; i < BLOOM_LEVEL_COUNT; ++i)
    {
        const float weight = baseWeights[i] * m_intensity;
        if (i < 4)
        {
            bloomWeightsA[i] = weight;
        }
        else
        {
            bloomWeightsB[i - 4] = weight;
        }
    }
    m_d3dEffect->SetFloatArray("g_BloomWeightsA", bloomWeightsA, 4);
    m_d3dEffect->SetFloatArray("g_BloomWeightsB", bloomWeightsB, 4);

    DrawFullscreenQuad(renderSource, m_texDownsample[0], "BrightPass");

    for (int i = 1; i < BLOOM_LEVEL_COUNT; ++i)
    {
        DrawFullscreenQuad(m_texDownsample[i - 1], m_texDownsample[i], "Downsample");
    }

    for (int i = 0; i < BLOOM_LEVEL_COUNT; ++i)
    {
        DrawFullscreenQuad(m_texDownsample[i], m_texBlur[i], "Blur3x3");
    }

    DrawCombineQuad(renderSource, renderTarget);
}

void PostEffectBloom::Finalize()
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

void PostEffectBloom::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
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

    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);

    const float texelSize[2] = { 1.0f / static_cast<float>(sourceDesc.Width), 1.0f / static_cast<float>(sourceDesc.Height) };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

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

void PostEffectBloom::DrawCombineQuad(LPDIRECT3DTEXTURE9 texScene,
                                      LPDIRECT3DTEXTURE9 texTarget)
{
    if (texScene == NULL || texTarget == NULL)
    {
        return;
    }

    D3DSURFACE_DESC targetDesc { };
    texTarget->GetLevelDesc(0, &targetDesc);

    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique("Combine");
    m_d3dEffect->SetTexture("g_SceneTex", texScene);
    m_d3dEffect->SetTexture("g_BlurTex0", m_texBlur[0]);
    m_d3dEffect->SetTexture("g_BlurTex1", m_texBlur[1]);
    m_d3dEffect->SetTexture("g_BlurTex2", m_texBlur[2]);
    m_d3dEffect->SetTexture("g_BlurTex3", m_texBlur[3]);
    m_d3dEffect->SetTexture("g_BlurTex4", m_texBlur[4]);
    m_d3dEffect->SetTexture("g_BlurTex5", m_texBlur[5]);

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

void PostEffectBloom::SetThreshold(const float arg)
{
    m_threshold = arg;
}

void PostEffectBloom::SetIntensity(const float arg)
{
    m_intensity = arg;
}

void PostEffectBloom::SetSize(const float arg)
{
    m_size = arg;
}

void PostEffectBloom::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnLostDevice();
    ReleaseTextures();
}

void PostEffectBloom::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnResetDevice();
    CreateTexture();
}

void PostEffectBloom::ReleaseTextures()
{
    for (int i = 0; i < BLOOM_LEVEL_COUNT; ++i)
    {
        SAFE_RELEASE(m_texDownsample[i]);
        SAFE_RELEASE(m_texBlur[i]);
    }
}

}
