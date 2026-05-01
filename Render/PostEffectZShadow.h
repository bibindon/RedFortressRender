#pragma once

#include <deque>
#include <vector>
#include "Common.h"
#include "MeshMixManager.h"

namespace NSRender
{
class MeshMixSkinAnim;

class PostEffectZShadow : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 sceneDepthTexture,
                            LPDIRECT3DTEXTURE9 sceneNormalTexture,
                            const std::deque<MeshMixManager>& meshMixList,
                            const std::vector<MeshMixSkinAnim*>& meshMixSkinAnimList);

    void SetEnable(const bool arg);
    void SetShadowIntensity(const float intensity);
    void SetShadowSaturationBoost(const float saturationBoost);

    void OnDeviceLost();
    void OnDeviceReset();


private:

    void RenderTechnique1();
    void RenderTechnique2();
    void RenderTechnique3();


    bool m_bEnable = true;
    float m_shadowIntensity = 0.5f;
    float m_shadowSaturationBoost = 0.35f;

    LPD3DXEFFECT g_fxDepthBufferShadow = NULL;

    LPDIRECT3DTEXTURE9 g_texTemp = NULL;

    LPDIRECT3DTEXTURE9 g_texRenderTargetLightZ = NULL;
    LPDIRECT3DTEXTURE9 g_texRenderTargetShadow = NULL;
    LPDIRECT3DTEXTURE9 g_texComposite = NULL;
    
    LPDIRECT3DSURFACE9 g_surfaceLightZStensil = NULL;
    LPDIRECT3DSURFACE9 oldRT0 = NULL;
    LPDIRECT3DSURFACE9 oldZ = NULL;
    
    LPDIRECT3DVERTEXDECLARATION9 g_pQuadDecl = NULL;


    void DrawFullscreenQuad();

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
    LPDIRECT3DTEXTURE9 m_sceneDepthTexture = NULL;
    LPDIRECT3DTEXTURE9 m_sceneNormalTexture = NULL;

    void CreateRawResource();
};

}

