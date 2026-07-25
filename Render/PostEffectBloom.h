#pragma once

#include "Common.h"

namespace NSRender
{

// TODO 一度小さく表示してから拡大する、というのをやった方がきれいらしい
class PostEffectBloom : public IDeviceResettable
{

public:

    void Initialize();
    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget);
    void Draw(LPDIRECT3DTEXTURE9 texEffectSource,
              LPDIRECT3DTEXTURE9 texScene,
              LPDIRECT3DTEXTURE9 texTarget);
    void Finalize();

    void SetThreshold(const float arg);
    void SetIntensity(const float arg);
    void SetSize(const float arg);
    void SetWeightSum(const float arg);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    static const int BLOOM_LEVEL_COUNT = 4;
    static const int BLOOM_LEVEL_DIVISORS[BLOOM_LEVEL_COUNT];

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    LPDIRECT3DTEXTURE9 m_texDownsample[BLOOM_LEVEL_COUNT] { };
    LPDIRECT3DTEXTURE9 m_texBlur[BLOOM_LEVEL_COUNT] { };
    LPDIRECT3DTEXTURE9 m_texUpsample[BLOOM_LEVEL_COUNT] { };

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);
    void DrawUpsampleQuad(LPDIRECT3DTEXTURE9 texSource,
                          LPDIRECT3DTEXTURE9 texAdd,
                          LPDIRECT3DTEXTURE9 texTarget);
    void DrawCombineQuad(LPDIRECT3DTEXTURE9 texScene,
                         LPDIRECT3DTEXTURE9 texTarget);
    void ReleaseTextures();
    int ComputeBloomTextureWidth(const int divisor) const;
    int ComputeBloomTextureHeight(const int divisor) const;

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
    float m_threshold = 0.95f;

    // ブルームの明るさ
    // 0.0 ~ 1.0
    float m_intensity = 1.0f;

    // ブルームの広さ
    // 0.0 ~ 1.0
    float m_size = 1.0f;

    // 合成時の重み合計
    float m_weightSum = 0.1f;

    void CreateTexture();
};

}

