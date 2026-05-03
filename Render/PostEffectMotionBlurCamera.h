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

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    LPD3DXEFFECT m_d3dEffect = NULL;
    LPDIRECT3DTEXTURE9 m_texWork = NULL;
    D3DXMATRIX m_prevViewProj { };
    bool m_hasPrevViewProj = false;
    int m_quality = 4;

    void CreateTexture();
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
