#include "PostEffectZShadow.h"

#include "Camera.h"
#include "MeshInstancing.h"
#include "MeshMixAnimNoBone.h"
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
constexpr int SHADOW_TEX_SIZE_DIVISOR_1 = 1;
constexpr int SHADOW_TEX_SIZE_DIVISOR_2 = 2;
constexpr int SHADOW_TEX_SIZE_DIVISOR_4 = 4;
constexpr int SHADOW_TEX_SIZE_DIVISOR_8 = 8;
constexpr int SHADOW_TEX_SIZE_DIVISOR_16 = 16;
constexpr int SHADOW_FAR_RESOLUTION_DIVISOR = 2;
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

int NormalizeShadowTextureScaleDivisor(const int scaleDivisor)
{
    switch (scaleDivisor)
    {
    case SHADOW_TEX_SIZE_DIVISOR_2:
    case SHADOW_TEX_SIZE_DIVISOR_4:
    case SHADOW_TEX_SIZE_DIVISOR_8:
    case SHADOW_TEX_SIZE_DIVISOR_16:
        return scaleDivisor;
    default:
        return SHADOW_TEX_SIZE_DIVISOR_1;
    }
}

int ShadowTextureScaleDivisorToVariantIndex(const int scaleDivisor)
{
    switch (NormalizeShadowTextureScaleDivisor(scaleDivisor))
    {
    case SHADOW_TEX_SIZE_DIVISOR_2:
        return 1;
    case SHADOW_TEX_SIZE_DIVISOR_4:
        return 2;
    case SHADOW_TEX_SIZE_DIVISOR_8:
        return 3;
    case SHADOW_TEX_SIZE_DIVISOR_16:
        return 4;
    default:
        return 0;
    }
}

int VariantIndexToShadowTextureScaleDivisor(const int variantIndex)
{
    switch (variantIndex)
    {
    case 1:
        return SHADOW_TEX_SIZE_DIVISOR_2;
    case 2:
        return SHADOW_TEX_SIZE_DIVISOR_4;
    case 3:
        return SHADOW_TEX_SIZE_DIVISOR_8;
    case 4:
        return SHADOW_TEX_SIZE_DIVISOR_16;
    default:
        return SHADOW_TEX_SIZE_DIVISOR_1;
    }
}

UINT ComputeLightZTextureSize(const int screenSize, const int scaleDivisor)
{
    const int normalizedDivisor = NormalizeShadowTextureScaleDivisor(scaleDivisor);
    return static_cast<UINT>((std::max)(1, (screenSize * 2) / normalizedDivisor));
}

UINT ComputeShadowTextureSize(const int screenSize, const int scaleDivisor)
{
    const int normalizedDivisor = NormalizeShadowTextureScaleDivisor(scaleDivisor);
    return static_cast<UINT>((std::max)(1, screenSize / normalizedDivisor));
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

    for (int cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_COUNT; ++cascadeIndex)
    {
        for (int variantIndex = 0; variantIndex < SHADOW_TEX_SIZE_VARIANT_COUNT; ++variantIndex)
        {
            SAFE_RELEASE(g_texRenderTargetLightZ[cascadeIndex][variantIndex]);
            SAFE_RELEASE(g_texRenderTargetShadow[cascadeIndex][variantIndex]);
            SAFE_RELEASE(g_surfaceLightZStensil[cascadeIndex][variantIndex]);
            SAFE_RELEASE(g_surfaceShadowStensil[cascadeIndex][variantIndex]);
        }
    }
    SAFE_RELEASE(g_pQuadDecl);

    m_isInitialized = false;
}

void PostEffectZShadow::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                             LPDIRECT3DTEXTURE9 texTarget,
                             LPDIRECT3DTEXTURE9 sceneDepthTexture,
                              LPDIRECT3DTEXTURE9 sceneNormalTexture,
                              const std::deque<MeshMixManager>& meshMixList,
                              const std::vector<MeshMixSkinAnim*>& meshMixSkinAnimList,
                              const std::vector<MeshMixAnimNoBone*>& meshMixAnimNoBoneList,
                              const std::unordered_map<std::wstring, MeshInstancing*>& meshInstancingMap)
{
    g_texTemp = renderTarget;
    m_texCompositeTarget = texTarget;
    m_pMeshList = &meshMixList;
    m_pSkinAnimMeshList = &meshMixSkinAnimList;
    m_pMeshMixAnimNoBoneList = &meshMixAnimNoBoneList;
    m_pMeshInstancingMap = &meshInstancingMap;
    m_sceneDepthTexture = sceneDepthTexture;
    m_sceneNormalTexture = sceneNormalTexture;

    RenderTechnique1(SHADOW_CASCADE_NEAR);
    RenderTechnique2(SHADOW_CASCADE_NEAR);
    RenderTechnique1(SHADOW_CASCADE_FAR);
    RenderTechnique2(SHADOW_CASCADE_FAR);
    RenderTechnique3();

    m_pMeshList = nullptr;
    m_pSkinAnimMeshList = nullptr;
    m_pMeshMixAnimNoBoneList = nullptr;
    m_pMeshInstancingMap = nullptr;
    m_sceneDepthTexture = NULL;
    m_sceneNormalTexture = NULL;
    m_texCompositeTarget = NULL;
}

void PostEffectZShadow::RenderTechnique1(const int cascadeIndex)
{
    HRESULT hr = E_FAIL;
    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture(cascadeIndex);
    LPDIRECT3DSURFACE9 activeLightZDepthStencil = GetActiveLightZDepthStencil(cascadeIndex);

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

    float coverage = m_coverage;
    if (cascadeIndex == SHADOW_CASCADE_FAR)
    {
        coverage = m_coverageFar;
    }
    const float viewWidth = CoverageToViewSize(coverage);
    const float nearViewWidth = CoverageToViewSize(m_coverage);
    const float focusYOffset = nearViewWidth * 0.35f;
    D3DXVECTOR3 focusPoint = Camera::GetLookAtPos();
    focusPoint.y -= focusYOffset;
    const D3DXVECTOR3 vLightEye = focusPoint + SHADOW_CAMERA_OFFSET;
    const D3DXVECTOR3 vLightAt = (Camera::GetEyePos() + Camera::GetLookAtPos()) * 0.5f;
    D3DXVECTOR3 vLightUp(0, 1, 0);
    D3DXMatrixLookAtLH(&mLightView[cascadeIndex], &vLightEye, &vLightAt, &vLightUp);

    D3DXVECTOR3 lightToCamera = Camera::GetEyePos() - vLightEye;
    float cameraDistance = D3DXVec3Length(&lightToCamera);
    cameraDistance = (std::max)(cameraDistance, 0.1f);

    fLightNear[cascadeIndex] = (std::max)(0.1f, cameraDistance * 0.5f);
    fLightFar[cascadeIndex] = (std::max)(fLightNear[cascadeIndex] + 0.1f, cameraDistance * 2.0f);

    const float viewHeight = viewWidth;
    D3DXMatrixOrthoLH(&mLightProj[cascadeIndex],
                      viewWidth,
                      viewHeight,
                      fLightNear[cascadeIndex],
                      fLightFar[cascadeIndex]);

    hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDepthFromLight");
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightView", &mLightView[cascadeIndex]);
    assert(hr == S_OK);

    D3DXMATRIX mLightViewProj = mLightView[cascadeIndex] * mLightProj[cascadeIndex];
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewProj", &mLightViewProj);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat ("g_lightNear", fLightNear[cascadeIndex]);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat ("g_lightFar", fLightFar[cascadeIndex]);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_meshAlphaClipThreshold", 0.5f);
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

        mWorldViewProjLight = mWorld * mLightView[cascadeIndex] * mLightProj[cascadeIndex];
        
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
            BOOL useAlphaCutout = FALSE;
            if (mesh.IsAlphaCutoutSubset(subsetIndex))
            {
                useAlphaCutout = TRUE;
            }
            hr = g_fxDepthBufferShadow->SetBool("g_useMeshAlphaCutout", useAlphaCutout);
            assert(hr == S_OK);

            LPDIRECT3DBASETEXTURE9 alphaTexture = nullptr;
            if (useAlphaCutout)
            {
                alphaTexture = mesh.GetSubsetTextureForShadow(subsetIndex);
            }

            hr = g_fxDepthBufferShadow->SetTexture("g_texMeshAlpha", alphaTexture);
            assert(hr == S_OK);

            hr = g_fxDepthBufferShadow->CommitChanges();
            assert(hr == S_OK);

            d3dMesh->DrawSubset(subsetIndex);
        }
    }

    g_fxDepthBufferShadow->EndPass();
    g_fxDepthBufferShadow->End();

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDepthFromLight");
    assert(hr == S_OK);

    for (auto& mesh : *m_pMeshMixAnimNoBoneList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(g_fxDepthBufferShadow, mLightViewProj);
        }
    }

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

void PostEffectZShadow::RenderTechnique2(const int cascadeIndex)
{
    HRESULT hr = E_FAIL;
    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture(cascadeIndex);
    LPDIRECT3DTEXTURE9 activeShadowTexture = GetActiveShadowTexture(cascadeIndex);
    LPDIRECT3DSURFACE9 activeShadowDepthStencil = GetActiveShadowDepthStencil(cascadeIndex);

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
    D3DXVECTOR3 vEye(Camera::GetEyePos());
    D3DXVECTOR3 vAt(Camera::GetLookAtPos());
    D3DXVECTOR3 vUp(0, 1, 0);

    D3DXMatrixLookAtLH(&mView, &vEye, &vAt, &vUp);

    D3DXMATRIX mProj = Camera::GetProjMatrix();

    D3DXMATRIX mLightViewProj = mLightView[cascadeIndex] * mLightProj[cascadeIndex];

    hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewProj", &mLightViewProj);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightView", &mLightView[cascadeIndex]);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_lightNear", fLightNear[cascadeIndex]);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetFloat("g_lightFar", fLightFar[cascadeIndex]);
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

    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBias", m_shadowBias);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetInt("g_shadowPcfTapCount", m_pcfTapCount);
    assert(hr == S_OK);

    BOOL writeNearCascade = FALSE;
    if (cascadeIndex == SHADOW_CASCADE_NEAR)
    {
        writeNearCascade = TRUE;
    }
    hr = g_fxDepthBufferShadow->SetBool("g_writeNearCascade", writeNearCascade);
    assert(hr == S_OK);

    if (m_pMeshInstancingMap != nullptr)
    {
        DWORD oldColorWriteEnable = 0;
        hr = Common::D3DDevice()->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);
        assert(hr == S_OK);

        hr = Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
        assert(hr == S_OK);

        const float alphaClipThreshold = 0.5f;
        for (const auto& meshEntry : *m_pMeshInstancingMap)
        {
            if (meshEntry.second != nullptr)
            {
                meshEntry.second->RenderToShadowOccluderEffect(g_fxDepthBufferShadow,
                                                               "TechniqueShadowOccluderInstancing",
                                                               alphaClipThreshold);
            }
        }

        hr = Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, oldColorWriteEnable);
        assert(hr == S_OK);
    }

    hr = g_fxDepthBufferShadow->SetTechnique(GetWriteShadowTechniqueName());
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
            BOOL useAlphaCutout = FALSE;
            if (mesh.IsAlphaCutoutSubset(subsetIndex))
            {
                useAlphaCutout = TRUE;
            }
            hr = g_fxDepthBufferShadow->SetBool("g_useMeshAlphaCutout", useAlphaCutout);
            assert(hr == S_OK);

            LPDIRECT3DBASETEXTURE9 alphaTexture = nullptr;
            if (useAlphaCutout)
            {
                alphaTexture = mesh.GetSubsetTextureForShadow(subsetIndex);
            }

            hr = g_fxDepthBufferShadow->SetTexture("g_texMeshAlpha", alphaTexture);
            assert(hr == S_OK);

            hr = g_fxDepthBufferShadow->CommitChanges();
            assert(hr == S_OK);

            hr = mesh.GetD3DMesh()->DrawSubset(subsetIndex);
            assert(hr == S_OK);
        }
    }

    hr = g_fxDepthBufferShadow->EndPass();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->End();
    assert(hr == S_OK);

    D3DXMATRIX mViewProj = mView * mProj;

    // MeshMixAnimNoBone participates in the light-depth pass as a caster.
    // Do not draw it into the receiver shadow mask here, because the current
    // Z-shadow pass cannot distinguish same-object self projection.

    hr = g_fxDepthBufferShadow->SetTechnique(GetWriteShadowSkinTechniqueName());
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetMatrix("g_matWorldViewProj", &mViewProj);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetBool("g_useMeshAlphaCutout", FALSE);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTexture("g_texMeshAlpha", nullptr);
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

    g_fxDepthBufferShadow->SetTechnique(GetCompositeTechniqueName());
    UINT nPassNum = 0;
    g_fxDepthBufferShadow->Begin(&nPassNum, 0);
    g_fxDepthBufferShadow->BeginPass(0);

    LPDIRECT3DTEXTURE9 activeShadowTexture = GetActiveShadowTexture(SHADOW_CASCADE_NEAR);
    LPDIRECT3DTEXTURE9 activeShadowTextureFar = GetActiveShadowTexture(SHADOW_CASCADE_FAR);
    D3DSURFACE_DESC descShadow{};
    D3DSURFACE_DESC descShadowFar{};
    activeShadowTexture->GetLevelDesc(0, &descShadow);
    activeShadowTextureFar->GetLevelDesc(0, &descShadowFar);
    g_fxDepthBufferShadow->SetFloat("g_compositeTexelW", 1.0f / static_cast<float>(descShadow.Width));
    g_fxDepthBufferShadow->SetFloat("g_compositeTexelH", 1.0f / static_cast<float>(descShadow.Height));
    g_fxDepthBufferShadow->SetFloat("g_compositeFarTexelW", 1.0f / static_cast<float>(descShadowFar.Width));
    g_fxDepthBufferShadow->SetFloat("g_compositeFarTexelH", 1.0f / static_cast<float>(descShadowFar.Height));
    g_fxDepthBufferShadow->SetTexture("g_texBase",   g_texTemp);               // 元のカラー
    g_fxDepthBufferShadow->SetTexture("g_texShadow", activeShadowTexture);     // 影アルファ
    g_fxDepthBufferShadow->SetTexture("g_texShadowFar", activeShadowTextureFar);
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
    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture(SHADOW_CASCADE_NEAR);
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

void PostEffectZShadow::SetCoverageFar(const float coverage)
{
    m_coverageFar = ClampZeroToOne(coverage);
}

void PostEffectZShadow::SetShadowBias(const float shadowBias)
{
    m_shadowBias = (std::max)(0.0f, shadowBias);
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
    m_shadowTextureScaleDivisor = NormalizeShadowTextureScaleDivisor(scaleDivisor);
}

void PostEffectZShadow::OnDeviceLost()
{
    if (!m_isInitialized || g_fxDepthBufferShadow == NULL)
    {
        return;
    }

    g_fxDepthBufferShadow->OnLostDevice();
    SAFE_RELEASE(g_texTemp);
    for (int cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_COUNT; ++cascadeIndex)
    {
        for (int variantIndex = 0; variantIndex < SHADOW_TEX_SIZE_VARIANT_COUNT; ++variantIndex)
        {
            SAFE_RELEASE(g_texRenderTargetLightZ[cascadeIndex][variantIndex]);
            SAFE_RELEASE(g_texRenderTargetShadow[cascadeIndex][variantIndex]);
            SAFE_RELEASE(g_surfaceLightZStensil[cascadeIndex][variantIndex]);
            SAFE_RELEASE(g_surfaceShadowStensil[cascadeIndex][variantIndex]);
        }
    }
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
    HRESULT hr = E_FAIL;

    for (int cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_COUNT; ++cascadeIndex)
    {
        for (int variantIndex = 0; variantIndex < SHADOW_TEX_SIZE_VARIANT_COUNT; ++variantIndex)
        {
            const int scaleDivisor = VariantIndexToShadowTextureScaleDivisor(variantIndex);
            UINT lightZWidth = ComputeLightZTextureSize(Common::ScreenW(), scaleDivisor);
            UINT lightZHeight = ComputeLightZTextureSize(Common::ScreenH(), scaleDivisor);
            UINT shadowWidth = ComputeShadowTextureSize(Common::ScreenW(), scaleDivisor);
            UINT shadowHeight = ComputeShadowTextureSize(Common::ScreenH(), scaleDivisor);
            if (cascadeIndex == SHADOW_CASCADE_FAR)
            {
                lightZWidth = (std::max)(1U, lightZWidth / SHADOW_FAR_RESOLUTION_DIVISOR);
                lightZHeight = (std::max)(1U, lightZHeight / SHADOW_FAR_RESOLUTION_DIVISOR);
                shadowWidth = (std::max)(1U, shadowWidth / SHADOW_FAR_RESOLUTION_DIVISOR);
                shadowHeight = (std::max)(1U, shadowHeight / SHADOW_FAR_RESOLUTION_DIVISOR);
            }

            hResult = D3DXCreateTexture(Common::D3DDevice(),
                                    lightZWidth,
                                    lightZHeight,
                                    1,
                                    D3DUSAGE_RENDERTARGET,
                                    D3DFMT_R32F,
                                    D3DPOOL_DEFAULT,
                                    &g_texRenderTargetLightZ[cascadeIndex][variantIndex]);
            assert(hResult == S_OK);

            hr = Common::D3DDevice()->CreateDepthStencilSurface(lightZWidth,
                                                            lightZHeight,
                                                            D3DFMT_D16,
                                                            D3DMULTISAMPLE_NONE,
                                                            0,
                                                            TRUE,
                                                            &g_surfaceLightZStensil[cascadeIndex][variantIndex],
                                                            NULL);
            assert(hr == S_OK);

            hResult = D3DXCreateTexture(Common::D3DDevice(),
                                    shadowWidth,
                                    shadowHeight,
                                    1,
                                    D3DUSAGE_RENDERTARGET,
                                    D3DFMT_A16B16G16R16F,
                                    D3DPOOL_DEFAULT,
                                    &g_texRenderTargetShadow[cascadeIndex][variantIndex]);
            assert(hResult == S_OK);

            hr = Common::D3DDevice()->CreateDepthStencilSurface(shadowWidth,
                                                            shadowHeight,
                                                            D3DFMT_D16,
                                                            D3DMULTISAMPLE_NONE,
                                                            0,
                                                            TRUE,
                                                            &g_surfaceShadowStensil[cascadeIndex][variantIndex],
                                                            NULL);
            assert(hr == S_OK);
        }
    }

    D3DVERTEXELEMENT9 elems[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    hResult = Common::D3DDevice()->CreateVertexDeclaration(elems, &g_pQuadDecl);
    assert(hResult == S_OK);
}

LPDIRECT3DTEXTURE9 PostEffectZShadow::GetActiveLightZTexture(const int cascadeIndex) const
{
    return g_texRenderTargetLightZ[cascadeIndex][GetActiveShadowTexVariantIndex()];
}

LPDIRECT3DSURFACE9 PostEffectZShadow::GetActiveLightZDepthStencil(const int cascadeIndex) const
{
    return g_surfaceLightZStensil[cascadeIndex][GetActiveShadowTexVariantIndex()];
}

LPDIRECT3DTEXTURE9 PostEffectZShadow::GetActiveShadowTexture(const int cascadeIndex) const
{
    return g_texRenderTargetShadow[cascadeIndex][GetActiveShadowTexVariantIndex()];
}

LPDIRECT3DSURFACE9 PostEffectZShadow::GetActiveShadowDepthStencil(const int cascadeIndex) const
{
    return g_surfaceShadowStensil[cascadeIndex][GetActiveShadowTexVariantIndex()];
}

int PostEffectZShadow::GetActiveShadowTexVariantIndex() const
{
    return ShadowTextureScaleDivisorToVariantIndex(m_shadowTextureScaleDivisor);
}

const char* PostEffectZShadow::GetWriteShadowTechniqueName() const
{
    if (m_pcfTapCount == 1)
    {
        return "TechniqueWriteShadow1";
    }

    if (m_pcfTapCount == 3)
    {
        return "TechniqueWriteShadow3";
    }

    if (m_pcfTapCount == 5)
    {
        return "TechniqueWriteShadow5";
    }

    if (m_pcfTapCount == 7)
    {
        return "TechniqueWriteShadow7";
    }

    if (m_pcfTapCount == 9)
    {
        return "TechniqueWriteShadow9";
    }

    return "TechniqueWriteShadow11";
}

const char* PostEffectZShadow::GetWriteShadowSkinTechniqueName() const
{
    if (m_pcfTapCount == 1)
    {
        return "TechniqueWriteShadowSkin1";
    }

    if (m_pcfTapCount == 3)
    {
        return "TechniqueWriteShadowSkin3";
    }

    if (m_pcfTapCount == 5)
    {
        return "TechniqueWriteShadowSkin5";
    }

    if (m_pcfTapCount == 7)
    {
        return "TechniqueWriteShadowSkin7";
    }

    if (m_pcfTapCount == 9)
    {
        return "TechniqueWriteShadowSkin9";
    }

    return "TechniqueWriteShadowSkin11";
}

const char* PostEffectZShadow::GetCompositeTechniqueName() const
{
    if (m_compositeTapCount == 1)
    {
        return "TechniqueComposite1";
    }

    if (m_compositeTapCount == 3)
    {
        return "TechniqueComposite3";
    }

    if (m_compositeTapCount == 5)
    {
        return "TechniqueComposite5";
    }

    if (m_compositeTapCount == 7)
    {
        return "TechniqueComposite7";
    }

    if (m_compositeTapCount == 9)
    {
        return "TechniqueComposite9";
    }

    return "TechniqueComposite11";
}

}

