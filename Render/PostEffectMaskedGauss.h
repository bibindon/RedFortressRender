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
    void SetMaskPath(const std::wstring& maskPath);

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    LPD3DXEFFECT m_d3dEffect = nullptr;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    LPDIRECT3DTEXTURE9 m_texBlurWork = nullptr;
    LPDIRECT3DTEXTURE9 m_texBlurResult = nullptr;
    LPDIRECT3DTEXTURE9 m_texMask = nullptr;
    std::wstring m_maskPath;
    int m_sampleSize = 101;

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
                            const std::string& technique);
    void DrawCompositeQuad(LPDIRECT3DTEXTURE9 texBlurred,
                           LPDIRECT3DTEXTURE9 texOriginal,
                           LPDIRECT3DTEXTURE9 texTarget);
};

}
