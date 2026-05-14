#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectFont : public IDeviceResettable
{
public:

    void Initialize();
    void Finalize();

    void BeginShadowPass();
    void DrawBlurredShadow();
    void SetGaussianSampleSize(const int sampleSize);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    void CreateResources();
    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DSURFACE9 renderTarget,
                            const char* technique,
                            const bool enableAlphaBlend);
    static int NormalizeGaussianSampleSize(const int sampleSize);

    LPD3DXEFFECT m_d3dEffect = NULL;
    LPDIRECT3DTEXTURE9 m_shadowTexture = NULL;
    LPDIRECT3DTEXTURE9 m_blurTexture = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    int m_gaussianSampleSize = 21;
};

}
