#include "PostEffectHeightFog.h"

namespace NSRender
{

void PostEffectHeightFog::Initialize()
{
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               L"../x64/Debug/PostEffectHeightFog.cso",
                                               nullptr,
                                               nullptr,
                                               D3DXSHADER_DEBUG,
                                               nullptr,
                                               &m_d3dEffect,
                                               nullptr);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_texWork);
    assert(SUCCEEDED(hResult));

    Common::AddDeviceLostResource(this);
}

LPDIRECT3DTEXTURE9 PostEffectHeightFog::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                             LPDIRECT3DTEXTURE9 texRenderTargetPos)
{
    if (renderTarget == nullptr || texRenderTargetPos == nullptr)
    {
        return renderTarget;
    }

    m_d3dEffect->SetFloat("g_IntensityHeight", m_intensity);
    m_d3dEffect->SetFloat("g_HeightStart", m_startHeight);
    m_d3dEffect->SetFloat("g_HeightMax", m_maxHeight);
    m_d3dEffect->SetFloat("g_PosRange", 50.0f);
    m_d3dEffect->SetVector("g_FogColor", &m_fogColor);
    m_d3dEffect->SetTexture("g_PosTex", texRenderTargetPos);

    DrawFullscreenQuad(renderTarget, m_texWork, "TechHeightFog");
    return m_texWork;
}

void PostEffectHeightFog::Finalize()
{
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
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

void PostEffectHeightFog::SetFogColor(const D3DXCOLOR& color)
{
    m_fogColor = D3DXVECTOR4(color.r, color.g, color.b, color.a);
}

void PostEffectHeightFog::OnDeviceLost()
{
    if (m_d3dEffect != nullptr)
    {
        m_d3dEffect->OnLostDevice();
    }
    SAFE_RELEASE(m_texWork);
}

void PostEffectHeightFog::OnDeviceReset()
{
    if (m_d3dEffect != nullptr)
    {
        m_d3dEffect->OnResetDevice();
    }

    const HRESULT hResult = D3DXCreateTexture(Common::D3DDevice(),
                                              Common::ScreenW(),
                                              Common::ScreenH(),
                                              1,
                                              D3DUSAGE_RENDERTARGET,
                                              D3DFMT_A16B16G16R16F,
                                              D3DPOOL_DEFAULT,
                                              &m_texWork);
    assert(SUCCEEDED(hResult));
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
