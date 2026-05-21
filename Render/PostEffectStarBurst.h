#pragma once

#include "Common.h"

namespace NSRender
{

// TODO 一度小さく表示してから拡大する、というのをやった方がきれいらしい
class PostEffectStarBurst : public IDeviceResettable
{

public:

    void Initialize();
    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget);
    void Finalize();

    void SetThreshold(const float arg);
    void SetIntensity(const float arg);
    void SetSize(const float arg);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    static const int STARBURST_LEVEL_COUNT = 7;
    static const int STARBURST_LEVEL_DIVISORS[STARBURST_LEVEL_COUNT];

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    LPDIRECT3DTEXTURE9 m_texDownsample[STARBURST_LEVEL_COUNT] { };
    LPDIRECT3DTEXTURE9 m_texBlur[STARBURST_LEVEL_COUNT] { };

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);
    void DrawCombineQuad(LPDIRECT3DTEXTURE9 texScene,
                         LPDIRECT3DTEXTURE9 texTarget);
    void ReleaseTextures();
    int ComputeTextureWidth(const int divisor) const;
    int ComputeTextureHeight(const int divisor) const;

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

    void CreateTexture();
};

}

