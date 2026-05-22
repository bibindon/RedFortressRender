#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectTAA : public IDeviceResettable
{
public:
    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget);

    void ResetHistory();
    void SetHistoryWeight(float historyWeight);

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:
    struct ScreenVertex
    {
        float x;
        float y;
        float z;
        float rhw;
        float u;
        float v;
    };

    void CreateResources();
    void ReleaseResources();
    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texTarget);
    void CopyTexture(LPDIRECT3DTEXTURE9 texSource,
                     LPDIRECT3DTEXTURE9 texTarget);

    LPD3DXEFFECT m_d3dEffect = NULL;
    LPDIRECT3DTEXTURE9 m_historyTexture = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    bool m_hasHistory = false;
    float m_historyWeight = 0.85f;
};

}
