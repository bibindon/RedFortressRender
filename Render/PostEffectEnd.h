#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectEnd : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 renderTarget);
    void DrawSingleChannel(LPDIRECT3DTEXTURE9 renderTarget);
    void DrawOverlay(LPDIRECT3DTEXTURE9 renderTarget,
                     const int x,
                     const int y,
                     const int width,
                     const int height);

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    LPD3DXEFFECT m_d3dEffect = NULL;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);
    void DrawScreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                        const float left,
                        const float top,
                        const float right,
                        const float bottom,
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

};

}

