#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectSSGI : public IDeviceResettable
{
public:

    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget,
              LPDIRECT3DTEXTURE9 texRenderTargetZ,
              LPDIRECT3DTEXTURE9 texRenderTargetPos,
              LPDIRECT3DTEXTURE9 texRenderTargetNormal,
              LPDIRECT3DTEXTURE9 texRenderTargetThickness);

    void SetSampleRadius(const float sampleRadius);
    void SetSampleCount(const int sampleCount);
    void SetDepthScaledSampleDistanceEnabled(const bool enabled);
    void SetBlurEnabled(const bool enabled);
    void SetBlurKernelSize(const int kernelSize);
    void SetDepthRange(const float nearPlane, const float farPlane);
    void SetTextureScaleDivisor(const int scaleDivisor);
    void SetIndirectLightStrength(const float strength);
    void SetIndirectLightMaxContribution(const float maxContribution);
    void SetUseThicknessEnabled(const bool enabled);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    float m_sampleRadius = 1.0f;
    int m_sampleCount = 16;
    bool m_depthScaledSampleDistanceEnabled = false;
    bool m_blurEnabled = true;
    int m_blurKernelSize = 21;
    int m_textureScaleDivisor = 1;
    float m_nearPlane = 0.1f;
    float m_farPlane = 30.0f;
    float m_indirectLightStrength = 1.0f;
    float m_indirectLightMaxContribution = 1.0f;
    bool m_useThicknessEnabled = true;

    LPD3DXEFFECT m_fxSSGI = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    LPDIRECT3DTEXTURE9 m_rtGiTex = NULL;
    LPDIRECT3DTEXTURE9 m_rtGiTempTex = NULL;

    struct FullscreenVertex
    {
        float x;
        float y;
        float z;
        float w;
        float u;
        float v;
    };

    void DrawFullscreenQuad();
    void CreateResources();
    const char* GetCreateTechniqueName() const;
    int NormalizeBlurKernelSize(const int kernelSize) const;
    const char* GetBlurTechniqueName() const;
    int NormalizeSampleCount(const int sampleCount) const;
    int NormalizeTextureScaleDivisor(const int scaleDivisor) const;
    UINT ComputeTextureSize(const int screenSize) const;
};

}
