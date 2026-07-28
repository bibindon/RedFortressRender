#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectFog : public IDeviceResettable
{

public:

    void Initialize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget,
              LPDIRECT3DTEXTURE9 texRenderTargetZ,
              LPDIRECT3DTEXTURE9 texRenderTargetPos,
              const bool enableZ,
              const bool enableHeight);
    void Finalize();

    void SetIntensityZ(const float arg);

    void SetIntensityHeight(const float arg);
    void SetHeightStart(const float arg);
    void SetPositionRange(const float positionRange);
    void SetDepthDecodeRange(const float nearPlane, const float farPlane);
    void SetFogDepthRange(const float nearPlane, const float farPlane);

    void SetFogColor(const D3DXCOLOR& color);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

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
    float m_positionRange = 33'000.0f;
    float m_depthDecodeNear = 0.1f;
    float m_depthDecodeFar = 33'000.0f;
    float m_fogNear = 0.1f;
    float m_fogFar = 33'000.0f;
    D3DXVECTOR4 m_fogColor = D3DXVECTOR4(0, 0, 0, 0);

};

}

