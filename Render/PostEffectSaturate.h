#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectSaturate : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget);

    void SetPostEffectSaturate(const float arg);
    float GetPostEffectSaturate() const;

    void SetEnable(const bool arg);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPD3DXEFFECT m_d3dEffect = NULL;

    LPDIRECT3DTEXTURE9 m_texWork = NULL;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);

    struct ScreenVertex
    {
        float x;
        float y;
        float z;
        float rhw;

        float u;
        float v;
    };

    float m_saturateLevel = 1.0f;

    bool m_bEnable = false;
};

}

