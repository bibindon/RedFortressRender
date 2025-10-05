#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectGauss
{

public:

    void Initialize();
    void Draw();
    void Finalize();

private:

    LPD3DXEFFECT g_pEffect3 = NULL;

    LPDIRECT3DTEXTURE9 g_pSceneTex = NULL;
    LPDIRECT3DTEXTURE9 g_pTempTex = NULL;

    LPDIRECT3DTEXTURE9 g_pSceneTex3 = NULL;

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

