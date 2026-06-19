#pragma once

#include <deque>
#include <unordered_map>
#include <vector>
#include "Common.h"
#include "MeshMixManager.h"

namespace NSRender
{
class MeshInstancing;
class MeshMixSkinAnim;

class PostEffectZShadow : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget,
              LPDIRECT3DTEXTURE9 sceneDepthTexture,
              LPDIRECT3DTEXTURE9 sceneNormalTexture,
              const std::deque<MeshMixManager>& meshMixList,
              const std::vector<MeshMixSkinAnim*>& meshMixSkinAnimList,
              const std::unordered_map<std::wstring, MeshInstancing*>& meshInstancingMap);

    void SetShadowIntensity(const float intensity);
    void SetShadowSaturationBoost(const float saturationBoost);
    void SetCoverage(const float coverage);
    void SetShadowBias(const float shadowBias);
    void SetPcfTapCount(const int tapCount);
    void SetCompositeTapCount(const int tapCount);
    void SetShadowTextureScaleDivisor(const int scaleDivisor);
    void DrawDebugLightDepthOverlay(const int x,
                                    const int y,
                                    const int width,
                                    const int height);

    void OnDeviceLost();
    void OnDeviceReset();


private:
    static const int SHADOW_TEX_SIZE_VARIANT_COUNT = 5;

    void RenderTechnique1();
    void RenderTechnique2();
    void RenderTechnique3();


    float m_shadowIntensity = 0.5f;
    float m_shadowSaturationBoost = 0.35f;
    float m_coverage = 0.5f;
    float m_shadowBias = 0.0121f;
    int m_pcfTapCount = 11;
    int m_compositeTapCount = 11;
    int m_shadowTextureScaleDivisor = 1;

    LPD3DXEFFECT g_fxDepthBufferShadow = NULL;

    LPDIRECT3DTEXTURE9 g_texTemp = NULL;

    LPDIRECT3DTEXTURE9 g_texRenderTargetLightZ[SHADOW_TEX_SIZE_VARIANT_COUNT] { };
    LPDIRECT3DTEXTURE9 g_texRenderTargetShadow[SHADOW_TEX_SIZE_VARIANT_COUNT] { };
    
    LPDIRECT3DSURFACE9 g_surfaceLightZStensil[SHADOW_TEX_SIZE_VARIANT_COUNT] { };
    LPDIRECT3DSURFACE9 g_surfaceShadowStensil[SHADOW_TEX_SIZE_VARIANT_COUNT] { };
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


    D3DXMATRIX mLightView;
    D3DXMATRIX mLightProj;

    float fLightNear = 10.0f;
    float fLightFar = 200.0f;

    const std::deque<MeshMixManager>* m_pMeshList;
    const std::vector<MeshMixSkinAnim*>* m_pSkinAnimMeshList = nullptr;
    const std::unordered_map<std::wstring, MeshInstancing*>* m_pMeshInstancingMap = nullptr;
    LPDIRECT3DTEXTURE9 m_sceneDepthTexture = NULL;
    LPDIRECT3DTEXTURE9 m_sceneNormalTexture = NULL;
    LPDIRECT3DTEXTURE9 m_texCompositeTarget = NULL;

    void CreateRawResource();
    LPDIRECT3DTEXTURE9 GetActiveLightZTexture() const;
    LPDIRECT3DSURFACE9 GetActiveLightZDepthStencil() const;
    LPDIRECT3DTEXTURE9 GetActiveShadowTexture() const;
    LPDIRECT3DSURFACE9 GetActiveShadowDepthStencil() const;
    int GetActiveShadowTexVariantIndex() const;
    const char* GetWriteShadowTechniqueName() const;
    const char* GetWriteShadowSkinTechniqueName() const;
    const char* GetCompositeTechniqueName() const;
};

}

