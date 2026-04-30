#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectDepthOfField : public IDeviceResettable
{
public:

    void Initialize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 texRenderTargetPos);

    void Finalize();

    void SetEnable(bool enable);
    bool GetEnable() const;

    void SetFocalDistance(float focalDistance);
    void SetFocusBandHalfWidth(float focusBandHalfWidth);
    void SetBlurRadiusPixels(float blurRadiusPixels);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texPosition,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);

    LPD3DXEFFECT m_d3dEffect = NULL;
    LPDIRECT3DTEXTURE9 m_texWork = NULL;

    bool m_enable = false;
    float m_focalDistance = 8.0f;
    float m_focusBandHalfWidth = 2.0f;
    float m_blurRadiusPixels = 1.0f;
};

}
