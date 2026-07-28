#pragma once

#include "Common.h"
#include "Camera.h"

namespace NSRender
{

class PostEffectGodRay : public IDeviceResettable
{

public:

    void Initialize();
    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texZ,
              LPDIRECT3DTEXTURE9 texTarget);
    void Finalize();

    // ゴッドレイ用光源のワールド座標
    void SetLightPos(const D3DXVECTOR3& pos) { m_lightPosWorld = pos; }

    void SetRayLength(const float arg)        { m_rayLength = arg; }
    void SetRayIntensity(const float arg)     { m_rayIntensity = arg; }
    void SetOcclusionFalloff(const float arg) { m_occlusionFalloff = arg; }
    void SetLightColor(const D3DXVECTOR3& c)  { m_lightColor = c; }
    void SetReverseSampling(const bool arg)   { m_reverseSampling = arg; }
    void SetVirtualProximityStrength(const float arg) { m_virtualProximityStrength = arg; }
    void SetDepthRange(const float nearPlane, const float farPlane);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    bool CreateTexture();
    bool HasRenderTextures() const;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);
    void BlurOcclusionTexture();

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    LPD3DXEFFECT        m_d3dEffect      = NULL;
    bool                m_isInitialized = false;
    bool                m_isRegisteredForDeviceReset = false;
    LPDIRECT3DTEXTURE9  m_texOcclusion   = NULL;
    LPDIRECT3DTEXTURE9  m_texBlurTemp    = NULL;
    LPDIRECT3DTEXTURE9  m_texOcclusionBlurred = NULL;

    D3DXVECTOR3 m_lightPosWorld   = D3DXVECTOR3(0.0f, 50.0f, 0.0f);
    float       m_rayLength       = 1.0f;
    float       m_rayIntensity    = 0.6f;
    float       m_occlusionFalloff = 5.0f;
    D3DXVECTOR3 m_lightColor      = D3DXVECTOR3(1.0f, 0.9f, 0.8f);
    bool        m_reverseSampling = false;
    float       m_virtualProximityStrength = 1.5f;
    float       m_depthNearPlane = 0.1f;
    float       m_depthFarPlane = 33'000.0f;
};

}
