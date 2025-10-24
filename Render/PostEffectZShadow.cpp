#include "PostEffectZShadow.h"

#include "Camera.h"

namespace NSRender
{
void PostEffectZShadow::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       L"../x64/Debug/PostEffectZShadow.cso",
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &g_fxDepthBufferShadow,
                                       NULL);
    assert(hResult == S_OK);

    CreateTexture();

    D3DVERTEXELEMENT9 elems[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    hResult = Common::D3DDevice()->CreateVertexDeclaration(elems, &g_pQuadDecl);
    assert(hResult == S_OK);

    Common::AddDeviceLostResource(this);
}

void PostEffectZShadow::Finalize()
{
    SAFE_RELEASE(g_fxDepthBufferShadow);

    SAFE_RELEASE(g_texRenderTargetLightZ);
    SAFE_RELEASE(g_texRenderTargetShadow);
    SAFE_RELEASE(g_pQuadDecl);
}

LPDIRECT3DTEXTURE9 PostEffectZShadow::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                           const std::vector<MeshMix>& meshMixList)
{
    g_texTemp = renderTarget;
    m_pMeshList = &meshMixList;

    RenderTechnique1();
    RenderTechnique2();
    RenderTechnique3();

    m_pMeshList = nullptr;

    return g_texComposite;
}

void PostEffectZShadow::RenderTechnique1()
{
    HRESULT hr = E_FAIL;

    hr = Common::D3DDevice()->GetRenderTarget(0, &oldRT0);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->GetDepthStencilSurface(&oldZ);
    assert(hr == S_OK);

    LPDIRECT3DSURFACE9 surfaceLightZ = NULL;

    hr = g_texRenderTargetLightZ->GetSurfaceLevel(0, &surfaceLightZ);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceLightZ);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetDepthStencilSurface(g_surfaceLightZStensil);
    assert(hr == S_OK);

    // Viewport をテクスチャのサイズに変更
    // これをしないと一部のエリアにしか描画されない
    D3DSURFACE_DESC descLightZ { };
    hr = g_texRenderTargetLightZ->GetLevelDesc(0, &descLightZ);
    assert(hr == S_OK);

    D3DVIEWPORT9 oldViewPort { };
    hr = Common::D3DDevice()->GetViewport(&oldViewPort);
    assert(hr == S_OK);

    D3DVIEWPORT9 viewPortLightZ{};
    viewPortLightZ.X = 0;
    viewPortLightZ.Y = 0;
    viewPortLightZ.Width  = descLightZ.Width;
    viewPortLightZ.Height = descLightZ.Height;
    viewPortLightZ.MinZ = 0.0f;
    viewPortLightZ.MaxZ = 1.0f;

    hr = Common::D3DDevice()->SetViewport(&viewPortLightZ);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->Clear(0, NULL,
                             D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                             D3DCOLOR_XRGB(255, 255, 255),
                             1.0f,
                             0);
    assert(hr == S_OK);

    D3DXVECTOR3 vLightEye(40, 50, -40);
    D3DXVECTOR3 vLightAt(0, 0, 0);
    D3DXVECTOR3 vLightUp(0, 1, 0);
    D3DXMatrixLookAtLH(&mLightView, &vLightEye, &vLightAt, &vLightUp);

    float viewWidth = 70.0f;
    float viewHeight = 70.0f;
    D3DXMatrixOrthoLH(&mLightProj, viewWidth, viewHeight, fLightNear, fLightFar);

    hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDepthFromLight");
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightView", &mLightView);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat ("g_lightNear", fLightNear);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat ("g_lightFar", fLightFar);
    assert(hr == S_OK);

    g_fxDepthBufferShadow->Begin(NULL, 0);
    g_fxDepthBufferShadow->BeginPass(0);

    for (auto& mesh : *m_pMeshList)
    {
        D3DXMATRIX mWorld;
        D3DXMATRIX mWorldViewProjLight;

        D3DXMatrixTranslation(&mWorld,
                              mesh.GetPos().x,
                              mesh.GetPos().y,
                              mesh.GetPos().z);

        mWorldViewProjLight = mWorld * mLightView * mLightProj;
        
        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorld", &mWorld);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorldViewProj", &mWorldViewProjLight);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->CommitChanges();
        assert(hr == S_OK);

        // メッシュ本体を描画
        LPD3DXMESH d3dMesh = mesh.GetD3DMesh();
        d3dMesh->DrawSubset(0);
    }

    g_fxDepthBufferShadow->EndPass();
    g_fxDepthBufferShadow->End();

    hr = Common::D3DDevice()->EndScene();
    assert(hr == S_OK);

    SAFE_RELEASE(surfaceLightZ);
    
    Common::D3DDevice()->SetViewport(&oldViewPort);
}

void PostEffectZShadow::RenderTechnique2()
{
    HRESULT hr = E_FAIL;

    LPDIRECT3DSURFACE9 surfaceShadow= NULL;
    hr = g_texRenderTargetShadow->GetSurfaceLevel(0, &surfaceShadow);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceShadow);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetDepthStencilSurface(oldZ);
    assert(hr == S_OK);

    D3DVIEWPORT9 viewportShadow{};

    viewportShadow.X = 0;
    viewportShadow.Y = 0;

    viewportShadow.Width  = Common::ScreenW();
    viewportShadow.Height = Common::ScreenH();

    viewportShadow.MinZ = 0.0f;
    viewportShadow.MaxZ = 1.0f;

    Common::D3DDevice()->SetViewport(&viewportShadow);

    hr = Common::D3DDevice()->Clear(0, NULL,
                             D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                             D3DCOLOR_ARGB(0, 0, 0, 0),
                             1.0f,
                             0);
    assert(hr == S_OK);

    // カメラ行列
    D3DXMATRIX mView;
    D3DXMATRIX mProj;

    D3DXMatrixPerspectiveFovLH(&mProj,
                               D3DXToRadian(45.0f),
                               (float)Common::ScreenW() / Common::ScreenH(),
                               1.0f,
                               100.0f);

    D3DXVECTOR3 vEye(Camera::GetEyePos());
    D3DXVECTOR3 vAt(Camera::GetLookAtPos());
    D3DXVECTOR3 vUp(0, 1, 0);

    D3DXMatrixLookAtLH(&mView, &vEye, &vAt, &vUp);

    D3DXMATRIX mLightViewProj = mLightView * mLightProj;

    hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewProj", &mLightViewProj);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightView", &mLightView);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_lightNear", fLightNear);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_lightFar", fLightFar);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTexture("g_texLightZ", g_texRenderTargetLightZ);
    assert(hr == S_OK);

    D3DSURFACE_DESC descLightZ{};

    hr = g_texRenderTargetLightZ->GetLevelDesc(0, &descLightZ);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelW", 1.0f / (float)descLightZ.Width);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelH", 1.0f / (float)descLightZ.Height);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBias",   0.001f);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowIntensity", 0.5f);
    assert(hr == S_OK);

    int nBlurSize = 3;

    hr = g_fxDepthBufferShadow->SetInt("g_nBlurSize", nBlurSize);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueWriteShadow");
    assert(hr == S_OK);

    UINT nPassNum = 0;

    hr = g_fxDepthBufferShadow->Begin(&nPassNum, 0);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->BeginPass(0);
    assert(hr == S_OK);

    for (auto& mesh : *m_pMeshList)
    {
        D3DXMATRIX mWorld;
        D3DXMatrixTranslation(&mWorld,
                              mesh.GetPos().x,
                              mesh.GetPos().y,
                              mesh.GetPos().z);

        D3DXMATRIX mWorldViewProj;
        mWorldViewProj = mWorld * mView * mProj;

        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorld", &mWorld);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorldViewProj", &mWorldViewProj);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->CommitChanges();
        assert(hr == S_OK);

        hr = mesh.GetD3DMesh()->DrawSubset(0);
        assert(hr == S_OK);
    }

    hr = g_fxDepthBufferShadow->EndPass();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->End();
    assert(hr == S_OK);

    hr = Common::D3DDevice()->EndScene();
    assert(hr == S_OK);

    SAFE_RELEASE(surfaceShadow);

    hr = Common::D3DDevice()->SetRenderTarget(0, oldRT0);
    assert(hr == S_OK);

    SAFE_RELEASE(oldRT0);
    SAFE_RELEASE(oldZ);
}

void PostEffectZShadow::RenderTechnique3()
{
    HRESULT hr = E_FAIL;

    // ★ いったん現在の RT を退避
    LPDIRECT3DSURFACE9 prevRT = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &prevRT);

    // ★ 合成先RT（g_texComposite）へ切り替え
    LPDIRECT3DSURFACE9 rtComp = NULL;
    g_texComposite->GetSurfaceLevel(0, &rtComp);
    Common::D3DDevice()->SetRenderTarget(0, rtComp);

    // ★ 合成先サイズのビューポートを設定
    D3DSURFACE_DESC descComp{};
    g_texComposite->GetLevelDesc(0, &descComp);
    D3DVIEWPORT9 vp{};
    vp.X = 0; vp.Y = 0; vp.Width = descComp.Width; vp.Height = descComp.Height;
    vp.MinZ = 0.0f; vp.MaxZ = 1.0f;
    Common::D3DDevice()->SetViewport(&vp);

    // ★ ここでは画面は触らない：Z無効/画面クリアは不要
    Common::D3DDevice()->BeginScene();

    g_fxDepthBufferShadow->SetTechnique("TechniqueComposite");
    UINT nPassNum = 0;
    g_fxDepthBufferShadow->Begin(&nPassNum, 0);
    g_fxDepthBufferShadow->BeginPass(0);

    g_fxDepthBufferShadow->SetTexture("g_texBase",   g_texTemp);               // 元のカラー
    g_fxDepthBufferShadow->SetTexture("g_texShadow", g_texRenderTargetShadow); // 影アルファ
    g_fxDepthBufferShadow->CommitChanges();

    DrawFullscreenQuad();

    g_fxDepthBufferShadow->EndPass();
    g_fxDepthBufferShadow->End();
    Common::D3DDevice()->EndScene();

    // ★ RT を元に戻す
    Common::D3DDevice()->SetRenderTarget(0, prevRT);

    SAFE_RELEASE(rtComp);
    SAFE_RELEASE(prevRT);
}

void PostEffectZShadow::DrawFullscreenQuad()
{
    QuadVertex v[4] { };

    float du = 0.5f / (float)Common::ScreenW();
    float dv = 0.5f / (float)Common::ScreenH();

    v[0].x = -1.0f;
    v[0].y = -1.0f;
    v[0].z = 0.0f;
    v[0].w = 1.0f;
    v[0].u = 0.0f + du;
    v[0].v = 1.0f - dv;

    v[1].x = -1.0f;
    v[1].y = 1.0f;
    v[1].z = 0.0f;
    v[1].w = 1.0f;
    v[1].u = 0.0f + du;
    v[1].v = 0.0f + dv;

    v[2].x = 1.0f;
    v[2].y = -1.0f;
    v[2].z = 0.0f;
    v[2].w = 1.0f;
    v[2].u = 1.0f - du;
    v[2].v = 1.0f - dv;

    v[3].x = 1.0f;
    v[3].y = 1.0f;
    v[3].z = 0.0f;
    v[3].w = 1.0f;
    v[3].u = 1.0f - du;
    v[3].v = 0.0f + dv;

    HRESULT hr = E_FAIL;

    hr = Common::D3DDevice()->SetVertexDeclaration(g_pQuadDecl);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(QuadVertex));
    assert(hr == S_OK);
}

void PostEffectZShadow::SetEnable(const bool arg)
{
    m_bEnable = arg;
}

void PostEffectZShadow::OnDeviceLost()
{
    g_fxDepthBufferShadow->OnLostDevice();
    SAFE_RELEASE(g_texTemp);
    SAFE_RELEASE(g_texRenderTargetLightZ);
    SAFE_RELEASE(g_texRenderTargetShadow);
    SAFE_RELEASE(g_texComposite);
}

void PostEffectZShadow::OnDeviceReset()
{
    g_fxDepthBufferShadow->OnResetDevice();
    CreateTexture();
}

void PostEffectZShadow::CreateTexture()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW() * 2,
                                Common::ScreenH() * 2,
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_R32F,
                                D3DPOOL_DEFAULT,
                                &g_texRenderTargetLightZ);

    assert(hResult == S_OK);

    D3DSURFACE_DESC bdesc{};
    g_texRenderTargetLightZ->GetLevelDesc(0, &bdesc);

    // 影用の深度ステンシル（サイズをRT2に合わせる）
    HRESULT hr = Common::D3DDevice()->CreateDepthStencilSurface(bdesc.Width,
                                                         bdesc.Height,
                                                         D3DFMT_D16,
                                                         D3DMULTISAMPLE_NONE,
                                                         0,
                                                         TRUE,
                                                         &g_surfaceLightZStensil,
                                                         NULL);
    assert(hr == S_OK);

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &g_texRenderTargetShadow);
    assert(hResult == S_OK);

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                        Common::ScreenW(),
                                        Common::ScreenH(),
                                        1,
                                        D3DUSAGE_RENDERTARGET,
                                        D3DFMT_A8R8G8B8,
                                        D3DPOOL_DEFAULT,
                                        &g_texComposite);
    assert(hResult == S_OK);

}

}

