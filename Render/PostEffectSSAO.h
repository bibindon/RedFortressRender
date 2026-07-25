#pragma once

#include "Common.h"

namespace NSRender
{

class PostEffectSSAO : public IDeviceResettable
{
public:

    void Initialize();
    void Finalize();

    void Draw(LPDIRECT3DTEXTURE9 texSource,
              LPDIRECT3DTEXTURE9 texTarget,
              LPDIRECT3DTEXTURE9 texRenderTargetZ,
              LPDIRECT3DTEXTURE9 texRenderTargetNormal,
              LPDIRECT3DTEXTURE9 texRenderTargetThickness);

    void SetShadowStrength(const float shadowStrength);
    void SetSaturationBoost(const float saturationBoost);
    void SetSampleRadius(const float sampleRadius);
    void SetSampleCount(const int sampleCount);
    void SetRandomSamplingDirectionEnabled(const bool enabled);
    void SetDepthScaledSampleDistanceEnabled(const bool enabled);
    void SetBlurEnabled(const bool enabled);
    void SetSeparableBlurEnabled(const bool enabled);
    void SetBlurKernelSize(const int kernelSize);
    void SetDepthRange(const float nearPlane, const float farPlane);
    void SetTextureScaleDivisor(const int scaleDivisor);
    void SetCompositeGaussian3x3Enabled(const bool enabled);
    void SetMaxDarknessClampEnabled(const bool enabled);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    float m_shadowStrength = 1.0f;
    float m_saturationBoost = 0.30f;
    float m_sampleRadius = 4.0f;
    int m_sampleCount = 16;
    bool m_randomSamplingDirectionEnabled = true;
    bool m_depthScaledSampleDistanceEnabled = false;
    bool m_blurEnabled = true;
    bool m_separableBlurEnabled = false;
    int m_blurKernelSize = 21;
    int m_textureScaleDivisor = 1;
    bool m_compositeGaussian3x3Enabled = false;
    bool m_maxDarknessClampEnabled = true;
    float m_nearPlane = 0.1f;
    float m_farPlane = 30'000.0f;

    LPD3DXEFFECT m_fxSSAO = NULL;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    LPDIRECT3DTEXTURE9 m_rtAoTex = NULL;
    LPDIRECT3DTEXTURE9 m_rtAoTempTex = NULL;

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
    const char* GetHorizontalBlurTechniqueName() const;
    const char* GetVerticalBlurTechniqueName() const;
    int NormalizeSampleCount(const int sampleCount) const;
    int NormalizeTextureScaleDivisor(const int scaleDivisor) const;
    const char* GetCompositeTechniqueName() const;
    UINT ComputeTextureSize(const int screenSize) const;
};

}
