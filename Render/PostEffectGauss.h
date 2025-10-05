#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectGauss
{

public:

    void Initialize();
    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget);
    void Finalize();

    void SetEnable(const bool arg);
    bool GetEnable() const;

private:

    LPD3DXEFFECT m_d3dEffect = NULL;

    LPDIRECT3DTEXTURE9 m_texWork = NULL;


    // フルスクリーンクアッド用
//    LPDIRECT3DVERTEXDECLARATION9 g_pQuadDecl = NULL;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 tex, const char* tech);


    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    bool m_bGaussianON = false;

};

}

