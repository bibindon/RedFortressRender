#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectGauss : public IDeviceResettable
{

public:

    void Initialize();
    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget);
    void Finalize();

    void SetSampleSize(const int sampleSize);

    // TODO
    // 0.0 ~ 1.0
    void SetIntensity(const float arg);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPD3DXEFFECT m_d3dEffect = NULL;

    LPDIRECT3DTEXTURE9 m_texWork = NULL;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    float m_intensity = 1.0;
    int m_sampleSize = 101;

};

}

