#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectHalo : public IDeviceResettable
{

public:

    void Initialize();
    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget);
    void Finalize();

    void SetThreshold(const float threshold) { m_threshold = threshold; }
    void SetIntensity(const float intensity) { m_intensity = intensity; }
    void SetRadiusPixels(const float radiusPixels) { m_radiusPixels = radiusPixels; }

    void OnDeviceLost();
    void OnDeviceReset();

private:

    static const int HALO_TEXTURE_DIVISOR = 4;

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    LPDIRECT3DTEXTURE9 m_texBright = NULL;
    LPDIRECT3DTEXTURE9 m_texBlur = NULL;

    float m_threshold = 2.5f;
    float m_intensity = 0.35f;
    float m_radiusPixels = 200.0f;

    struct ScreenVertex
    {
        float x;
        float y;
        float z;
        float rhw;
        float u;
        float v;
    };

    void CreateTexture();
    void ReleaseTextures();
    int ComputeTextureWidth() const;
    int ComputeTextureHeight() const;
    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);
    void DrawCombineQuad(LPDIRECT3DTEXTURE9 texScene,
                         LPDIRECT3DTEXTURE9 texTarget);
};

}
