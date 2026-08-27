#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectMaskedGauss : public IDeviceResettable
{
public:

    void Initialize();
    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget);
    void Finalize();

    void SetSampleSize(const int sampleSize);
    void SetIntensity(const float intensity);
    void SetAmount(const float amount);
    void SetMaskPath(const std::wstring& maskPath);

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    LPD3DXEFFECT m_d3dEffect = nullptr;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    LPDIRECT3DTEXTURE9 m_texDown[5] { };
    LPDIRECT3DTEXTURE9 m_texUp[5] { };
    LPDIRECT3DTEXTURE9 m_texWeak = nullptr;
    LPDIRECT3DTEXTURE9 m_texStrong = nullptr;
    LPDIRECT3DTEXTURE9 m_texBlurResult = nullptr;
    LPDIRECT3DTEXTURE9 m_texMask = nullptr;
    std::wstring m_maskPath;
    std::wstring m_loadedMaskPath;
    int m_sampleSize = 101;
    float m_intensity = 1.0f;
    float m_amount = 1.0f;

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    void CreateWorkTextures();
    void ReleaseWorkTextures();
    void LoadMaskTexture();
    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique,
                            float filterSpacing = 1.0f);
    void DrawBlendQuad(LPDIRECT3DTEXTURE9 texBase,
                       LPDIRECT3DTEXTURE9 texBlur,
                       LPDIRECT3DTEXTURE9 texTarget,
                       float blend);
    void DrawStageToTexture(LPDIRECT3DTEXTURE9 texSource,
                            int actualStage,
                            LPDIRECT3DTEXTURE9 texTarget);
    void DrawFullResolutionBlurTo(LPDIRECT3DTEXTURE9 texSource,
                                  LPDIRECT3DTEXTURE9 texTarget);
    void BuildDownChain(LPDIRECT3DTEXTURE9 texSource,
                        int firstLevel,
                        int lastLevel);
    void BuildUpChain(int firstLevel,
                      int lastLevel);
    void DrawCompositeQuad(LPDIRECT3DTEXTURE9 texBlurred,
                           LPDIRECT3DTEXTURE9 texOriginal,
                           LPDIRECT3DTEXTURE9 texTarget);
    int ComputeBlurStrength() const;
    int ComputeLevelWidth(int level) const;
    int ComputeLevelHeight(int level) const;

    static const int GAUSSIAN_LEVEL_COUNT = 5;
    static const int GAUSSIAN_START_EXP = 1;
    static const int GAUSSIAN_LEVEL_EXP_STEP = 1;
    static const int GAUSSIAN_BLUR_STRENGTH_MAX = 96;
};

}
