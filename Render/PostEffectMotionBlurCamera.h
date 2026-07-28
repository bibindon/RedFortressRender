#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectMotionBlurCamera : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 depthTexture,
              LPDIRECT3DTEXTURE9 texTarget,
              bool& applied);
    void UpdateFrameMatrices();

    void SetQuality(const int quality);
    int GetQuality() const;
    void SetMaxBlurPixels(const float maxBlurPixels);
    float GetMaxBlurPixels() const;
    void SetSampleCount(const int sampleCount);
    int GetSampleCount() const;
    void SetDepthRange(const float nearPlane, const float farPlane);

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    D3DXMATRIX m_prevViewProj { };
    ULONGLONG m_prevFrameTick = 0;
    float m_frameMotionScale = 2.0f;
    bool m_hasPrevViewProj = false;
    int m_quality = 4;
    float m_maxBlurPixels = 24.0f;
    int m_sampleCount = 13;
    float m_depthNearPlane = 0.1f;
    float m_depthFarPlane = 33'000.0f;

    bool ShouldApplyMotionBlur(const D3DXMATRIX& currentViewProj);
    float UpdateFrameMotionScale();
    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 depthTexture,
                            LPDIRECT3DTEXTURE9 texTarget);

    struct ScreenVertex
    {
        float x;
        float y;
        float z;
        float rhw;
        float u;
        float v;
    };

};

}
