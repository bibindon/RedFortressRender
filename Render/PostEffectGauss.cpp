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
                                           &g_pEffect3,
                                           NULL);
        assert(SUCCEEDED(hResult));

        // オフスクリーン用テクスチャ
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pSceneTex);

        // ブラー用一時テクスチャ
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pTempTex);
}

void PostEffectGauss::Draw()
{
    g_pEffect3->SetBool("g_bFilterON", m_bGaussianON);

    // 2) 横ブラー: 入力=m_texPostEffectBack3, 出力=g_pTempTex
    {
        LPDIRECT3DSURFACE9 pTempRT = NULL;
        g_pTempTex->GetSurfaceLevel(0, &pTempRT);
        Common::D3DDevice()->SetRenderTarget(0, pTempRT);
        SAFE_RELEASE(pTempRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();
        DrawFullscreenQuad(g_pSceneTex3, "GaussianH");
        Common::D3DDevice()->EndScene();
    }

    // 3) 縦ブラー: 入力=m_texPostEffectBack1, 出力=g_pSceneTex3（最終テクスチャを更新）
    {
        LPDIRECT3DSURFACE9 pSceneRT = NULL;
        g_pSceneTex3->GetSurfaceLevel(0, &pSceneRT);
        Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
        SAFE_RELEASE(pSceneRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();
        DrawFullscreenQuad(g_pTempTex, "GaussianV");
        Common::D3DDevice()->EndScene();
    }
}

void PostEffectGauss::Finalize()
{

}

void PostEffectGauss::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 tex, const char* tech)
{
    Common::D3DDevice()->SetVertexShader(NULL);

    g_pEffect3->SetTechnique(tech);
    g_pEffect3->SetTexture("g_SrcTex", tex);

    float texelSize[2] = { 1.0f / 1600, 1.0f / 900};
    g_pEffect3->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] = {
        {                    -0.5f,                     -0.5f, 0, 1, 0, 0 },
        { 1600 - 0.5f,                     -0.5f, 0, 1, 1, 0 },
        {                    -0.5f, 900 - 0.5f, 0, 1, 0, 1 },
        { 1600 - 0.5f, 900 - 0.5f, 0, 1, 1, 1 }
    };

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    g_pEffect3->Begin(NULL, 0);
    g_pEffect3->BeginPass(0);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
    g_pEffect3->EndPass();
    g_pEffect3->End();
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}

}
