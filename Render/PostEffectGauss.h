#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectGauss : public IDeviceResettable
{

public:

    void Initialize();
    void DrawHorizontal(LPDIRECT3DTEXTURE9 texSource,
                        LPDIRECT3DTEXTURE9 texTarget);
    void DrawVertical(LPDIRECT3DTEXTURE9 texSource,
                      LPDIRECT3DTEXTURE9 texTarget);
    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget);
    void Finalize();

    void SetSampleSize(const int sampleSize);

    // 0.0 ~ 1.0
    void SetIntensity(const float arg);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique,
                            float filterSpacing = 1.0f);
    void DrawBlendQuad(LPDIRECT3DTEXTURE9 texBase,
                       LPDIRECT3DTEXTURE9 texBlur,
                       LPDIRECT3DTEXTURE9 texTarget,
                       float blend);
    void DrawFullResolutionBlurTo(LPDIRECT3DTEXTURE9 texSource,
                                  LPDIRECT3DTEXTURE9 texTarget);
    void BuildDownChain(LPDIRECT3DTEXTURE9 texSource,
                        int firstLevel,
                        int lastLevel);
    void BuildUpChain(int firstLevel,
                      int lastLevel);
    void DrawStageToTexture(LPDIRECT3DTEXTURE9 texSource,
                            int actualStage,
                            LPDIRECT3DTEXTURE9 texTarget);
    void CreateWorkTextures();
    void ReleaseWorkTextures();
    int ComputeBlurStrength() const;
    int ComputeLevelWidth(int level) const;
    int ComputeLevelHeight(int level) const;

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    static const int GAUSSIAN_LEVEL_COUNT = 5;
    static const int GAUSSIAN_START_EXP = 1;
    static const int GAUSSIAN_LEVEL_EXP_STEP = 1;
    static const int GAUSSIAN_BLUR_STRENGTH_MAX = 96;

    LPDIRECT3DTEXTURE9 m_texDown[GAUSSIAN_LEVEL_COUNT] { };
    LPDIRECT3DTEXTURE9 m_texUp[GAUSSIAN_LEVEL_COUNT] { };
    LPDIRECT3DTEXTURE9 m_texTemp = NULL;
    LPDIRECT3DTEXTURE9 m_texWeak = NULL;

    float m_intensity = 1.0f;
    int m_sampleSize = 101;

};

}

