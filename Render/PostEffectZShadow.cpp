#include "PostEffectZShadow.h"

#include "Camera.h"
#include "MeshMixSkinAnim.h"

#include "Util.h"

namespace NSRender
{
namespace
{
constexpr float SHADOW_VIEW_SIZE_MIN = 1.0f;
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
    if (m_isInitialized)
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectZShadow.cso";
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       effectPath.c_str(),
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &g_fxDepthBufferShadow,
                                       NULL);
    assert(hResult == S_OK);

    CreateRawResource();

    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }

    m_isInitialized = true;
}

void PostEffectZShadow::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }

    SAFE_RELEASE(g_fxDepthBufferShadow);

    SAFE_RELEASE(g_texRenderTargetLightZ);
    SAFE_RELEASE(g_texRenderTargetLightZHalf);
    SAFE_RELEASE(g_texRenderTargetShadow);
    SAFE_RELEASE(g_texRenderTargetShadowHalf);
    SAFE_RELEASE(g_surfaceLightZStensil);
    SAFE_RELEASE(g_surfaceLightZStensilHalf);
    SAFE_RELEASE(g_surfaceShadowStensil);
    SAFE_RELEASE(g_surfaceShadowStensilHalf);
    SAFE_RELEASE(g_pQuadDecl);

    m_isInitialized = false;
}

void PostEffectZShadow::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                             LPDIRECT3DTEXTURE9 texTarget,
                             LPDIRECT3DTEXTURE9 sceneDepthTexture,
                             LPDIRECT3DTEXTURE9 sceneNormalTexture,
                             const std::deque<MeshMixManager>& meshMixList,
                             const std::vector<MeshMixSkinAnim*>& meshMixSkinAnimList)
{
    g_texTemp = renderTarget;
    m_texCompositeTarget = texTarget;
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
    m_texCompositeTarget = NULL;
}

void PostEffectZShadow::RenderTechnique1()
{
    HRESULT hr = E_FAIL;
    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture();
    LPDIRECT3DSURFACE9 activeLightZDepthStencil = GetActiveLightZDepthStencil();

    hr = Common::D3DDevice()->GetRenderTarget(0, &oldRT0);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->GetDepthStencilSurface(&oldZ);
    assert(hr == S_OK);

    LPDIRECT3DSURFACE9 surfaceLightZ = NULL;

    hr = activeLightZTexture->GetSurfaceLevel(0, &surfaceLightZ);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceLightZ);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetDepthStencilSurface(activeLightZDepthStencil);
    assert(hr == S_OK);

    // Viewport をテクスチャのサイズに変更
    // これをしないと一部のエリアにしか描画されない
    D3DSURFACE_DESC descLightZ { };
    hr = activeLightZTexture->GetLevelDesc(0, &descLightZ);
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

    const float viewWidth = CoverageToViewSize(m_coverage);
    const float focusYOffset = viewWidth * 0.35f;
    D3DXVECTOR3 focusPoint = Camera::GetLookAtPos();
    focusPoint.y -= focusYOffset;
    const D3DXVECTOR3 vLightEye = focusPoint + SHADOW_CAMERA_OFFSET;
    const D3DXVECTOR3 vLightAt = (Camera::GetEyePos() + Camera::GetLookAtPos()) * 0.5f;
    D3DXVECTOR3 vLightUp(0, 1, 0);
    D3DXMatrixLookAtLH(&mLightView, &vLightEye, &vLightAt, &vLightUp);

    D3DXVECTOR3 lightToCamera = Camera::GetEyePos() - vLightEye;
    float cameraDistance = D3DXVec3Length(&lightToCamera);
    cameraDistance = (std::max)(cameraDistance, 0.1f);

    fLightNear = (std::max)(0.1f, cameraDistance * 0.5f);
    fLightFar = (std::max)(fLightNear + 0.1f, cameraDistance * 2.0f);

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
        DWORD subsetCount = 1;
        if (mesh.GetSubsetCount() > 0)
        {
            subsetCount = mesh.GetSubsetCount();
        }
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
    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture();
    LPDIRECT3DTEXTURE9 activeShadowTexture = GetActiveShadowTexture();
    LPDIRECT3DSURFACE9 activeShadowDepthStencil = GetActiveShadowDepthStencil();

    LPDIRECT3DSURFACE9 surfaceShadow= NULL;
    hr = activeShadowTexture->GetSurfaceLevel(0, &surfaceShadow);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceShadow);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetDepthStencilSurface(activeShadowDepthStencil);
    assert(hr == S_OK);

    D3DVIEWPORT9 viewportShadow{};
    D3DSURFACE_DESC descShadow{};
    hr = activeShadowTexture->GetLevelDesc(0, &descShadow);
    assert(hr == S_OK);

    viewportShadow.X = 0;
    viewportShadow.Y = 0;

    viewportShadow.Width  = descShadow.Width;
    viewportShadow.Height = descShadow.Height;

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
                               static_cast<float>(descShadow.Width) / static_cast<float>(descShadow.Height),
                               0.1f,
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

    hr = g_fxDepthBufferShadow->SetTexture("g_texLightZ", activeLightZTexture);
    assert(hr == S_OK);

    D3DSURFACE_DESC descLightZ{};

    hr = activeLightZTexture->GetLevelDesc(0, &descLightZ);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelW", 1.0f / (float)descLightZ.Width);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelH", 1.0f / (float)descLightZ.Height);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBias", 0.0002f);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetInt("g_shadowPcfTapCount", m_pcfTapCount);
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

        DWORD subsetCount = 1;
        if (mesh.GetSubsetCount() > 0)
        {
            subsetCount = mesh.GetSubsetCount();
        }
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

    hr = Common::D3DDevice()->SetDepthStencilSurface(oldZ);
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

    // ★ 合成先RT（m_texCompositeTarget）へ切り替え
    LPDIRECT3DSURFACE9 rtComp = NULL;
    m_texCompositeTarget->GetSurfaceLevel(0, &rtComp);
    Common::D3DDevice()->SetRenderTarget(0, rtComp);

    // ★ 合成先サイズのビューポートを設定
    D3DSURFACE_DESC descComp{};
    m_texCompositeTarget->GetLevelDesc(0, &descComp);
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

    LPDIRECT3DTEXTURE9 activeShadowTexture = GetActiveShadowTexture();
    D3DSURFACE_DESC descShadow{};
    activeShadowTexture->GetLevelDesc(0, &descShadow);
    g_fxDepthBufferShadow->SetFloat("g_compositeTexelW", 1.0f / static_cast<float>(descShadow.Width));
    g_fxDepthBufferShadow->SetFloat("g_compositeTexelH", 1.0f / static_cast<float>(descShadow.Height));
    g_fxDepthBufferShadow->SetTexture("g_texBase",   g_texTemp);               // 元のカラー
    g_fxDepthBufferShadow->SetTexture("g_texShadow", activeShadowTexture);     // 影アルファ
    g_fxDepthBufferShadow->SetTexture("g_texSceneDepth", m_sceneDepthTexture);
    g_fxDepthBufferShadow->SetTexture("g_texSceneNormal", m_sceneNormalTexture);
    g_fxDepthBufferShadow->SetFloat("g_shadowIntensity", m_shadowIntensity);
    g_fxDepthBufferShadow->SetFloat("g_shadowSaturationBoost", m_shadowSaturationBoost);
    g_fxDepthBufferShadow->SetFloat("g_edgeDepthThreshold", 0.010f);
    g_fxDepthBufferShadow->SetFloat("g_edgeNormalThreshold", 0.50f);
    g_fxDepthBufferShadow->SetInt("g_shadowPcfTapCount", m_pcfTapCount);
    g_fxDepthBufferShadow->SetInt("g_shadowCompositeTapCount", m_compositeTapCount);
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

void PostEffectZShadow::DrawDebugLightDepthOverlay(const int x,
                                                   const int y,
                                                   const int width,
                                                   const int height)
{
    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture();
    if (g_fxDepthBufferShadow == NULL || activeLightZTexture == NULL)
    {
        return;
    }

    D3DVIEWPORT9 oldViewport { };
    Common::D3DDevice()->GetViewport(&oldViewport);

    D3DVIEWPORT9 debugViewport { };
    debugViewport.X = static_cast<DWORD>(x);
    debugViewport.Y = static_cast<DWORD>(y);
    debugViewport.Width = static_cast<DWORD>((std::max)(1, width));
    debugViewport.Height = static_cast<DWORD>((std::max)(1, height));
    debugViewport.MinZ = 0.0f;
    debugViewport.MaxZ = 1.0f;
    Common::D3DDevice()->SetViewport(&debugViewport);

    HRESULT hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDebugLightZ");
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTexture("g_texLightZ", activeLightZTexture);
    assert(hr == S_OK);

    UINT passCount = 0;
    hr = g_fxDepthBufferShadow->Begin(&passCount, 0);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->BeginPass(0);
    assert(hr == S_OK);

    DrawFullscreenQuad();

    hr = g_fxDepthBufferShadow->EndPass();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->End();
    assert(hr == S_OK);

    hr = Common::D3DDevice()->EndScene();
    assert(hr == S_OK);

    Common::D3DDevice()->SetViewport(&oldViewport);
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

void PostEffectZShadow::SetPcfTapCount(const int tapCount)
{
    m_pcfTapCount = NormalizeShadowBlurTapCount(tapCount);
}

void PostEffectZShadow::SetCompositeTapCount(const int tapCount)
{
    m_compositeTapCount = NormalizeShadowBlurTapCount(tapCount);
}

void PostEffectZShadow::SetShadowTextureScaleDivisor(const int scaleDivisor)
{
    if (scaleDivisor == 2)
    {
        m_shadowTextureScaleDivisor = 2;
    }
    else
    {
        m_shadowTextureScaleDivisor = 1;
    }
}

void PostEffectZShadow::OnDeviceLost()
{
    if (!m_isInitialized || g_fxDepthBufferShadow == NULL)
    {
        return;
    }

    g_fxDepthBufferShadow->OnLostDevice();
    SAFE_RELEASE(g_texTemp);
    SAFE_RELEASE(g_texRenderTargetLightZ);
    SAFE_RELEASE(g_texRenderTargetLightZHalf);
    SAFE_RELEASE(g_surfaceLightZStensil);
    SAFE_RELEASE(g_surfaceLightZStensilHalf);
    SAFE_RELEASE(g_texRenderTargetShadow);
    SAFE_RELEASE(g_texRenderTargetShadowHalf);
    SAFE_RELEASE(g_surfaceShadowStensil);
    SAFE_RELEASE(g_surfaceShadowStensilHalf);
    SAFE_RELEASE(g_pQuadDecl);
}

void PostEffectZShadow::OnDeviceReset()
{
    if (!m_isInitialized || g_fxDepthBufferShadow == NULL)
    {
        return;
    }

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
                                (std::max)(1, Common::ScreenW()),
                                (std::max)(1, Common::ScreenH()),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_R32F,
                                D3DPOOL_DEFAULT,
                                &g_texRenderTargetLightZHalf);

    assert(hResult == S_OK);

    D3DSURFACE_DESC halfDesc{};
    g_texRenderTargetLightZHalf->GetLevelDesc(0, &halfDesc);

    hr = Common::D3DDevice()->CreateDepthStencilSurface(halfDesc.Width,
                                                        halfDesc.Height,
                                                        D3DFMT_D16,
                                                        D3DMULTISAMPLE_NONE,
                                                        0,
                                                        TRUE,
                                                        &g_surfaceLightZStensilHalf,
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

    hr = Common::D3DDevice()->CreateDepthStencilSurface((std::max)(1, Common::ScreenW()),
                                                        (std::max)(1, Common::ScreenH()),
                                                        D3DFMT_D16,
                                                        D3DMULTISAMPLE_NONE,
                                                        0,
                                                        TRUE,
                                                        &g_surfaceShadowStensil,
                                                        NULL);
    assert(hr == S_OK);

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                (std::max)(1, Common::ScreenW() / 2),
                                (std::max)(1, Common::ScreenH() / 2),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &g_texRenderTargetShadowHalf);
    assert(hResult == S_OK);

    hr = Common::D3DDevice()->CreateDepthStencilSurface((std::max)(1, Common::ScreenW() / 2),
                                                        (std::max)(1, Common::ScreenH() / 2),
                                                        D3DFMT_D16,
                                                        D3DMULTISAMPLE_NONE,
                                                        0,
                                                        TRUE,
                                                        &g_surfaceShadowStensilHalf,
                                                        NULL);
    assert(hr == S_OK);

    D3DVERTEXELEMENT9 elems[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    hResult = Common::D3DDevice()->CreateVertexDeclaration(elems, &g_pQuadDecl);
    assert(hResult == S_OK);
}

LPDIRECT3DTEXTURE9 PostEffectZShadow::GetActiveLightZTexture() const
{
    if (m_shadowTextureScaleDivisor == 2)
    {
        return g_texRenderTargetLightZHalf;
    }

    return g_texRenderTargetLightZ;
}

LPDIRECT3DSURFACE9 PostEffectZShadow::GetActiveLightZDepthStencil() const
{
    if (m_shadowTextureScaleDivisor == 2)
    {
        return g_surfaceLightZStensilHalf;
    }

    return g_surfaceLightZStensil;
}

LPDIRECT3DTEXTURE9 PostEffectZShadow::GetActiveShadowTexture() const
{
    if (m_shadowTextureScaleDivisor == 2)
    {
        return g_texRenderTargetShadowHalf;
    }

    return g_texRenderTargetShadow;
}

LPDIRECT3DSURFACE9 PostEffectZShadow::GetActiveShadowDepthStencil() const
{
    if (m_shadowTextureScaleDivisor == 2)
    {
        return g_surfaceShadowStensilHalf;
    }

    return g_surfaceShadowStensil;
}

}

