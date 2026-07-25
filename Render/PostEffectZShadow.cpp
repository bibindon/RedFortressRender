#include "PostEffectZShadow.h"

#include <stdexcept>

#include "Camera.h"
#include "MeshInstancing2.h"
#include "MeshMixAnimNoBone2.h"
#include "MeshMix2.h"
#include "MeshMixSkinAnimCommon.h"

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
constexpr int SHADOW_REFERENCE_SCREEN_WIDTH = 1600;
constexpr int SHADOW_REFERENCE_SCREEN_HEIGHT = 900;
constexpr int SHADOW_REFERENCE_LIGHT_Z_SIZE = 2048;
constexpr int SHADOW_LIGHT_Z_SIZE_MIN = 512;
constexpr int SHADOW_LIGHT_Z_SIZE_MAX = 2048;
constexpr int SHADOW_LIGHT_Z_SIZE_ALIGNMENT = 64;
const D3DXVECTOR3 SHADOW_CAMERA_OFFSET(40.0f, 50.0f, -40.0f);

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

int ShadowTapCountToIndex(const int tapCount)
{
    switch (tapCount)
    {
    case 3:
        return 1;
    case 5:
        return 2;
    case 7:
        return 3;
    case 9:
        return 4;
    case 11:
        return 5;
    default:
        return 0;
    }
}

const char* GetFixedDirectCompositeTechniqueName(const int pcfTapCount,
                                                 const int compositeTapCount)
{
    static const char* techniqueNames[6][6] =
    {
        { "TechniqueDirectP1C1", "TechniqueDirectP1C3", "TechniqueDirectP1C5",
          "TechniqueDirectP1C7", "TechniqueDirectP1C9", "TechniqueDirectP1C11" },
        { "TechniqueDirectP3C1", "TechniqueDirectP3C3", "TechniqueDirectP3C5",
          "TechniqueDirectP3C7", "TechniqueDirectP3C9", "TechniqueDirectP3C11" },
        { "TechniqueDirectP5C1", "TechniqueDirectP5C3", "TechniqueDirectP5C5",
          "TechniqueDirectP5C7", "TechniqueDirectP5C9", "TechniqueDirectP5C11" },
        { "TechniqueDirectP7C1", "TechniqueDirectP7C3", "TechniqueDirectP7C5",
          "TechniqueDirectP7C7", "TechniqueDirectP7C9", "TechniqueDirectP7C11" },
        { "TechniqueDirectP9C1", "TechniqueDirectP9C3", "TechniqueDirectP9C5",
          "TechniqueDirectP9C7", "TechniqueDirectP9C9", "TechniqueDirectP9C11" },
        { "TechniqueDirectP11C1", "TechniqueDirectP11C3", "TechniqueDirectP11C5",
          "TechniqueDirectP11C7", "TechniqueDirectP11C9", "TechniqueDirectP11C11" },
    };
    const int pcfIndex = ShadowTapCountToIndex(pcfTapCount);
    const int compositeIndex = ShadowTapCountToIndex(compositeTapCount);
    return techniqueNames[pcfIndex][compositeIndex];
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

UINT ComputeLightZTextureSize(const int screenWidth,
                              const int screenHeight,
                              const int scaleDivisor)
{
    const float widthScale = static_cast<float>(screenWidth) /
        static_cast<float>(SHADOW_REFERENCE_SCREEN_WIDTH);
    const float heightScale = static_cast<float>(screenHeight) /
        static_cast<float>(SHADOW_REFERENCE_SCREEN_HEIGHT);
    const float renderScale = (std::min)(widthScale, heightScale);
    const int scaledSize = static_cast<int>(
        static_cast<float>(SHADOW_REFERENCE_LIGHT_Z_SIZE) * renderScale + 0.5f);
    const int alignedSize = ((scaledSize + SHADOW_LIGHT_Z_SIZE_ALIGNMENT / 2) /
        SHADOW_LIGHT_Z_SIZE_ALIGNMENT) * SHADOW_LIGHT_Z_SIZE_ALIGNMENT;
    const int clampedSize = (std::max)(SHADOW_LIGHT_Z_SIZE_MIN,
        (std::min)(alignedSize, SHADOW_LIGHT_Z_SIZE_MAX));
    const int normalizedDivisor = NormalizeShadowTextureScaleDivisor(scaleDivisor);
    return static_cast<UINT>((std::max)(1, clampedSize / normalizedDivisor));
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
                             LPDIRECT3DTEXTURE9 receiverDepthTexture,
                             LPDIRECT3DTEXTURE9 sceneNormalTexture,
                             const float sceneDepthNear,
                             const float sceneDepthFar,
                             const std::vector<IMeshMixSkinAnim*>& meshMixSkinAnimList,
                              const std::vector<MeshMixAnimNoBone2*>& meshMixAnimNoBone2List,
                              const std::vector<MeshMix2*>& meshMix2List,
                              const std::unordered_map<std::wstring, MeshInstancing2*>& meshInstancing2Map)
{
    g_texTemp = renderTarget;
    m_texCompositeTarget = texTarget;
    m_pSkinAnimMeshList = &meshMixSkinAnimList;
    m_pMeshMixAnimNoBone2List = &meshMixAnimNoBone2List;
    m_pMeshMix2List = &meshMix2List;
    m_pMeshInstancing2Map = &meshInstancing2Map;
    m_sceneDepthTexture = sceneDepthTexture;
    m_receiverDepthTexture = receiverDepthTexture;
    m_sceneNormalTexture = sceneNormalTexture;
    m_sceneDepthNear = sceneDepthNear;
    m_sceneDepthFar = sceneDepthFar;

    RenderTechnique1(SHADOW_CASCADE_NEAR);
    if (m_farCascadeEnabled)
    {
        RenderTechnique1(SHADOW_CASCADE_FAR);
    }
    RenderTechnique3Direct();

    m_pSkinAnimMeshList = nullptr;
    m_pMeshMixAnimNoBone2List = nullptr;
    m_pMeshMix2List = nullptr;
    m_pMeshInstancing2Map = nullptr;
    m_sceneDepthTexture = NULL;
    m_receiverDepthTexture = NULL;
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

    g_fxDepthBufferShadow->EndPass();
    g_fxDepthBufferShadow->End();

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDepthFromLight");
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetBool("g_useMeshAlphaCutout", FALSE);
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTexture("g_texMeshAlpha", nullptr);
    assert(hr == S_OK);

    for (auto& mesh : *m_pMeshMix2List)
    {
        if (mesh != nullptr && mesh->IsDepthBufferShadowEnabled())
        {
            mesh->RenderToEffect(g_fxDepthBufferShadow, mLightViewProj);
        }
    }

    hr = g_fxDepthBufferShadow->SetTechnique("TechniqueDepthFromLightSkin");
    assert(hr == S_OK);

    for (auto& mesh : *m_pMeshMixAnimNoBone2List)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(g_fxDepthBufferShadow, mLightViewProj);
        }
    }

    for (auto& mesh : *m_pSkinAnimMeshList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(g_fxDepthBufferShadow);
        }
    }

    hr = Common::D3DDevice()->EndScene();
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetRenderTarget(0, oldRT0);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetDepthStencilSurface(oldZ);
    assert(hr == S_OK);

    SAFE_RELEASE(surfaceLightZ);
    SAFE_RELEASE(oldRT0);
    SAFE_RELEASE(oldZ);

    Common::D3DDevice()->SetViewport(&oldViewPort);
}

void PostEffectZShadow::RenderTechnique2FromGBuffer(const int cascadeIndex)
{
    HRESULT hr = E_FAIL;
    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture(cascadeIndex);
    LPDIRECT3DTEXTURE9 activeShadowTexture = GetActiveShadowTexture(cascadeIndex);

    LPDIRECT3DSURFACE9 surfaceShadow = NULL;
    hr = activeShadowTexture->GetSurfaceLevel(0, &surfaceShadow);
    assert(hr == S_OK);

    D3DVIEWPORT9 oldViewport{};
    hr = Common::D3DDevice()->GetViewport(&oldViewport);
    assert(hr == S_OK);

    D3DSURFACE_DESC shadowDescription{};
    hr = activeShadowTexture->GetLevelDesc(0, &shadowDescription);
    assert(hr == S_OK);

    D3DSURFACE_DESC lightDepthDescription{};
    hr = activeLightZTexture->GetLevelDesc(0, &lightDepthDescription);
    assert(hr == S_OK);

    D3DVIEWPORT9 shadowViewport{};
    shadowViewport.X = 0;
    shadowViewport.Y = 0;
    shadowViewport.Width = shadowDescription.Width;
    shadowViewport.Height = shadowDescription.Height;
    shadowViewport.MinZ = 0.0f;
    shadowViewport.MaxZ = 1.0f;

    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceShadow);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetDepthStencilSurface(NULL);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetViewport(&shadowViewport);
    assert(hr == S_OK);

    const D3DXMATRIX viewMatrix = Camera::GetViewMatrix();
    const D3DXMATRIX projectionMatrix = Camera::GetProjMatrix();
    D3DXMATRIX inverseViewMatrix;
    D3DXMATRIX inverseProjectionMatrix;
    if (D3DXMatrixInverse(&inverseViewMatrix, NULL, &viewMatrix) == NULL ||
        D3DXMatrixInverse(&inverseProjectionMatrix, NULL, &projectionMatrix) == NULL)
    {
        SAFE_RELEASE(surfaceShadow);
        throw std::runtime_error("Failed to invert the camera matrix for Z shadow reconstruction.");
    }

    D3DXMATRIX lightViewProjectionMatrix = mLightView[cascadeIndex] * mLightProj[cascadeIndex];

    hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);

    hr = g_fxDepthBufferShadow->SetTechnique(GetBuildShadowFromGBufferTechniqueName());
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matInverseView", &inverseViewMatrix);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matInverseProjection", &inverseProjectionMatrix);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightView", &mLightView[cascadeIndex]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewProj", &lightViewProjectionMatrix);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_lightNear", fLightNear[cascadeIndex]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_lightFar", fLightFar[cascadeIndex]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverDepthNear", Camera::GetNear());
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverDepthFar", Camera::GetFar());
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_sceneDepthNear", m_sceneDepthNear);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_sceneDepthFar", m_sceneDepthFar);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverTexelW", 1.0f / static_cast<float>(shadowDescription.Width));
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverTexelH", 1.0f / static_cast<float>(shadowDescription.Height));
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelW", 1.0f / static_cast<float>(lightDepthDescription.Width));
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelH", 1.0f / static_cast<float>(lightDepthDescription.Height));
    assert(hr == S_OK);
    float activeShadowBias = m_shadowBias;
    if (cascadeIndex == SHADOW_CASCADE_FAR)
    {
        activeShadowBias = m_shadowBiasFar;
    }
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBias", activeShadowBias);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowIntensity", m_shadowIntensity);
    assert(hr == S_OK);
    BOOL writeNearCascade = FALSE;
    if (cascadeIndex == SHADOW_CASCADE_NEAR)
    {
        writeNearCascade = TRUE;
    }
    hr = g_fxDepthBufferShadow->SetBool("g_writeNearCascade", writeNearCascade);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texLightZ", activeLightZTexture);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texReceiverDepth", m_receiverDepthTexture);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texSceneDepth", m_sceneDepthTexture);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texSceneNormal", m_sceneNormalTexture);
    assert(hr == S_OK);

    UINT passCount = 0;
    hr = g_fxDepthBufferShadow->Begin(&passCount, 0);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->BeginPass(0);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->CommitChanges();
    assert(hr == S_OK);

    DrawFullscreenQuad();

    hr = g_fxDepthBufferShadow->EndPass();
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->End();
    assert(hr == S_OK);
    hr = Common::D3DDevice()->EndScene();
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetRenderTarget(0, oldRT0);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->SetDepthStencilSurface(oldZ);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->SetViewport(&oldViewport);
    assert(hr == S_OK);

    SAFE_RELEASE(surfaceShadow);
    SAFE_RELEASE(oldRT0);
    SAFE_RELEASE(oldZ);
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

    float activeShadowBias = m_shadowBias;
    if (cascadeIndex == SHADOW_CASCADE_FAR)
    {
        activeShadowBias = m_shadowBiasFar;
    }
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBias", activeShadowBias);
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

    D3DXMATRIX mViewProj = mView * mProj;

    if (m_pMeshInstancing2Map != nullptr)
    {
        DWORD oldColorWriteEnable = 0;
        hr = Common::D3DDevice()->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);
        assert(hr == S_OK);

        hr = Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
        assert(hr == S_OK);

        const float alphaClipThreshold = 0.5f;
        for (const auto& meshEntry : *m_pMeshInstancing2Map)
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

    if (m_pMeshMixAnimNoBone2List != nullptr)
    {
        DWORD oldColorWriteEnable = 0;
        hr = Common::D3DDevice()->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);
        assert(hr == S_OK);

        hr = Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetTechnique(GetWriteShadowSkinTechniqueName());
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetBool("g_useMeshAlphaCutout", FALSE);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetTexture("g_texMeshAlpha", nullptr);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->CommitChanges();
        assert(hr == S_OK);

        for (auto& mesh : *m_pMeshMixAnimNoBone2List)
        {
            if (mesh != nullptr)
            {
                mesh->RenderToEffect(g_fxDepthBufferShadow, mViewProj);
            }
        }

        hr = Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, oldColorWriteEnable);
        assert(hr == S_OK);
    }

    if (m_pMeshMix2List != nullptr)
    {
        DWORD oldColorWriteEnable = 0;
        hr = Common::D3DDevice()->GetRenderState(D3DRS_COLORWRITEENABLE, &oldColorWriteEnable);
        assert(hr == S_OK);

        hr = Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetTechnique(GetWriteShadowTechniqueName());
        assert(hr == S_OK);

        for (auto& mesh : *m_pMeshMix2List)
        {
            if (mesh != nullptr && mesh->IsDepthBufferShadowEnabled())
            {
                mesh->RenderToEffect(g_fxDepthBufferShadow, mViewProj);
            }
        }

        hr = Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, oldColorWriteEnable);
        assert(hr == S_OK);
    }

    if (m_pMeshMix2List != nullptr)
    {
        hr = g_fxDepthBufferShadow->SetTechnique(GetWriteShadowTechniqueName());
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetBool("g_useMeshAlphaCutout", FALSE);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->SetTexture("g_texMeshAlpha", nullptr);
        assert(hr == S_OK);

        hr = g_fxDepthBufferShadow->CommitChanges();
        assert(hr == S_OK);

        for (auto& mesh : *m_pMeshMix2List)
        {
            if (mesh != nullptr && mesh->IsDepthBufferShadowEnabled())
            {
                mesh->RenderToEffect(g_fxDepthBufferShadow, mViewProj);
            }
        }
    }

    // MeshMixAnimNoBone2 participates in the light-depth pass as a caster.
    // It already wrote depth-only above, so it occludes receiver shadows
    // behind it without drawing self-shadow color onto its own surface.

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

void PostEffectZShadow::RenderTechnique3Direct()
{
    HRESULT hr = E_FAIL;
    LPDIRECT3DSURFACE9 previousRenderTarget = NULL;
    LPDIRECT3DSURFACE9 previousDepthStencil = NULL;
    LPDIRECT3DSURFACE9 compositeSurface = NULL;
    D3DVIEWPORT9 previousViewport{};

    hr = Common::D3DDevice()->GetRenderTarget(0, &previousRenderTarget);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->GetDepthStencilSurface(&previousDepthStencil);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->GetViewport(&previousViewport);
    assert(hr == S_OK);
    hr = m_texCompositeTarget->GetSurfaceLevel(0, &compositeSurface);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->SetRenderTarget(0, compositeSurface);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->SetDepthStencilSurface(NULL);
    assert(hr == S_OK);

    D3DSURFACE_DESC compositeDescription{};
    hr = m_texCompositeTarget->GetLevelDesc(0, &compositeDescription);
    assert(hr == S_OK);
    D3DVIEWPORT9 compositeViewport{};
    compositeViewport.X = 0;
    compositeViewport.Y = 0;
    compositeViewport.Width = compositeDescription.Width;
    compositeViewport.Height = compositeDescription.Height;
    compositeViewport.MinZ = 0.0f;
    compositeViewport.MaxZ = 1.0f;
    hr = Common::D3DDevice()->SetViewport(&compositeViewport);
    assert(hr == S_OK);

    LPDIRECT3DTEXTURE9 nearLightDepthTexture = GetActiveLightZTexture(SHADOW_CASCADE_NEAR);
    LPDIRECT3DTEXTURE9 farLightDepthTexture = GetActiveLightZTexture(SHADOW_CASCADE_FAR);
    D3DSURFACE_DESC nearLightDepthDescription{};
    D3DSURFACE_DESC farLightDepthDescription{};
    hr = nearLightDepthTexture->GetLevelDesc(0, &nearLightDepthDescription);
    assert(hr == S_OK);
    hr = farLightDepthTexture->GetLevelDesc(0, &farLightDepthDescription);
    assert(hr == S_OK);

    const D3DXMATRIX viewMatrix = Camera::GetViewMatrix();
    const D3DXMATRIX projectionMatrix = Camera::GetProjMatrix();
    D3DXMATRIX inverseViewMatrix;
    D3DXMATRIX inverseProjectionMatrix;
    if (D3DXMatrixInverse(&inverseViewMatrix, NULL, &viewMatrix) == NULL ||
        D3DXMatrixInverse(&inverseProjectionMatrix, NULL, &projectionMatrix) == NULL)
    {
        SAFE_RELEASE(compositeSurface);
        SAFE_RELEASE(previousRenderTarget);
        SAFE_RELEASE(previousDepthStencil);
        throw std::runtime_error("Failed to invert the camera matrix for direct Z shadow composition.");
    }

    D3DXMATRIX nearLightViewProjection =
        mLightView[SHADOW_CASCADE_NEAR] * mLightProj[SHADOW_CASCADE_NEAR];
    D3DXMATRIX farLightViewProjection =
        mLightView[SHADOW_CASCADE_FAR] * mLightProj[SHADOW_CASCADE_FAR];

    const char* directCompositeTechnique =
        GetFixedDirectCompositeTechniqueName(m_pcfTapCount, m_compositeTapCount);
    hr = g_fxDepthBufferShadow->SetTechnique(directCompositeTechnique);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matInverseView", &inverseViewMatrix);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matInverseProjection", &inverseProjectionMatrix);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightView", &mLightView[SHADOW_CASCADE_NEAR]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewProj", &nearLightViewProjection);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewFar", &mLightView[SHADOW_CASCADE_FAR]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetMatrix("g_matLightViewProjFar", &farLightViewProjection);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_lightNear", fLightNear[SHADOW_CASCADE_NEAR]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_lightFar", fLightFar[SHADOW_CASCADE_NEAR]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_lightNearFarCascade", fLightNear[SHADOW_CASCADE_FAR]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_lightFarFarCascade", fLightFar[SHADOW_CASCADE_FAR]);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverDepthNear", Camera::GetNear());
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverDepthFar", Camera::GetFar());
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_sceneDepthNear", m_sceneDepthNear);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_sceneDepthFar", m_sceneDepthFar);
    assert(hr == S_OK);
    const float compositeTexelWidth = 1.0f / static_cast<float>(compositeDescription.Width);
    const float compositeTexelHeight = 1.0f / static_cast<float>(compositeDescription.Height);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverTexelW", compositeTexelWidth);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_receiverTexelH", compositeTexelHeight);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_compositeTexelW", compositeTexelWidth);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_compositeTexelH", compositeTexelHeight);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelW",
                                         1.0f / static_cast<float>(nearLightDepthDescription.Width));
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowTexelH",
                                         1.0f / static_cast<float>(nearLightDepthDescription.Height));
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowFarTexelW",
                                         1.0f / static_cast<float>(farLightDepthDescription.Width));
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowFarTexelH",
                                         1.0f / static_cast<float>(farLightDepthDescription.Height));
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBias", m_shadowBias);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowBiasFar", m_shadowBiasFar);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowIntensity", m_shadowIntensity);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_shadowSaturationBoost", m_shadowSaturationBoost);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_edgeDepthThreshold", 0.010f);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetFloat("g_edgeNormalThreshold", 0.50f);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetInt("g_shadowPcfTapCount", m_pcfTapCount);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetInt("g_shadowCompositeTapCount", m_compositeTapCount);
    assert(hr == S_OK);
    BOOL farCascadeEnabled = FALSE;
    if (m_farCascadeEnabled)
    {
        farCascadeEnabled = TRUE;
    }
    hr = g_fxDepthBufferShadow->SetBool("g_farCascadeEnabled", farCascadeEnabled);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texBase", g_texTemp);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texSceneDepth", m_sceneDepthTexture);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texReceiverDepth", m_receiverDepthTexture);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texSceneNormal", m_sceneNormalTexture);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texLightZ", nearLightDepthTexture);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->SetTexture("g_texLightZFar", farLightDepthTexture);
    assert(hr == S_OK);

    hr = Common::D3DDevice()->BeginScene();
    assert(hr == S_OK);
    UINT passCount = 0;
    hr = g_fxDepthBufferShadow->Begin(&passCount, 0);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->BeginPass(0);
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->CommitChanges();
    assert(hr == S_OK);

    DrawFullscreenQuad();

    hr = g_fxDepthBufferShadow->EndPass();
    assert(hr == S_OK);
    hr = g_fxDepthBufferShadow->End();
    assert(hr == S_OK);
    hr = Common::D3DDevice()->EndScene();
    assert(hr == S_OK);

    hr = Common::D3DDevice()->SetRenderTarget(0, previousRenderTarget);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->SetDepthStencilSurface(previousDepthStencil);
    assert(hr == S_OK);
    hr = Common::D3DDevice()->SetViewport(&previousViewport);
    assert(hr == S_OK);

    SAFE_RELEASE(compositeSurface);
    SAFE_RELEASE(previousRenderTarget);
    SAFE_RELEASE(previousDepthStencil);
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
    BOOL farCascadeEnabled = FALSE;
    if (m_farCascadeEnabled)
    {
        farCascadeEnabled = TRUE;
    }
    g_fxDepthBufferShadow->SetBool("g_farCascadeEnabled", farCascadeEnabled);
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
                                                   const int height,
                                                   const int cascadeIndex)
{
    if (cascadeIndex == SHADOW_CASCADE_FAR && !m_farCascadeEnabled)
    {
        return;
    }

    int activeCascadeIndex = SHADOW_CASCADE_NEAR;
    if (cascadeIndex >= 0 && cascadeIndex < SHADOW_CASCADE_COUNT)
    {
        activeCascadeIndex = cascadeIndex;
    }

    LPDIRECT3DTEXTURE9 activeLightZTexture = GetActiveLightZTexture(activeCascadeIndex);
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

void PostEffectZShadow::SetShadowBiasFar(const float shadowBias)
{
    m_shadowBiasFar = (std::max)(0.0f, shadowBias);
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

void PostEffectZShadow::SetFarCascadeEnabled(const bool enabled)
{
    m_farCascadeEnabled = enabled;
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
            const UINT lightZSize = ComputeLightZTextureSize(Common::ScreenW(),
                                                             Common::ScreenH(),
                                                             scaleDivisor);
            UINT lightZWidth = lightZSize;
            UINT lightZHeight = lightZSize;
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

const char* PostEffectZShadow::GetBuildShadowFromGBufferTechniqueName() const
{
    if (m_pcfTapCount == 1)
    {
        return "TechniqueBuildShadowFromGBuffer1";
    }

    if (m_pcfTapCount == 3)
    {
        return "TechniqueBuildShadowFromGBuffer3";
    }

    if (m_pcfTapCount == 5)
    {
        return "TechniqueBuildShadowFromGBuffer5";
    }

    if (m_pcfTapCount == 7)
    {
        return "TechniqueBuildShadowFromGBuffer7";
    }

    if (m_pcfTapCount == 9)
    {
        return "TechniqueBuildShadowFromGBuffer9";
    }

    return "TechniqueBuildShadowFromGBuffer11";
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


