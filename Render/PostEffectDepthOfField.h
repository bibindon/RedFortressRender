#pragma once

#include <vector>

#include "Common.h"

namespace NSRender
{

enum class DepthOfFieldMode
{
    Disabled = 0,
    Enabled = 1,
    AutoNear = 2,
};

class PostEffectDepthOfField : public IDeviceResettable
{
public:

    void Initialize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texRenderTargetPos,
              LPDIRECT3DTEXTURE9 texTarget,
              bool useAutoBlendTexture = false);

    void Finalize();

    void SetFocalDistance(float focalDistance);
    void SetStartNear(float startNear);
    void SetMaxBlurDistance(float maxBlurDistance);
    void SetFocusBandHalfWidth(float focusBandHalfWidth);
    void SetBlurRadiusPixels(float blurRadiusPixels);
    void SetAutoActivationDistance(float autoActivationDistance);
    void SetBlend(float blend);
    void SetPositionRange(float positionRange);
    void UpdateAutoBlend(LPDIRECT3DTEXTURE9 texCameraDepth,
                         float nearPlane,
                         float farPlane,
                         float deltaTime);
    void ResetAutoBlend();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                            LPDIRECT3DTEXTURE9 texPosition,
                            LPDIRECT3DTEXTURE9 texTarget,
                            const std::string& technique);
    void DrawAutoPass(LPDIRECT3DTEXTURE9 texSource,
                      LPDIRECT3DTEXTURE9 texTarget,
                      int targetWidth,
                      int targetHeight,
                      const std::string& technique);
    void CreateAutoResources();
    void ReleaseAutoResources();

    struct AutoDepthLevel
    {
        LPDIRECT3DTEXTURE9 texture = NULL;
        int width = 0;
        int height = 0;
    };

    LPD3DXEFFECT m_d3dEffect = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    std::vector<AutoDepthLevel> m_autoDepthLevels;
    LPDIRECT3DTEXTURE9 m_autoBlendTextures[2] { NULL, NULL };
    int m_autoBlendTextureIndex = 0;

    float m_focalDistance = 8.0f;
    float m_startNear = 0.0f;
    float m_maxBlurDistance = 16.0f;
    float m_focusBandHalfWidth = 2.0f;
    float m_blurRadiusPixels = 1.0f;
    float m_blend = 1.0f;
    float m_positionRange = 30'000.0f;
    float m_autoActivationDistance = 10.0f;
    float m_autoCenterRadiusNdc = 0.35f;
    float m_autoBlendSpeed = 2.5f;
};

}
