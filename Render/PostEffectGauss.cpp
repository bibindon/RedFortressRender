#include "PostEffectGauss.h"
#include "PostEffectBloom.h"

namespace NSRender
{

void PostEffectGauss::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       //L"res\\shader\\PostEffectGaussian.fx",
                                       L"../x64/Debug/PostEffectGaussian.cso",
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_d3dEffect,
                                       NULL);
    assert(SUCCEEDED(hResult));

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texWork);
    Common::AddDeviceLostResource(this);
}

LPDIRECT3DTEXTURE9 PostEffectGauss::Draw(LPDIRECT3DTEXTURE9 renderTarget)
{
    if (!m_bEnable)
    {
        return renderTarget;
    }

    m_d3dEffect->SetBool("g_bFilterON", m_bEnable);
    m_d3dEffect->SetInt("g_sampleSize", m_sampleSize);

    DrawFullscreenQuad(renderTarget,    m_texWork,      "GaussianH");
    DrawFullscreenQuad(m_texWork,       renderTarget,   "GaussianH");
    DrawFullscreenQuad(renderTarget,    m_texWork,      "GaussianV");
    DrawFullscreenQuad(m_texWork,       renderTarget,   "GaussianV");

    return renderTarget;
}

void PostEffectGauss::Finalize()
{
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectGauss::SetEnable(const bool arg)
{
    m_bEnable = arg;
}

bool PostEffectGauss::GetEnable() const
{
    return m_bEnable;
}

void PostEffectGauss::SetSampleSize(const int sampleSize)
{
    m_sampleSize = sampleSize;
}

void PostEffectGauss::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                         LPDIRECT3DTEXTURE9 texTarget,
                                         const std::string& technique)
{
    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);

    float texelSize[2] = { 1.0f / Common::ScreenW(), 1.0f / Common::ScreenH() };

    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

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

void PostEffectGauss::SetIntensity(const float arg)
{
    m_intensity = arg;
}

void PostEffectGauss::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
    SAFE_RELEASE(m_texWork);
}

void PostEffectGauss::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A16B16G16R16F,
                      D3DPOOL_DEFAULT,
                      &m_texWork);
}

}
