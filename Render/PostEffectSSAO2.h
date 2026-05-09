#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectSSAO2 : public IDeviceResettable
{
public:

    void Initialize();
    void Finalize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 texRenderTargetZ,
                            LPDIRECT3DTEXTURE9 texRenderTargetPos,
                            LPDIRECT3DTEXTURE9 texRenderTargetNormal,
                            LPDIRECT3DTEXTURE9 texRenderTargetThickness);

    void SetBrightness(const float brightness);
    void SetSaturationBoost(const float saturationBoost);
    void SetSampleRadius(const float sampleRadius);
    void SetBlurEnabled(const bool enabled);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    float m_brightness = 1.0f;
    float m_saturationBoost = 0.30f;
    float m_sampleRadius = 4.0f;
    bool m_blurEnabled = true;

    LPD3DXEFFECT m_fxSSAO2 = NULL;

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
};

}
