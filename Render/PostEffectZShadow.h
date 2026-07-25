#pragma once

#include <unordered_map>
#include <vector>
#include "Common.h"

namespace NSRender
{
class MeshInstancing2;
class MeshMixAnimNoBone2;
class MeshMix2;
class IMeshMixSkinAnim;

class PostEffectZShadow : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget,
              LPDIRECT3DTEXTURE9 sceneDepthTexture,
              LPDIRECT3DTEXTURE9 receiverDepthTexture,
              LPDIRECT3DTEXTURE9 sceneNormalTexture,
              float sceneDepthNear,
              float sceneDepthFar,
              const std::vector<IMeshMixSkinAnim*>& meshMixSkinAnimList,
              const std::vector<MeshMixAnimNoBone2*>& meshMixAnimNoBone2List,
              const std::vector<MeshMix2*>& meshMix2List,
              const std::unordered_map<std::wstring, MeshInstancing2*>& meshInstancing2Map);

    void SetShadowIntensity(const float intensity);
    void SetShadowSaturationBoost(const float saturationBoost);
    void SetCoverage(const float coverage);
    void SetCoverageFar(const float coverage);
    void SetShadowBias(const float shadowBias);
    void SetShadowBiasFar(const float shadowBias);
    void SetPcfTapCount(const int tapCount);
    void SetCompositeTapCount(const int tapCount);
    void SetShadowTextureScaleDivisor(const int scaleDivisor);
    void SetFarCascadeEnabled(bool enabled);
    void DrawDebugLightDepthOverlay(const int x,
                                    const int y,
                                    const int width,
                                    const int height,
                                    const int cascadeIndex);

    void OnDeviceLost();
    void OnDeviceReset();


private:
    static const int SHADOW_TEX_SIZE_VARIANT_COUNT = 5;
    static const int SHADOW_CASCADE_COUNT = 2;
    static const int SHADOW_CASCADE_NEAR = 0;
    static const int SHADOW_CASCADE_FAR = 1;

    void RenderTechnique1(const int cascadeIndex);
    void RenderTechnique2(const int cascadeIndex);
    void RenderTechnique2FromGBuffer(const int cascadeIndex);
    void RenderTechnique3();
    void RenderTechnique3Direct();


    float m_shadowIntensity = 0.5f;
    float m_shadowSaturationBoost = 0.35f;
    float m_coverage = 0.3f;
    float m_coverageFar = 0.8f;
    float m_shadowBias = 0.0004f;
    float m_shadowBiasFar = 0.01924f;
    int m_pcfTapCount = 1;
    int m_compositeTapCount = 1;
    int m_shadowTextureScaleDivisor = 1;
    bool m_farCascadeEnabled = false;

    LPD3DXEFFECT g_fxDepthBufferShadow = NULL;

    LPDIRECT3DTEXTURE9 g_texTemp = NULL;

    LPDIRECT3DTEXTURE9 g_texRenderTargetLightZ[SHADOW_CASCADE_COUNT][SHADOW_TEX_SIZE_VARIANT_COUNT] { };
    LPDIRECT3DTEXTURE9 g_texRenderTargetShadow[SHADOW_CASCADE_COUNT][SHADOW_TEX_SIZE_VARIANT_COUNT] { };
    
    LPDIRECT3DSURFACE9 g_surfaceLightZStensil[SHADOW_CASCADE_COUNT][SHADOW_TEX_SIZE_VARIANT_COUNT] { };
    LPDIRECT3DSURFACE9 g_surfaceShadowStensil[SHADOW_CASCADE_COUNT][SHADOW_TEX_SIZE_VARIANT_COUNT] { };
    LPDIRECT3DSURFACE9 oldRT0 = NULL;
    LPDIRECT3DSURFACE9 oldZ = NULL;
    
    LPDIRECT3DVERTEXDECLARATION9 g_pQuadDecl = NULL;


    void DrawFullscreenQuad();
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    struct QuadVertex
    {
        // クリップ空間（-1..1, w=1）
        float x, y, z, w;
    
        // テクスチャ座標
        float u, v;
    };


    D3DXMATRIX mLightView[SHADOW_CASCADE_COUNT];
    D3DXMATRIX mLightProj[SHADOW_CASCADE_COUNT];

    float fLightNear[SHADOW_CASCADE_COUNT] { 10.0f, 10.0f };
    float fLightFar[SHADOW_CASCADE_COUNT] { 200.0f, 200.0f };

    const std::vector<IMeshMixSkinAnim*>* m_pSkinAnimMeshList = nullptr;
    const std::vector<MeshMixAnimNoBone2*>* m_pMeshMixAnimNoBone2List = nullptr;
    const std::vector<MeshMix2*>* m_pMeshMix2List = nullptr;
    const std::unordered_map<std::wstring, MeshInstancing2*>* m_pMeshInstancing2Map = nullptr;
    LPDIRECT3DTEXTURE9 m_sceneDepthTexture = NULL;
    LPDIRECT3DTEXTURE9 m_receiverDepthTexture = NULL;
    LPDIRECT3DTEXTURE9 m_sceneNormalTexture = NULL;
    LPDIRECT3DTEXTURE9 m_texCompositeTarget = NULL;
    float m_sceneDepthNear = 0.1f;
    float m_sceneDepthFar = 30'000.0f;

    void CreateRawResource();
    LPDIRECT3DTEXTURE9 GetActiveLightZTexture(const int cascadeIndex) const;
    LPDIRECT3DSURFACE9 GetActiveLightZDepthStencil(const int cascadeIndex) const;
    LPDIRECT3DTEXTURE9 GetActiveShadowTexture(const int cascadeIndex) const;
    LPDIRECT3DSURFACE9 GetActiveShadowDepthStencil(const int cascadeIndex) const;
    int GetActiveShadowTexVariantIndex() const;
    const char* GetBuildShadowFromGBufferTechniqueName() const;
    const char* GetWriteShadowTechniqueName() const;
    const char* GetWriteShadowSkinTechniqueName() const;
    const char* GetCompositeTechniqueName() const;
};

}


