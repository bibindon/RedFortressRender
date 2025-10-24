#include "PostEffectSSAO.h"

#include "Camera.h"

namespace NSRender
{
void PostEffectSSAO::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       //L"res\\shader\\PostEffectSSAO.fx",
                                       L"../x64/Debug/PostEffectSSAO.cso",
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_fxSSAO,
                                       NULL);

    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_rtAoTex);

    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_rtAoTempTex);

    assert(SUCCEEDED(hResult));
    Common::AddDeviceLostResource(this);
}

void PostEffectSSAO::Finalize()
{

}

LPDIRECT3DTEXTURE9 PostEffectSSAO::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                        LPDIRECT3DTEXTURE9 m_texRenderTargetZ,
                                        LPDIRECT3DTEXTURE9 m_texRenderTargetPos)
{
    HRESULT hr = E_FAIL;

    // 画面サイズから invSize を計算
    D3DSURFACE_DESC descZ = {};
    m_texRenderTargetZ->GetLevelDesc(0, &descZ);
    D3DXVECTOR2 invSize(1.0f / descZ.Width, 1.0f / descZ.Height);

    // ビュー・プロジェクション行列
    D3DXMATRIX matrixView = Camera::GetViewMatrix();
    D3DXMATRIX matrixProj = Camera::GetProjMatrix();

    // 旧RT退避
    LPDIRECT3DSURFACE9 oldRt0 = NULL;
    hr = Common::D3DDevice()->GetRenderTarget(0, &oldRt0);

    // サーフェス取得
    LPDIRECT3DSURFACE9 surfAO = NULL;
    LPDIRECT3DSURFACE9 surfAOTemp = NULL;
    LPDIRECT3DSURFACE9 surfRT1 = NULL;
    LPDIRECT3DSURFACE9 surfRT2 = NULL;

    m_rtAoTex->GetSurfaceLevel(0, &surfAO);
    m_rtAoTempTex->GetSurfaceLevel(0, &surfAOTemp);
    renderTarget->GetSurfaceLevel(0, &surfRT1);

    // ========= AO 生成 =========
    hr = Common::D3DDevice()->SetRenderTarget(0, surfAO);
    Common::D3DDevice()->SetRenderTarget(1, NULL);

    Common::D3DDevice()->Clear(0, NULL,
                               D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                               D3DCOLOR_RGBA(255, 255, 255, 255),
                               1.0f, 0);

    m_fxSSAO->SetMatrix("g_matView",  &matrixView);
    m_fxSSAO->SetMatrix("g_matProj",  &matrixProj);
    m_fxSSAO->SetFloat("g_fNear", Camera::GetNear());
    m_fxSSAO->SetFloat("g_fFar",  Camera::GetFar());
    m_fxSSAO->SetFloat("g_posRange", 50.0f);
    m_fxSSAO->SetFloatArray("g_invSize", (FLOAT*)&invSize, 2);

    m_fxSSAO->SetTexture("texZ",   m_texRenderTargetZ);
    m_fxSSAO->SetTexture("texPos", m_texRenderTargetPos);

    m_fxSSAO->SetFloat("g_aoStepWorld", 4.0f);   // 5 → 4（半径を少し縮める）
    m_fxSSAO->SetFloat("g_originPush", 0.05f);  // 0.15 → 0.05（押し出し弱め）
//    m_fxSSAO->SetFloat("g_planeThickness", 0.006f); // 0.02 → 0.006（同一面厚みを薄く）
    m_fxSSAO->SetFloat("g_edgeZ", 0.006f); // 0.01 → 0.006（縁の深度許容を広げる）
    m_fxSSAO->SetFloat("g_aoStrength", 1.2f);   // 1.5 → 1.2（強すぎ抑制）
    m_fxSSAO->SetFloat("g_aoBias", 0.0002f);// 0.0001 → 0.0002（微バイアス）

    m_fxSSAO->SetTechnique("TechniqueAO_Create"); // PS_AO を実行
    m_fxSSAO->Begin(NULL, 0);
    m_fxSSAO->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO->EndPass();
    m_fxSSAO->End();

    // ========= Blur H =========
    hr = Common::D3DDevice()->SetRenderTarget(0, surfAOTemp);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(255,255,255,255), 1.0f, 0);

    m_fxSSAO->SetTexture("texAO",  m_rtAoTex);
    m_fxSSAO->SetTexture("texZ",   m_texRenderTargetZ);
    m_fxSSAO->SetFloatArray("g_invSize", (FLOAT*)&invSize, 2);
    m_fxSSAO->SetFloat("g_sigmaPx", 8.0f);
    m_fxSSAO->SetFloat("g_depthReject", 0.0001f);

    m_fxSSAO->SetTechnique("TechniqueAO_BlurH");
    m_fxSSAO->Begin(NULL, 0);
    m_fxSSAO->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO->EndPass();
    m_fxSSAO->End();

    // ========= Blur V =========
    hr = Common::D3DDevice()->SetRenderTarget(0, surfAO);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(255,255,255,255), 1.0f, 0);

    m_fxSSAO->SetTexture("texAO",  m_rtAoTempTex);
    m_fxSSAO->SetTexture("texZ",   m_texRenderTargetZ);
    m_fxSSAO->SetFloatArray("g_invSize", (FLOAT*)&invSize, 2);

    m_fxSSAO->SetTechnique("TechniqueAO_BlurV");
    m_fxSSAO->Begin(NULL, 0);
    m_fxSSAO->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO->EndPass();
    m_fxSSAO->End();

    // ========= Composite: Color × AO =========
    // 入力: m_pRenderTarget1（カラー）, m_rtAoTex（AO）
    // 出力: m_pRenderTarget2（一旦ここに書き、最後に RT2→RT1 へコピー）
    hr = Common::D3DDevice()->SetRenderTarget(0, surfAOTemp);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0,0,0,255), 1.0f, 0);

    m_fxSSAO->SetTexture("texColor", renderTarget);
    m_fxSSAO->SetTexture("texAO",    m_rtAoTex);

    m_fxSSAO->SetTechnique("TechniqueAO_Composite");
    m_fxSSAO->Begin(NULL, 0);
    m_fxSSAO->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO->EndPass();
    m_fxSSAO->End();

    // ========= RT2 → RT1 へコピー（読み書き競合を避ける）=========
    Common::D3DDevice()->StretchRect(surfAOTemp, NULL, surfRT1, NULL, D3DTEXF_NONE);

    // 後始末
    Common::D3DDevice()->SetRenderTarget(0, oldRt0);

    SAFE_RELEASE(surfAO);
    SAFE_RELEASE(surfAOTemp);
    SAFE_RELEASE(surfRT1);
    SAFE_RELEASE(surfRT2);
    SAFE_RELEASE(oldRt0);

    return renderTarget;
}

void PostEffectSSAO::DrawFullscreenQuad()
{
    static FullscreenVertex vertices[4] =
    {
        { -1.f, -1.f, 0.f, 1.f, 0.f, 1.f },
        { -1.f,  1.f, 0.f, 1.f, 0.f, 0.f },
        {  1.f, -1.f, 0.f, 1.f, 1.f, 1.f },
        {  1.f,  1.f, 0.f, 1.f, 1.f, 0.f }
    };

    static D3DVERTEXELEMENT9 decl[] =
    {
        { 0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    LPDIRECT3DVERTEXDECLARATION9 vertexDecl = NULL;
    Common::D3DDevice()->CreateVertexDeclaration(decl, &vertexDecl);
    Common::D3DDevice()->SetVertexDeclaration(vertexDecl);

    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(FullscreenVertex));

    SAFE_RELEASE(vertexDecl);
}

void PostEffectSSAO::SetEnable(const bool arg)
{

}

void PostEffectSSAO::OnDeviceLost()
{
    m_fxSSAO->OnLostDevice();
    SAFE_RELEASE(m_rtAoTex);
    SAFE_RELEASE(m_rtAoTempTex);

}

void PostEffectSSAO::OnDeviceReset()
{
    m_fxSSAO->OnLostDevice();

    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_rtAoTex);

    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_rtAoTempTex);

    assert(SUCCEEDED(hResult));
}

}
