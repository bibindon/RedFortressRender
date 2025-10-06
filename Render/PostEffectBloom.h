#pragma once

#include "Common.h"

namespace NSRender
{

// TODO 一度小さく表示してから拡大する、というのをやった方がきれいらしい
class PostEffectBloom
{

public:

    void Initialize();
    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget);
    void Finalize();

    void SetPostEffectSaturate(const float arg);
    float GetPostEffectSaturate() const;

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
        float x;
        float y;
        float z;
        float rhw;

        float u;
        float v;
    };

    float m_saturateLevel = 1.0f;

    // どれくらいの明るさからブルームの対象とするか
    float m_threshold = 1.0f;

    // ブルームの明るさ
    float m_intensity = 1.0f;

    // ブルームの広さ
    float m_area = 1.0f;
};

}

