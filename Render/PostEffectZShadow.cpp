#include "PostEffectZShadow.h"

#include "Camera.h"
#include "MeshMixSkinAnim.h"

namespace NSRender
{
namespace
{
constexpr float SHADOW_VIEW_SIZE_MIN = 3.0f;
constexpr float SHADOW_VIEW_SIZE_MAX = 120.0f;
constexpr int SHADOW_BLUR_TAP_COUNT_MIN = 1;
constexpr int SHADOW_BLUR_TAP_COUNT_MAX = 11;
const D3DXVECTOR3 SHADOW_CAMERA_OFFSET(40.0f, 50.0f, -40.0f);

D3DXMATRIX BuildMeshWorldMatrix(const MeshMixManager& mesh)
{
    D3DXMATRIX matWorld{};
    D3DXMatrixIdentity(&matWorld);

    D3DXMATRIX matWork{};
    D3DXMatrixIdentity(&matWork);

    D3DXMatrixScaling(&matWork, mesh.GetScale(), mesh.GetScale(), mesh.GetScale());
    matWorld *= matWork;

    const D3DXVECTOR3 rotation = mesh.GetRot();
    D3DXMatrixRotationYawPitchRoll(&matWork, rotation.y, rotation.x, rotation.z);
    matWorld *= matWork;

    const D3DXVECTOR3 position = mesh.GetPos();
    D3DXMatrixTranslation(&matWork, position.x, position.y, position.z);
    matWorld *= matWork;

    return matWorld;
}

float ClampZeroToOne(const float value)
{
    return (std::max)(0.0f, (std::min)(value, 1.0f));
}

int NormalizeShadowBlurTapCount(const int tapCount)
{
    int normalized = (std::max)(SHADOW_BLUR_TAP_COUNT_MIN, (std::min)(tapCount, SHADOW_BLUR_TAP_COUNT_MAX));
    if ((normalized % 2) == 0)
    {
        --normalized;
    }
    return (std::max)(SHADOW_BLUR_TAP_COUNT_MIN, normalized);
}

float CoverageToViewSize(const float coverage)
{
    return SHADOW_VIEW_SIZE_MIN + ((SHADOW_VIEW_SIZE_MAX - SHADOW_VIEW_SIZE_MIN) * ClampZeroToOne(coverage));
}
}

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

    CreateRawResource();

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
                                           LPDIRECT3DTEXTURE9 sceneDepthTexture,
                                           LPDIRECT3DTEXTURE9 sceneNormalTexture,
                                           const std::deque<MeshMixManager>& meshMixList,
                                           const std::vector<MeshMixSkinAnim*>& meshMixSkinAnimList)
{
    if (!m_bEnable)
    {
        return renderTarget;
    }
    g_texTemp = renderTarget;
    m_pMeshList = &meshMixList;
    m_pSkinAnimMeshList = &meshMixSkinAnimList;
    m_sceneDepthTexture = sceneDepthTexture;
    m_sceneNormalTexture = sceneNormalTexture;

    RenderTechnique1();
    RenderTechnique2();
    RenderTechnique3();

    m_pMeshList = nullptr;
    m_pSkinAnimMeshList = nullptr;
    m_sceneDepthTexture = NULL;
    m_sceneNormalTexture = NULL;

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

    const D3DXVECTOR3 focusPoint = Camera::GetEyePos();
    const D3DXVECTOR3 vLightEye = focusPoint + SHADOW_CAMERA_OFFSET;
    const D3DXVECTOR3 vLightAt = focusPoint;
    D3DXVECTOR3 vLightUp(0, 1, 0);
    D3DXMatrixLookAtLH(&mLightView, &vLightEye, &vLightAt, &vLightUp);

    const float viewWidth = CoverageToViewSize(m_coverage);
    const float viewHeight = viewWidth;
    D3DXMatrixOrthoLH(&mLightProj, viewWidth, viewHeight, fLightNear, fLightFar);

    hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDepthFromLight");
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightView", &mLightView);
    assert(hr == S_OK);

    D3DXMATRIX mLightViewProj = mLightView * mLightProj;
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewProj", &mLightViewProj);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat ("g_lightNear", fLightNear);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat ("g_lightFar", fLightFar);
    assert(hr == S_OK);

    g_fxDepthBufferShadow->Begin(NULL, 0);
    g_fxDepthBufferShadow->BeginPass(0);

    for (auto& mesh : *m_pMeshList)
    {
        if (!mesh.IsEnabled())
        {
            continue;
        }

        if (!mesh.IsLoaded())
        {
            continue;
        }

        if (!mesh.IsDepthBufferShadowEnabled())
        {
            continue;
        }

        D3DXMATRIX mWorld = BuildMeshWorldMatrix(mesh);
        D3DXMATRIX mWorldViewProjLight;

        mWorldViewProjLight = mWorld * mLightView * mLightProj;
        
        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorld", &mWorld);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorldViewProj", &mWorldViewProjLight);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->CommitChanges();
        assert(hr == S_OK);

        // subset ごとに描画
        LPD3DXMESH d3dMesh = mesh.GetD3DMesh();
        const DWORD subsetCount = (mesh.GetSubsetCount() > 0) ? mesh.GetSubsetCount() : 1;
        for (DWORD subsetIndex = 0; subsetIndex < subsetCount; ++subsetIndex)
        {
            d3dMesh->DrawSubset(subsetIndex);
        }
    }

    g_fxDepthBufferShadow->EndPass();
    g_fxDepthBufferShadow->End();

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDepthFromLightSkin");
    assert(hr == S_OK);

    for (auto& mesh : *m_pSkinAnimMeshList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(g_fxDepthBufferShadow);
        }
    }

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

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBias", 0.0002f);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetInt("g_shadowBlurTapCount", m_blurTapCount);
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
        if (!mesh.IsEnabled())
        {
            continue;
        }

        if (!mesh.IsLoaded())
        {
            continue;
        }

        if (!mesh.IsDepthBufferShadowEnabled())
        {
            continue;
        }

        D3DXMATRIX mWorld = BuildMeshWorldMatrix(mesh);

        D3DXMATRIX mWorldViewProj;
        mWorldViewProj = mWorld * mView * mProj;

        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorld", &mWorld);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetMatrix("g_matWorldViewProj", &mWorldViewProj);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->CommitChanges();
        assert(hr == S_OK);

        const DWORD subsetCount = (mesh.GetSubsetCount() > 0) ? mesh.GetSubsetCount() : 1;
        for (DWORD subsetIndex = 0; subsetIndex < subsetCount; ++subsetIndex)
        {
            hr = mesh.GetD3DMesh()->DrawSubset(subsetIndex);
            assert(hr == S_OK);
        }
    }

    hr = g_fxDepthBufferShadow->EndPass();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->End();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueWriteShadowSkin");
    assert(hr == S_OK);

    D3DXMATRIX mViewProj = mView * mProj;
    hr = g_fxDepthBufferShadow->SetMatrix("g_matWorldViewProj", &mViewProj);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->CommitChanges();
    assert(hr == S_OK);

    for (auto& mesh : *m_pSkinAnimMeshList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(g_fxDepthBufferShadow);
        }
    }

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

    g_fxDepthBufferShadow->SetFloat("g_compositeTexelW", 1.0f / static_cast<float>(descComp.Width));
    g_fxDepthBufferShadow->SetFloat("g_compositeTexelH", 1.0f / static_cast<float>(descComp.Height));
    g_fxDepthBufferShadow->SetTexture("g_texBase",   g_texTemp);               // 元のカラー
    g_fxDepthBufferShadow->SetTexture("g_texShadow", g_texRenderTargetShadow); // 影アルファ
    g_fxDepthBufferShadow->SetTexture("g_texSceneDepth", m_sceneDepthTexture);
    g_fxDepthBufferShadow->SetTexture("g_texSceneNormal", m_sceneNormalTexture);
    g_fxDepthBufferShadow->SetFloat("g_shadowIntensity", m_shadowIntensity);
    g_fxDepthBufferShadow->SetFloat("g_shadowSaturationBoost", m_shadowSaturationBoost);
    g_fxDepthBufferShadow->SetFloat("g_edgeDepthThreshold", 0.010f);
    g_fxDepthBufferShadow->SetFloat("g_edgeNormalThreshold", 0.50f);
    g_fxDepthBufferShadow->SetInt("g_shadowBlurTapCount", m_blurTapCount);
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
    const float dx = 1.0f / static_cast<float>(Common::ScreenW());
    const float dy = 1.0f / static_cast<float>(Common::ScreenH());

    v[0].x = -1.0f;
    v[0].y = -1.0f;
    v[0].z = 0.0f;
    v[0].w = 1.0f;
    v[0].u = 0.0f;
    v[0].v = 1.0f;

    v[1].x = -1.0f;
    v[1].y = 1.0f;
    v[1].z = 0.0f;
    v[1].w = 1.0f;
    v[1].u = 0.0f;
    v[1].v = 0.0f;

    v[2].x = 1.0f;
    v[2].y = -1.0f;
    v[2].z = 0.0f;
    v[2].w = 1.0f;
    v[2].u = 1.0f;
    v[2].v = 1.0f;

    v[3].x = 1.0f;
    v[3].y = 1.0f;
    v[3].z = 0.0f;
    v[3].w = 1.0f;
    v[3].u = 1.0f;
    v[3].v = 0.0f;

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

void PostEffectZShadow::SetShadowIntensity(const float intensity)
{
    m_shadowIntensity = intensity;
}

void PostEffectZShadow::SetShadowSaturationBoost(const float saturationBoost)
{
    m_shadowSaturationBoost = saturationBoost;
}

void PostEffectZShadow::SetCoverage(const float coverage)
{
    m_coverage = ClampZeroToOne(coverage);
}

void PostEffectZShadow::SetBlurTapCount(const int tapCount)
{
    m_blurTapCount = NormalizeShadowBlurTapCount(tapCount);
}

void PostEffectZShadow::OnDeviceLost()
{
    g_fxDepthBufferShadow->OnLostDevice();
    SAFE_RELEASE(g_texTemp);
    SAFE_RELEASE(g_texRenderTargetLightZ);
    SAFE_RELEASE(g_surfaceLightZStensil);
    SAFE_RELEASE(g_texRenderTargetShadow);
    SAFE_RELEASE(g_texComposite);
    SAFE_RELEASE(g_pQuadDecl);
}

void PostEffectZShadow::OnDeviceReset()
{
    g_fxDepthBufferShadow->OnResetDevice();
    CreateRawResource();
}

void PostEffectZShadow::CreateRawResource()
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
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &g_texRenderTargetShadow);
    assert(hResult == S_OK);

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                        Common::ScreenW(),
                                        Common::ScreenH(),
                                        1,
                                        D3DUSAGE_RENDERTARGET,
                                        D3DFMT_A16B16G16R16F,
                                        D3DPOOL_DEFAULT,
                                        &g_texComposite);
    assert(hResult == S_OK);

    D3DVERTEXELEMENT9 elems[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    hResult = Common::D3DDevice()->CreateVertexDeclaration(elems, &g_pQuadDecl);
    assert(hResult == S_OK);
}

}

