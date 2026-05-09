#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectFog : public IDeviceResettable
{

public:

    void Initialize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 texRenderTargetZ,
                            LPDIRECT3DTEXTURE9 texRenderTargetPos,
                            const bool enableZ,
                            const bool enableHeight);
    void Finalize();

    void SetIntensityZ(const float arg);

    void SetIntensityHeight(const float arg);
    void SetHeightStart(const float arg);
    void SetPositionRange(const float positionRange);

    void SetFogColor(const D3DXCOLOR& color);

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

    float m_intensityZ = 2.0f;
    float m_intensityHeight = 0.3f;

    float m_heightStart= 0.0f;
    float m_positionRange = 30'000.0f;
    D3DXVECTOR4 m_fogColor = D3DXVECTOR4(0, 0, 0, 0);

};

}

