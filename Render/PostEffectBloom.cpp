#include "PostEffectBloom.h"

namespace NSRender
{

void PostEffectBloom::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       L"res\\shader\\PostEffectBloom.fx",
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_d3dEffect,
                                       NULL);
    assert(SUCCEEDED(hResult));

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texWork);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &g_pSceneTex2);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &g_pBrightTex);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &g_pBlurTexH);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &g_pBlurTexV);
}

LPDIRECT3DTEXTURE9 PostEffectBloom::Draw(LPDIRECT3DTEXTURE9 renderTarget)
{
    // ------------------------------------------------------------
    // (1) BrightPass : “ü—Í = g_pSceneTex, o—Í = g_pBrightTex
    // ------------------------------------------------------------
    {
        m_d3dEffect->SetTexture("g_SrcTex", renderTarget);

        float texelSize[2] = { 1.0f / Common::ScreenW(), 1.0f / Common::ScreenH() };
        m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

        DrawFullscreenQuad(m_texWork, "BrightPass");
    }

    // ------------------------------------------------------------
    // (2) Horizontal Blur : “ü—Í = g_pBrightTex, o—Í = g_pBlurTexH
    // ------------------------------------------------------------
    {
        m_d3dEffect->SetTexture("g_SrcTex", m_texWork);

        // ‰¡•ûŒü
        float dir[4] = { 1, 0, 0, 0 };
        m_d3dEffect->SetFloatArray("g_Direction", dir, 4);

        DrawFullscreenQuad(g_pBlurTexH, "Blur");
    }

    // ------------------------------------------------------------
    // (3) Vertical Blur : “ü—Í = g_pBlurTexH, o—Í = g_pBlurTexV
    // ------------------------------------------------------------
    {
        m_d3dEffect->SetTexture("g_SrcTex", g_pBlurTexH);

        // c•ûŒü
        float dir[4] = { 0, 1, 0, 0 };
        m_d3dEffect->SetFloatArray("g_Direction", dir, 4);

        DrawFullscreenQuad(g_pBlurTexV, "Blur");
    }

    // ------------------------------------------------------------
    // (4) Combine : (SceneTex + BlurTexV) ¨ g_pSceneTex2
    // ------------------------------------------------------------
    {
        m_d3dEffect->SetTexture("g_SceneTex", renderTarget);
        m_d3dEffect->SetTexture("g_BlurTex", g_pBlurTexV);

        DrawFullscreenQuad(g_pSceneTex2, "Combine");
    }

    return g_pSceneTex2;
}

void PostEffectBloom::Finalize()
{
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectBloom::SetPostEffectSaturate(const float level)
{
    m_saturateLevel = level;
}

float PostEffectBloom::GetPostEffectSaturate() const
{
    return m_saturateLevel;
}

void PostEffectBloom::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                                         const std::string& technique)
{
    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());

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

void PostEffectBloom::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
}

void PostEffectBloom::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();
}

}
