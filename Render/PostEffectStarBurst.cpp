#include "PostEffectStarBurst.h"

namespace NSRender
{

void PostEffectStarBurst::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       //L"res\\shader\\PostEffectStarBurst.fx",
                                       L"../x64/Debug/PostEffectStarBurst.cso",
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
                      &m_texPostEffectBack1);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBright);

    // 0°（水平）
    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBlurH);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBlurH2);

    // 60°
    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBlurV);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBlurV2);

    // 120°（★追加）
    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBlurD);

    D3DXCreateTexture(Common::D3DDevice(),
                      1600,
                      900,
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBlurD2);
}

LPDIRECT3DTEXTURE9 PostEffectStarBurst::Draw(LPDIRECT3DTEXTURE9 renderSource)
{
    if (m_d3dEffect == NULL)
    {
        return renderSource;
    }

    if (!m_bEnable)
    {
        return renderSource;
    }

    // テクセルサイズ（ブラーで使用）
    float texelSize[2] = { 1.0f / Common::ScreenW(), 1.0f / Common::ScreenH() };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    // (2) BrightPass : 入力=m_renderTarget, 出力=g_pBrightTex2
    SetRTFromTex(m_texBright);
    DrawFullscreenQuad(renderSource, "BrightPass");

    // (3a) 0° ブラー : 入力=m_texBright, 出力=g_pBlurTexH2
    SetRTFromTex(m_texBlurH);
    {
        float dir[4] = { 1.0f, 0.0f, 0, 0 };
        m_d3dEffect->SetFloatArray("g_Direction", dir, 4);
    }

    DrawFullscreenQuad(m_texBright, "Blur");

    SetRTFromTex(m_texBlurH2);
    DrawFullscreenQuad(m_texBlurH, "Blur");

    // (3b) 60° ブラー : 入力=m_texBright, 出力=g_pBlurTexV2
    SetRTFromTex(m_texBlurV);
    {
        float dir[4] = { 0.5f, 0.8660254f, 0, 0 };
        m_d3dEffect->SetFloatArray("g_Direction", dir, 4);
    }

    DrawFullscreenQuad(m_texBright, "Blur");

    SetRTFromTex(m_texBlurV2);
    DrawFullscreenQuad(m_texBlurV, "Blur");

    // (3c) 120° ブラー : 入力=m_texBright, 出力=g_pBlurTexD
    SetRTFromTex(m_texBlurD);
    {
        float dir[4] = { -0.5f, 0.8660254f, 0, 0 };
        m_d3dEffect->SetFloatArray("g_Direction", dir, 4);
    }

    DrawFullscreenQuad(m_texBright, "Blur");

    SetRTFromTex(m_texBlurD2);
    DrawFullscreenQuad(m_texBlurD, "Blur");

    // (4) 合成 : (SceneTex2 + 0° + 60° + 120°) → g_pSceneTex3
    SetRTFromTex(m_texPostEffectBack1);
    m_d3dEffect->SetTexture("g_SceneTex", renderSource);
    m_d3dEffect->SetTexture("g_BlurTexH", m_texBlurH2);
    m_d3dEffect->SetTexture("g_BlurTexV", m_texBlurV2);
    m_d3dEffect->SetTexture("g_BlurTex60", m_texBlurD2); // Combine は3軸を加算
    DrawFullscreenQuad(NULL, "Combine");

    return m_texPostEffectBack1;
}

void PostEffectStarBurst::SetRTFromTex(LPDIRECT3DTEXTURE9 tex)
{
    LPDIRECT3DSURFACE9 rt = NULL;
    tex->GetSurfaceLevel(0, &rt);                 // AddRef 済みで返る
    Common::D3DDevice()->SetRenderTarget(0, rt);         // Device 側が参照を保持
    SAFE_RELEASE(rt);                             // 即ReleaseでOK
}

void PostEffectStarBurst::Finalize()
{
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectStarBurst::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                             const std::string& technique)
{
    Common::D3DDevice()->SetVertexShader(NULL);
    Common::D3DDevice()->SetVertexDeclaration(NULL);

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

    if (texSource)
    {
        m_d3dEffect->SetTexture("g_SrcTex", texSource);
    }
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

void PostEffectStarBurst::SetEnable(const bool arg)
{
    m_bEnable = arg;
}

void PostEffectStarBurst::SetThreshold(const float arg)
{
    m_threshold = arg;
}

void PostEffectStarBurst::SetIntensity(const float arg)
{
    m_intensity = arg;
}

void PostEffectStarBurst::SetSize(const float arg)
{
    m_size = arg;
}

void PostEffectStarBurst::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
}

void PostEffectStarBurst::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();
}

}
