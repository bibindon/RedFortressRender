#pragma once

#include "Common.h"

namespace NSRender
{

// TODO 一度小さく表示してから拡大する、というのをやった方がきれいらしい
class PostEffectStarBurst : public IDeviceResettable
{

public:

    void Initialize();
    LPDIRECT3DTEXTURE9 Draw(LPDIRECT3DTEXTURE9 renderTarget);
    void Finalize();

    void SetEnable(const bool arg);
    void SetThreshold(const float arg);
    void SetIntensity(const float arg);
    void SetSize(const float arg);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPD3DXEFFECT m_d3dEffect = NULL;

    LPDIRECT3DTEXTURE9 m_texPostEffectBack1 = NULL;
    LPDIRECT3DTEXTURE9 m_texBright = NULL;
    LPDIRECT3DTEXTURE9 m_texBlurH = NULL;
    LPDIRECT3DTEXTURE9 m_texBlurV = NULL;
    LPDIRECT3DTEXTURE9 m_texBlurD = NULL;
    LPDIRECT3DTEXTURE9 m_texBlurH2 = NULL;
    LPDIRECT3DTEXTURE9 m_texBlurV2 = NULL;
    LPDIRECT3DTEXTURE9 m_texBlurD2 = NULL;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texTarget,
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

    // どれくらいの明るさからブルームの対象とするか
    // 0.0 ~ 1.0
    float m_threshold = 1.0f;

    // ブルームの明るさ
    // 0.0 ~ 1.0
    float m_intensity = 1.0f;

    // ブルームの広さ
    // 0.0 ~ 1.0
    float m_size = 1.0f;

    bool m_bEnable = false;

    void SetRTFromTex(LPDIRECT3DTEXTURE9 tex);

    void CreateTexture();
};

}

