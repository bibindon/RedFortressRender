#pragma once

#include "Common.h"

namespace NSRender
{

enum class DepthOfFieldMode
{
    Disabled = 0,
    Enabled = 1,
    AutoNear = 2,
};

class PostEffectDepthOfField : public IDeviceResettable
{
public:

    void Initialize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 texRenderTargetPos);

    void Finalize();

    void SetFocalDistance(float focalDistance);
    void SetMaxBlurDistance(float maxBlurDistance);
    void SetFocusBandHalfWidth(float focusBandHalfWidth);
    void SetBlurRadiusPixels(float blurRadiusPixels);
    void SetAutoActivationDistance(float autoActivationDistance);
    void SetBlend(float blend);
    void SetPositionRange(float positionRange);
    float GetBlend() const;
    void UpdateAutoBlend(LPDIRECT3DTEXTURE9 texRenderTargetPos);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texPosition,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);
    float MeasureCenterNearestDistance(LPDIRECT3DTEXTURE9 texRenderTargetPos);
    void CreateReadbackSurface();

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    LPDIRECT3DTEXTURE9 m_texWork = NULL;
    LPDIRECT3DSURFACE9 m_surfacePositionReadback = NULL;

    float m_focalDistance = 8.0f;
    float m_maxBlurDistance = 16.0f;
    float m_focusBandHalfWidth = 2.0f;
    float m_blurRadiusPixels = 1.0f;
    float m_blend = 1.0f;
    float m_positionRange = 30'000.0f;
    float m_autoActivationDistance = 10.0f;
    float m_autoCenterRadiusNdc = 0.35f;
    float m_autoBlendSpeed = 2.5f;
    DWORD m_lastAutoBlendTick = 0;
};

}
