#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectSSAO : public IDeviceResettable
{

public:

    void Initialize();
    void Finalize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 m_texRenderTargetZ,
                            LPDIRECT3DTEXTURE9 m_texRenderTargetPos,
                            LPDIRECT3DTEXTURE9 m_texRenderTargetNormal);

    void SetEnable(const bool arg);
    void SetBrightness(const float brightness);

    void OnDeviceLost();
    void OnDeviceReset();

    // SSAOの距離が表示される範囲
    static constexpr float Z_RANGE = 50.f;

private:

    bool m_bEnable = true;
    float m_brightness = 1.0f;

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

