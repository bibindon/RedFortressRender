#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectSSAO2 : public IDeviceResettable
{
public:

    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget,
              LPDIRECT3DTEXTURE9 texRenderTargetZ,
              LPDIRECT3DTEXTURE9 texRenderTargetPos,
              LPDIRECT3DTEXTURE9 texRenderTargetNormal,
              LPDIRECT3DTEXTURE9 texRenderTargetThickness);

    void SetShadowStrength(const float shadowStrength);
    void SetSaturationBoost(const float saturationBoost);
    void SetSampleRadius(const float sampleRadius);
    void SetSampleCount(const int sampleCount);
    void SetDepthScaledSampleDistanceEnabled(const bool enabled);
    void SetBlurEnabled(const bool enabled);
    void SetDepthRange(const float nearPlane, const float farPlane);
    void SetTextureScaleDivisor(const int scaleDivisor);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    float m_shadowStrength = 1.0f;
    float m_saturationBoost = 0.30f;
    float m_sampleRadius = 4.0f;
    int m_sampleCount = 16;
    bool m_depthScaledSampleDistanceEnabled = false;
    bool m_blurEnabled = true;
    int m_textureScaleDivisor = 1;
    float m_nearPlane = 0.1f;
    float m_farPlane = 30'000.0f;
    float m_positionRange = 30'000.0f;

    LPD3DXEFFECT m_fxSSAO2 = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    LPDIRECT3DTEXTURE9 m_rtAoTex = NULL;
    LPDIRECT3DTEXTURE9 m_rtAoTempTex = NULL;

    struct FullscreenVertex
    {
        float x;
        float y;
        float z;
        float w;
        float u;
        float v;
    };

    void DrawFullscreenQuad();
    void CreateResources();
    int NormalizeTextureScaleDivisor(const int scaleDivisor) const;
    UINT ComputeTextureSize(const int screenSize) const;
};

}
