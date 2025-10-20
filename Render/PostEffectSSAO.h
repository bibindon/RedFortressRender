#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectSSAO
{

public:

    void Initialize();
    void Finalize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 m_rtZTex,
                            LPDIRECT3DTEXTURE9 m_rtPosTex);

    void SetEnable(const bool arg);

    void OnDeviceLost();
    void OnDeviceReset();


private:

    bool m_bEnable = true;

    LPD3DXEFFECT m_fxSSAO = NULL;

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

};

}

