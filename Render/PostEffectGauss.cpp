#include "PostEffectGauss.h"

namespace NSRender
{

void PostEffectGauss::Initialize()
{
    HRESULT hResult = E_FAIL;

        // エフェクト読み込み
        hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                           L"res\\shader\\PostEffectGaussian.fx",
                                           NULL,
                                           NULL,
                                           D3DXSHADER_DEBUG,
                                           NULL,
                                           &m_d3dEffect,
                                           NULL);
        assert(SUCCEEDED(hResult));

        // ブラー用一時テクスチャ
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &m_texWork);
}

LPDIRECT3DTEXTURE9 PostEffectGauss::Draw(LPDIRECT3DTEXTURE9 renderTarget)
{
    m_d3dEffect->SetBool("g_bFilterON", m_bGaussianON);

    DrawFullscreenQuad(renderTarget, m_texWork, "GaussianH");
    DrawFullscreenQuad(m_texWork, renderTarget, "GaussianV");

    return renderTarget;
}

void PostEffectGauss::Finalize()
{

}

void PostEffectGauss::SetEnable(const bool arg)
{
    m_bGaussianON = arg;
}

bool PostEffectGauss::GetEnable() const
{
    return m_bGaussianON;
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

    float texelSize[2] = { 1.0f / 1600, 1.0f / 900};
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] = {
        {                    -0.5f,                     -0.5f, 0, 1, 0, 0 },
        { 1600 - 0.5f,                     -0.5f, 0, 1, 1, 0 },
        {                    -0.5f, 900 - 0.5f, 0, 1, 0, 1 },
        { 1600 - 0.5f, 900 - 0.5f, 0, 1, 1, 1 }
    };

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

}
