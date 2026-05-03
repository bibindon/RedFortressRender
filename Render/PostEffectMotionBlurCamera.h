#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectMotionBlurCamera : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 depthTexture);
    void UpdateFrameMatrices();

    void SetQuality(const int quality);
    int GetQuality() const;
    void SetMaxBlurPixels(const float maxBlurPixels);
    float GetMaxBlurPixels() const;
    void SetSampleCount(const int sampleCount);
    int GetSampleCount() const;

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    LPD3DXEFFECT m_d3dEffect = NULL;
    LPDIRECT3DTEXTURE9 m_texWork = NULL;
    D3DXMATRIX m_prevViewProj { };
    D3DXMATRIX m_motionBlurPrevViewProj { };
    D3DXVECTOR3 m_prevEye { };
    D3DXVECTOR3 m_prevLookAt { };
    ULONGLONG m_prevFrameTick = 0;
    float m_frameMotionScale = 2.0f;
    float m_motionBlurScaleThisFrame = 1.0f;
    bool m_hasPrevViewProj = false;
    int m_quality = 4;
    float m_maxBlurPixels = 24.0f;
    int m_sampleCount = 13;

    void CreateTexture();
    bool ShouldApplyMotionBlur(const D3DXMATRIX& currentViewProj);
    float UpdateFrameMotionScale();
    void UpdateMotionBlurPrevViewProj();
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
