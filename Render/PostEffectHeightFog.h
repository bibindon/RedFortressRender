// 高さフォグ
// 高さフォグでは、高さとカメラとの距離で霧の強さを決める。
#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectHeightFog : public IDeviceResettable
{
public:

    void Initialize();

    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 texRenderTargetPos);
    void Finalize();

    void SetIntensity(const float intensity);
    void SetStartHeight(const float startHeight);
    void SetMaxHeight(const float maxHeight);
    void SetFogColor(const D3DXCOLOR& color);

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);

    LPD3DXEFFECT m_d3dEffect = nullptr;
    LPDIRECT3DTEXTURE9 m_texWork = nullptr;
    float m_intensity = 0.3f;
    float m_startHeight = 0.0f;
    float m_maxHeight = -5.0f;
    D3DXVECTOR4 m_fogColor = D3DXVECTOR4(0.72f, 0.78f, 0.86f, 1.0f);
};

}
