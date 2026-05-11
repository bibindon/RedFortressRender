#pragma comment( lib, "d3d9.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif

#include "Render.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <algorithm>
#include <cassert>
#include <crtdbg.h>
#include <cwctype>
#include <vector>

#include "Common.h"

#include "MeshOld.h"
#include "AnimMesh.h"
#include "SkinAnimMesh.h"
#include "MeshMixSkinAnim.h"

#include "Camera.h"
#include "Light.h"

#include "Font.h"
#include <chrono>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cwctype>
#include <utility>

namespace NSRender
{
namespace
{
bool TryParseBoolSetting(const std::wstring& value, bool& result)
{
    std::wstring normalized;
    normalized.reserve(value.size());
    for (wchar_t ch : value)
    {
        normalized.push_back(static_cast<wchar_t>(std::towlower(ch)));
    }

    if (normalized == L"1" || normalized == L"true" || normalized == L"on" || normalized == L"yes")
    {
        result = true;
        return true;
    }

    if (normalized == L"0" || normalized == L"false" || normalized == L"off" || normalized == L"no")
    {
        result = false;
        return true;
    }

    return false;
}
}

std::wstring Render::Trim(const std::wstring& text)
{
    const auto first = std::find_if_not(text.begin(), text.end(), [](wchar_t ch)
    {
        return std::iswspace(ch) != 0;
    });

    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](wchar_t ch)
    {
        return std::iswspace(ch) != 0;
    }).base();

    if (first >= last)
    {
        return L"";
    }

    return std::wstring(first, last);
}

int Render::NormalizeGaussianSampleSize(const int sampleSize)
{
    int normalized = (std::max)(1, (std::min)(sampleSize, 101));

    if ((normalized % 2) == 0)
    {
        --normalized;
    }

    return (std::max)(1, normalized);
}

int NormalizeFXAAQuality(const int quality)
{
    return (std::max)(1, (std::min)(quality, 8));
}

int NormalizeMotionBlurCameraQuality(const int quality)
{
    return (std::max)(1, (std::min)(quality, 8));
}

float NormalizeMotionBlurCameraMaxBlurPixels(const float maxBlurPixels)
{
    return (std::max)(1.0f, (std::min)(maxBlurPixels, 64.0f));
}

int NormalizeMotionBlurCameraSampleCount(const int sampleCount)
{
    return (std::max)(2, (std::min)(sampleCount, 21));
}

float ClampUnitSetting(const float value)
{
    return (std::max)(0.0f, (std::min)(value, 1.0f));
}

void Render::LoadSettingsCsv(const std::wstring& settingsCsvPath)
{
    m_settings.clear();

    if (settingsCsvPath.empty())
    {
        return;
    }

    std::wifstream file(settingsCsvPath);
    if (!file)
    {
        return;
    }

    std::wstring line;
    while (std::getline(file, line))
    {
        const std::size_t commentPos = line.find(L'#');
        if (commentPos != std::wstring::npos)
        {
            line = line.substr(0, commentPos);
        }

        const std::size_t commaPos = line.find(L',');
        if (commaPos == std::wstring::npos)
        {
            continue;
        }

        const std::wstring key = Trim(line.substr(0, commaPos));
        const std::wstring value = Trim(line.substr(commaPos + 1));

        if (!key.empty())
        {
            m_settings[key] = value;
        }
    }
}

void Render::ReloadSettingsCsv(const std::wstring& settingsCsvPath)
{
    // CSV の再読み込みも初期化時と同じ setter 経路を通して反映する。
    LoadSettingsCsv(settingsCsvPath);
    ApplySettings();
}

void Render::ApplySettings()
{
    const auto cameraNear = m_settings.find(L"CameraNear");
    const auto cameraFar = m_settings.find(L"CameraFar");
    if (cameraNear != m_settings.end() || cameraFar != m_settings.end())
    {
        float nearPlane = Camera::GetNear();
        float farPlane = Camera::GetFar();
        try
        {
            if (cameraNear != m_settings.end())
            {
                nearPlane = std::stof(cameraNear->second);
            }
            if (cameraFar != m_settings.end())
            {
                farPlane = std::stof(cameraFar->second);
            }
            SetCameraClipPlanes(nearPlane, farPlane);
        }
        catch (...)
        {
        }
    }

    const auto gbufferNear = m_settings.find(L"GBufferNear");
    const auto gbufferFar = m_settings.find(L"GBufferFar");
    if (gbufferNear != m_settings.end() || gbufferFar != m_settings.end())
    {
        float nearPlane = 0.1f;
        float farPlane = 30'000.0f;
        try
        {
            if (gbufferNear != m_settings.end())
            {
                nearPlane = std::stof(gbufferNear->second);
            }
            if (gbufferFar != m_settings.end())
            {
                farPlane = std::stof(gbufferFar->second);
            }
            SetGBufferClipPlanes(nearPlane, farPlane);
        }
        catch (...)
        {
        }
    }

    const auto depthBufferShadowEnable = m_settings.find(L"DepthBufferShadowEnable");
    if (depthBufferShadowEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(depthBufferShadowEnable->second, enabled))
        {
            SetPostEffectDepthBufferShadow(enabled);
        }
    }

    const auto ssao2Enable = m_settings.find(L"SSAO2Enable");
    if (ssao2Enable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssao2Enable->second, enabled))
        {
            SetPostEffectSSAO2(enabled);
        }
    }

    const auto ssao2BlurEnable = m_settings.find(L"SSAO2BlurEnable");
    if (ssao2BlurEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssao2BlurEnable->second, enabled))
        {
            SetPostEffectSSAO2Blur(enabled);
        }
    }

    const auto ssao2DepthScaledSampleDistanceEnable = m_settings.find(L"SSAO2DepthScaledSampleDistanceEnable");
    if (ssao2DepthScaledSampleDistanceEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(ssao2DepthScaledSampleDistanceEnable->second, enabled))
        {
            SetPostEffectSSAO2DepthScaledSampleDistance(enabled);
        }
    }

    const auto fogEnable = m_settings.find(L"FogEnable");
    if (fogEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(fogEnable->second, enabled))
        {
            SetPostEffectFog(enabled);
        }
    }

    const auto saturateEnable = m_settings.find(L"SaturateEnable");
    if (saturateEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(saturateEnable->second, enabled))
        {
            SetPostEffectSaturateEnable(enabled);
        }
    }

    const auto gaussianEnable = m_settings.find(L"GaussianEnable");
    if (gaussianEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(gaussianEnable->second, enabled))
        {
            SetPostEffectGaussianFilter(enabled);
        }
    }

    const auto gaussianSampleSize = m_settings.find(L"GaussianSampleSize");
    if (gaussianSampleSize != m_settings.end())
    {
        try
        {
            SetPostEffectGaussianSampleSize(std::stoi(gaussianSampleSize->second));
        }
        catch (...)
        {
            SetPostEffectGaussianSampleSize(m_gaussianSampleSize);
        }
    }
    else
    {
        SetPostEffectGaussianSampleSize(m_gaussianSampleSize);
    }

    const auto maskedGaussianEnable = m_settings.find(L"MaskedGaussianEnable");
    if (maskedGaussianEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(maskedGaussianEnable->second, enabled))
        {
            SetPostEffectMaskedGaussianFilter(enabled);
        }
    }

    const auto maskedGaussianMaskPath = m_settings.find(L"MaskedGaussianMaskPath");
    if (maskedGaussianMaskPath != m_settings.end())
    {
        SetPostEffectMaskedGaussianMaskPath(maskedGaussianMaskPath->second);
    }

    const auto fxaaEnable = m_settings.find(L"FXAAEnable");
    if (fxaaEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(fxaaEnable->second, enabled))
        {
            SetPostEffectFXAA(enabled);
        }
    }

    const auto fxaaQuality = m_settings.find(L"FXAAQuality");
    if (fxaaQuality != m_settings.end())
    {
        try
        {
            SetPostEffectFXAAQuality(std::stoi(fxaaQuality->second));
        }
        catch (...)
        {
            SetPostEffectFXAAQuality(m_fxaaQuality);
        }
    }
    else
    {
        SetPostEffectFXAAQuality(m_fxaaQuality);
    }

    const auto motionBlurCameraEnable = m_settings.find(L"MotionBlurCameraEnable");
    if (motionBlurCameraEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(motionBlurCameraEnable->second, enabled))
        {
            SetPostEffectMotionBlurCamera(enabled);
        }
    }

    const auto motionBlurCameraQuality = m_settings.find(L"MotionBlurCameraQuality");
    if (motionBlurCameraQuality != m_settings.end())
    {
        try
        {
            SetPostEffectMotionBlurCameraQuality(std::stoi(motionBlurCameraQuality->second));
        }
        catch (...)
        {
            SetPostEffectMotionBlurCameraQuality(m_motionBlurCameraQuality);
        }
    }
    else
    {
        SetPostEffectMotionBlurCameraQuality(m_motionBlurCameraQuality);
    }

    const auto motionBlurCameraMaxBlurPixels = m_settings.find(L"MotionBlurCameraMaxBlurPixels");
    if (motionBlurCameraMaxBlurPixels != m_settings.end())
    {
        try
        {
            SetPostEffectMotionBlurCameraMaxBlurPixels(std::stof(motionBlurCameraMaxBlurPixels->second));
        }
        catch (...)
        {
            SetPostEffectMotionBlurCameraMaxBlurPixels(m_motionBlurCameraMaxBlurPixels);
        }
    }

    const auto motionBlurCameraSampleCount = m_settings.find(L"MotionBlurCameraSampleCount");
    if (motionBlurCameraSampleCount != m_settings.end())
    {
        try
        {
            SetPostEffectMotionBlurCameraSampleCount(std::stoi(motionBlurCameraSampleCount->second));
        }
        catch (...)
        {
            SetPostEffectMotionBlurCameraSampleCount(m_motionBlurCameraSampleCount);
        }
    }

    const auto sssEnable = m_settings.find(L"SSSEnable");
    if (sssEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(sssEnable->second, enabled))
        {
            SetMeshMixSSS(enabled);
        }
    }

    const auto sssIntensity = m_settings.find(L"SSSIntensity");
    if (sssIntensity != m_settings.end())
    {
        try
        {
            SetMeshMixSSSIntensity(std::stof(sssIntensity->second));
        }
        catch (...)
        {
            SetMeshMixSSSIntensity(m_meshMixSSSIntensity);
        }
    }

    float sssColorR = static_cast<float>((m_meshMixSSSColor >> 16) & 0xff) / 255.0f;
    float sssColorG = static_cast<float>((m_meshMixSSSColor >> 8) & 0xff) / 255.0f;
    float sssColorB = static_cast<float>(m_meshMixSSSColor & 0xff) / 255.0f;
    bool sssColorChanged = false;

    const auto sssColorRSetting = m_settings.find(L"SSSColorR");
    if (sssColorRSetting != m_settings.end())
    {
        try
        {
            sssColorR = ClampUnitSetting(std::stof(sssColorRSetting->second));
            sssColorChanged = true;
        }
        catch (...)
        {
        }
    }

    const auto sssColorGSetting = m_settings.find(L"SSSColorG");
    if (sssColorGSetting != m_settings.end())
    {
        try
        {
            sssColorG = ClampUnitSetting(std::stof(sssColorGSetting->second));
            sssColorChanged = true;
        }
        catch (...)
        {
        }
    }

    const auto sssColorBSetting = m_settings.find(L"SSSColorB");
    if (sssColorBSetting != m_settings.end())
    {
        try
        {
            sssColorB = ClampUnitSetting(std::stof(sssColorBSetting->second));
            sssColorChanged = true;
        }
        catch (...)
        {
        }
    }

    if (sssColorChanged)
    {
        const DWORD r = static_cast<DWORD>(sssColorR * 255.0f + 0.5f);
        const DWORD g = static_cast<DWORD>(sssColorG * 255.0f + 0.5f);
        const DWORD b = static_cast<DWORD>(sssColorB * 255.0f + 0.5f);
        SetMeshMixSSSColor((r << 16) | (g << 8) | b);
    }

    const auto fogIntensity = m_settings.find(L"FogIntensity");
    if (fogIntensity != m_settings.end())
    {
        try
        {
            SetPostEffectFogIntensity(std::stof(fogIntensity->second));
        }
        catch (...)
        {
            SetPostEffectFogIntensity(2.0f);
        }
    }
    else
    {
        SetPostEffectFogIntensity(2.0f);
    }

    const auto fogHeightEnable = m_settings.find(L"FogHeightEnable");
    if (fogHeightEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(fogHeightEnable->second, enabled))
        {
            SetPostEffectFogHeightEnable(enabled);
        }
    }

    const auto fogHeightIntensity = m_settings.find(L"FogHeightIntensity");
    if (fogHeightIntensity != m_settings.end())
    {
        try
        {
            SetPostEffectFogHeightIntensity(std::stof(fogHeightIntensity->second));
        }
        catch (...)
        {
            SetPostEffectFogHeightIntensity(0.3f);
        }
    }
    else
    {
        SetPostEffectFogHeightIntensity(0.3f);
    }

    const auto fogHeightStart = m_settings.find(L"FogHeightStart");
    if (fogHeightStart != m_settings.end())
    {
        try
        {
            SetPostEffectFogHeightStart(std::stof(fogHeightStart->second));
        }
        catch (...)
        {
            SetPostEffectFogHeightStart(0.0f);
        }
    }
    else
    {
        SetPostEffectFogHeightStart(0.0f);
    }

    const auto fogHeightMax = m_settings.find(L"FogHeightMax");
    if (fogHeightMax != m_settings.end())
    {
        try
        {
            SetPostEffectHeightFogMax(std::stof(fogHeightMax->second));
        }
        catch (...)
        {
            SetPostEffectHeightFogMax(-5.0f);
        }
    }
    else
    {
        SetPostEffectHeightFogMax(-5.0f);
    }

    const auto fogHeightDistanceStart = m_settings.find(L"FogHeightDistanceStart");
    if (fogHeightDistanceStart != m_settings.end())
    {
        try
        {
            SetPostEffectHeightFogDistanceStart(std::stof(fogHeightDistanceStart->second));
        }
        catch (...)
        {
            SetPostEffectHeightFogDistanceStart(0.0f);
        }
    }
    else
    {
        SetPostEffectHeightFogDistanceStart(0.0f);
    }

    const auto fogHeightDistanceMax = m_settings.find(L"FogHeightDistanceMax");
    if (fogHeightDistanceMax != m_settings.end())
    {
        try
        {
            SetPostEffectHeightFogDistanceMax(std::stof(fogHeightDistanceMax->second));
        }
        catch (...)
        {
            SetPostEffectHeightFogDistanceMax(20.0f);
        }
    }
    else
    {
        SetPostEffectHeightFogDistanceMax(20.0f);
    }

    const auto shadowIntensity = m_settings.find(L"ShadowIntensity");
    if (shadowIntensity != m_settings.end())
    {
        try
        {
            SetPostEffectDepthBufferShadowIntensity(std::stof(shadowIntensity->second));
        }
        catch (...)
        {
            SetPostEffectDepthBufferShadowIntensity(0.5f);
        }
    }
    else
    {
        SetPostEffectDepthBufferShadowIntensity(0.5f);
    }

    const auto ssao2ShadowStrength = m_settings.find(L"SSAO2ShadowStrength");
    if (ssao2ShadowStrength != m_settings.end())
    {
        try
        {
            SetPostEffectSSAO2ShadowStrength(std::stof(ssao2ShadowStrength->second));
        }
        catch (...)
        {
            SetPostEffectSSAO2ShadowStrength(1.0f);
        }
    }
    else
    {
        SetPostEffectSSAO2ShadowStrength(1.0f);
    }

    const auto ssao2ShadowSaturationBoost = m_settings.find(L"SSAO2ShadowSaturationBoost");
    if (ssao2ShadowSaturationBoost != m_settings.end())
    {
        try
        {
            SetPostEffectSSAO2SaturationBoost(std::stof(ssao2ShadowSaturationBoost->second));
        }
        catch (...)
        {
            SetPostEffectSSAO2SaturationBoost(0.30f);
        }
    }
    else
    {
        SetPostEffectSSAO2SaturationBoost(0.30f);
    }

    const auto ssao2SampleCount = m_settings.find(L"SSAO2SampleCount");
    if (ssao2SampleCount != m_settings.end())
    {
        try
        {
            SetPostEffectSSAO2SampleCount(std::stoi(ssao2SampleCount->second));
        }
        catch (...)
        {
            SetPostEffectSSAO2SampleCount(16);
        }
    }
    else
    {
        SetPostEffectSSAO2SampleCount(16);
    }

    const auto shadowSaturationBoost = m_settings.find(L"ShadowSaturationBoost");
    if (shadowSaturationBoost != m_settings.end())
    {
        try
        {
            SetPostEffectDepthBufferShadowSaturationBoost(std::stof(shadowSaturationBoost->second));
        }
        catch (...)
        {
            SetPostEffectDepthBufferShadowSaturationBoost(0.35f);
        }
    }
    else
    {
        SetPostEffectDepthBufferShadowSaturationBoost(0.35f);
    }

    const auto ssao2SampleRadius = m_settings.find(L"SSAO2SampleRadius");
    if (ssao2SampleRadius != m_settings.end())
    {
        try
        {
            SetPostEffectSSAO2SampleRadius(std::stof(ssao2SampleRadius->second));
        }
        catch (...)
        {
            SetPostEffectSSAO2SampleRadius(4.0f);
        }
    }
    else
    {
        SetPostEffectSSAO2SampleRadius(4.0f);
    }

    const auto bloomEnable = m_settings.find(L"BloomEnable");
    if (bloomEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(bloomEnable->second, enabled))
        {
            SetPostEffectBloom(enabled);
        }
    }

    const auto bloomThreshold = m_settings.find(L"BloomThreshold");
    if (bloomThreshold != m_settings.end())
    {
        try
        {
            SetPostEffectBloomThreshold(std::stof(bloomThreshold->second));
        }
        catch (...)
        {
            SetPostEffectBloomThreshold(2.5f);
        }
    }
    else
    {
        SetPostEffectBloomThreshold(2.5f);
    }

    const auto depthOfFieldMode = m_settings.find(L"DepthOfFieldMode");
    if (depthOfFieldMode != m_settings.end())
    {
        try
        {
            const int modeValue = std::stoi(depthOfFieldMode->second);
            if (modeValue <= 0)
            {
                SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
            }
            else if (modeValue == 1)
            {
                SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Enabled);
            }
            else
            {
                SetPostEffectDepthOfFieldMode(DepthOfFieldMode::AutoNear);
            }
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
        }
    }
    else
    {
        const auto depthOfFieldEnable = m_settings.find(L"DepthOfFieldEnable");
        if (depthOfFieldEnable != m_settings.end())
        {
            bool enabled = false;
            if (TryParseBoolSetting(depthOfFieldEnable->second, enabled))
            {
                SetPostEffectDepthOfField(enabled);
            }
        }
    }

    const auto depthOfFieldFocalDistance = m_settings.find(L"DepthOfFieldFocalDistance");
    if (depthOfFieldFocalDistance != m_settings.end())
    {
        try
        {
            SetPostEffectDepthOfFieldFocalDistance(std::stof(depthOfFieldFocalDistance->second));
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldFocalDistance(8.0f);
        }
    }
    else
    {
        SetPostEffectDepthOfFieldFocalDistance(8.0f);
    }

    const auto depthOfFieldAutoActivationDistance = m_settings.find(L"DepthOfFieldAutoActivationDistance");
    if (depthOfFieldAutoActivationDistance != m_settings.end())
    {
        try
        {
            SetPostEffectDepthOfFieldAutoActivationDistance(std::stof(depthOfFieldAutoActivationDistance->second));
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldAutoActivationDistance(10.0f);
        }
    }
    else
    {
        SetPostEffectDepthOfFieldAutoActivationDistance(10.0f);
    }

    const auto starBurstEnable = m_settings.find(L"StarBurstEnable");
    if (starBurstEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(starBurstEnable->second, enabled))
        {
            SetPostEffectStarBurst(enabled);
        }
    }

    const auto starBurstThreshold = m_settings.find(L"StarBurstThreshold");
    if (starBurstThreshold != m_settings.end())
    {
        try
        {
            SetPostEffectStarBurstThreshold(std::stof(starBurstThreshold->second));
        }
        catch (...)
        {
            SetPostEffectStarBurstThreshold(2.8f);
        }
    }
    else
    {
        SetPostEffectStarBurstThreshold(2.8f);
    }
}

void Render::Initialize(HWND hWnd, const std::wstring& settingsCsvPath)
{
    HRESULT hResult = E_FAIL;

    m_hWnd = hWnd;

    LoadSettingsCsv(settingsCsvPath);

    m_windowManager.Initialize(hWnd);

    m_sprite.Initialize();

    CreateTexture();

    m_GBuffer.Initialize();

    ApplySettings();

    // 画面転送
    m_postEffectEnd.Initialize();

    Common::AddDeviceLostResource(this);
}

void Render::Finalize()
{
    Common::D3DDevice()->Release();
    Common::SetD3DDevice(NULL);
}

void Render::Draw()
{
    HRESULT hResult = E_FAIL;

    if (m_bShowFPS)
    {
        float fps = CalcFPS();
        ShowFPS(fps);
    }

    //---------------------------------------------------------------
    // ポストエフェクトと一部のメッシュ描画のために深度画像と
    // ワールド座標画像を先に作成
    //---------------------------------------------------------------
    LPDIRECT3DTEXTURE9 pTexTempZ = NULL;
    LPDIRECT3DTEXTURE9 pTexTempPos = NULL;
    LPDIRECT3DTEXTURE9 pTexTempNoral = NULL;
    LPDIRECT3DTEXTURE9 pTexTempThickness = NULL;
    m_GBuffer.Draw(m_meshMixList, m_meshMixSkinAnimList, &pTexTempZ, &pTexTempPos, &pTexTempNoral, &pTexTempThickness);

    MeshMixManager::SetSharedThicknessTexture(pTexTempThickness);

    DrawPass1(true);

    //---------------------------------------------------------------
    // ポストエフェクト
    // m_pRenderTarget1やm_pRenderTarget2に代入しないこと
    //---------------------------------------------------------------

    LPDIRECT3DTEXTURE9 pTempTexture = NULL;

    pTempTexture = m_pRenderTarget1;

    if (m_postEffectZShadowEnabled)
    {
        EnsurePostEffectZShadowInitialized();
        pTempTexture = m_postEffectZShadow.Draw(pTempTexture,
                                                pTexTempZ,
                                                pTexTempNoral,
                                                m_meshMixList,
                                                m_meshMixSkinAnimList);
    }

    if (m_postEffectSSAO2Enabled)
    {
        EnsurePostEffectSSAO2Initialized();
        pTempTexture = m_postEffectSSAO2.Draw(pTempTexture,
                                              pTexTempZ,
                                              pTexTempPos,
                                              pTexTempNoral,
                                              pTexTempThickness);
    }

    if (m_postEffectFogZEnabled)
    {
        EnsurePostEffectFogInitialized();
        pTempTexture = m_postEffectFog.Draw(pTempTexture,
                                            pTexTempZ,
                                            pTexTempPos,
                                            m_postEffectFogZEnabled,
                                            false);
    }

    if (m_postEffectFogHeightEnabled)
    {
        EnsurePostEffectHeightFogInitialized();
        pTempTexture = m_postEffectHeightFog.Draw(pTempTexture, pTexTempPos);
    }

    if (m_postEffectSaturateEnabled)
    {
        EnsurePostEffectSaturateInitialized();
        pTempTexture = m_postEffectSaturate.Draw(pTempTexture);
    }

    if (m_postEffectDepthOfFieldMode == DepthOfFieldMode::Enabled)
    {
        EnsurePostEffectDepthOfFieldInitialized();
        m_postEffectDepthOfField.SetBlend(1.0f);
        pTempTexture = m_postEffectDepthOfField.Draw(pTempTexture, pTexTempPos);
    }
    else if (m_postEffectDepthOfFieldMode == DepthOfFieldMode::AutoNear)
    {
        EnsurePostEffectDepthOfFieldInitialized();
        m_postEffectDepthOfField.UpdateAutoBlend(pTexTempPos);
        if (m_postEffectDepthOfField.GetBlend() > 0.001f)
        {
            pTempTexture = m_postEffectDepthOfField.Draw(pTempTexture, pTexTempPos);
        }
    }

    if (m_postEffectBloomEnabled)
    {
        EnsurePostEffectBloomInitialized();
        pTempTexture = m_PostEffectBloom.Draw(pTempTexture);
    }

    if (m_postEffectStarBurstEnabled)
    {
        EnsurePostEffectStarBurstInitialized();
        pTempTexture = m_postEffectStarBurst.Draw(pTempTexture);
    }

    if (m_postEffectGodRayEnabled)
    {
        EnsurePostEffectGodRayInitialized();
        pTempTexture = m_postEffectGodRay.Draw(pTempTexture, pTexTempZ);
    }

    if (m_postEffectGaussEnabled)
    {
        EnsurePostEffectGaussInitialized();
        pTempTexture = m_postEffectGauss.Draw(pTempTexture);
    }

    if (m_postEffectMaskedGaussEnabled)
    {
        EnsurePostEffectMaskedGaussInitialized();
        pTempTexture = m_postEffectMaskedGauss.Draw(pTempTexture);
    }

    if (m_postEffectMotionBlurCameraEnabled)
    {
        EnsurePostEffectMotionBlurCameraInitialized();
        pTempTexture = m_postEffectMotionBlurCamera.Draw(pTempTexture, pTexTempZ);
    }
    else
    {
        m_postEffectMotionBlurCamera.UpdateFrameMatrices();
    }

    if (m_postEffectFXAAEnabled)
    {
        EnsurePostEffectFXAAInitialized();
        pTempTexture = m_postEffectFXAA.Draw(pTempTexture);
    }

    // g_pRenderTargetの内容を画面に転送
    switch (m_debugGBufferView)
    {
    case DebugGBufferView::WorldPos:
        m_postEffectEnd.Draw(pTexTempPos);
        break;
    case DebugGBufferView::Normal:
        m_postEffectEnd.Draw(pTexTempNoral);
        break;
    case DebugGBufferView::Depth:
        m_postEffectEnd.DrawSingleChannel(pTexTempZ);
        break;
    case DebugGBufferView::Thickness:
        m_postEffectEnd.DrawSingleChannel(pTexTempThickness);
        break;
    default:
        m_postEffectEnd.Draw(pTempTexture);
        break;
    }

    // 文字と画像は彩度フィルタの影響を受けないようにする
    Draw2D();

    if (m_postEffectZShadowEnabled)
    {
        if (false)
        {
            m_postEffectZShadow.DrawDebugLightDepthOverlay(10, 10, 256, 256);
        }
    }

    hResult = Common::D3DDevice()->Present(NULL, NULL, NULL, NULL);
    assert(hResult == S_OK);

    m_windowManager.ChangeWindowMode();

}

void Render::ChangeResolution(const int W, const int H)
{
    m_windowManager.ChangeResolution(W, H);
}

void Render::ChangeWindowMode(const eWindowMode eWindowMode_)
{
    m_windowManager.RequestWindowMode(eWindowMode_);
    m_windowManager.ChangeWindowMode();
}

int Render::AddMesh(const std::wstring& filePath,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rot,
                    const float scale,
                    const float radius,
                    const float uvTile)
{
    m_meshList.push_back(MeshOld(filePath, pos, rot, scale, radius, uvTile));
    m_meshEnabledList.push_back(true);
    m_meshList.rbegin()->Initialize();

    return (int)m_meshList.size() - 1;
}

bool Render::RemoveMesh(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshEnabledList.size()))
    {
        return false;
    }

    m_meshEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshNoLighting(const std::wstring& filePath,
                              const D3DXVECTOR3& pos,
                              const D3DXVECTOR3& rot,
                              const float scale,
                              const float radius,
                              const float uvTile)
{
    m_meshList.push_back(MeshOld(L".\\MeshNoLighting.cso", filePath, pos, rot, scale, radius, uvTile));
    m_meshEnabledList.push_back(true);
    m_meshList.rbegin()->Initialize();

    return (int)m_meshList.size() - 1;
}

void Render::AddMeshSmooth(const std::wstring& filePath,
                                     const D3DXVECTOR3& pos,
                                     const D3DXVECTOR3& rot,
                                     const float scale,
                                     const float radius)
{
    MeshSmooth mesh;
    m_meshSmoothList.push_back(mesh);
    m_meshSmoothList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
}

void Render::AddMeshSSSLike(const std::wstring& filePath,
                                      const D3DXVECTOR3& pos,
                                      const D3DXVECTOR3& rot,
                                      const float scale,
                                      const float radius)
{
    MeshSSSLike mesh;
    m_meshSSSLikeList.push_back(mesh);
    m_meshSSSLikeList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
}

int Render::AddMeshSSS(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius)
{
    auto mesh = MeshSSS(filePath, pos, rot, scale, radius);
    m_meshSSSList.push_back(mesh);
    m_meshSSSEnabledList.push_back(true);
    m_meshSSSList.rbegin()->Initialize();
    return static_cast<int>(m_meshSSSList.size()) - 1;
}

bool Render::RemoveMeshSSS(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshSSSEnabledList.size()))
    {
        return false;
    }

    m_meshSSSEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshPointLight(const std::wstring& filePath,
                              const D3DXVECTOR3& pos,
                              const D3DXVECTOR3& rot,
                              const float scale,
                              const float radius)
{
    MeshPointLight mesh;
    m_meshPointLightList.push_back(mesh);
    m_meshPointLightEnabledList.push_back(true);
    m_meshPointLightList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
    return static_cast<int>(m_meshPointLightList.size()) - 1;
}

bool Render::RemoveMeshPointLight(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshPointLightEnabledList.size()))
    {
        return false;
    }

    m_meshPointLightEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshNormalMapping(const std::wstring& filePath,
                                 const std::wstring& normalMap,
                                 const D3DXVECTOR3& pos,
                                 const D3DXVECTOR3& rot,
                                 const float scale,
                                 const float radius)
{
    MeshNormalMapping mesh;
    m_meshNormalMapList.push_back(mesh);
    m_meshNormalMapEnabledList.push_back(true);
    m_meshNormalMapList.rbegin()->Initialize(filePath, normalMap, pos, rot, scale, radius);
    return static_cast<int>(m_meshNormalMapList.size()) - 1;
}

bool Render::RemoveMeshNormalMapping(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshNormalMapEnabledList.size()))
    {
        return false;
    }

    m_meshNormalMapEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshPOM(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius)
{
    MeshPOM mesh;
    m_meshPOMList.push_back(mesh);
    m_meshPOMEnabledList.push_back(true);
    m_meshPOMList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
    return static_cast<int>(m_meshPOMList.size()) - 1;
}

bool Render::RemoveMeshPOM(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshPOMEnabledList.size()))
    {
        return false;
    }

    m_meshPOMEnabledList.at(id) = false;
    return true;
}

int Render::AddAnimMesh(const std::wstring& filePath,
                        const D3DXVECTOR3& pos,
                        const D3DXVECTOR3& rot,
                        const float scale,
                        const AnimSetMap& animSetMap)
{
    AnimMesh* animMesh = NEW AnimMesh(filePath, pos, rot, scale, animSetMap);
    m_animMeshList.push_back(animMesh);
    return static_cast<int>(m_animMeshList.size()) - 1;
}

bool Render::RemoveAnimMesh(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_animMeshList.size()) || m_animMeshList.at(id) == nullptr)
    {
        return false;
    }

    SAFE_DELETE(m_animMeshList.at(id));
    return true;
}

int Render::AddSkinAnimMesh(const std::wstring& filePath,
                            const D3DXVECTOR3& pos,
                            const D3DXVECTOR3& rot,
                            const float scale,
                            const AnimSetMap& animSetMap)
{
    SkinAnimMesh* mesh = NEW SkinAnimMesh(filePath, pos, rot, scale, animSetMap);
    m_skinAnimMeshList.push_back(mesh);
    return static_cast<int>(m_skinAnimMeshList.size()) - 1;
}

bool Render::RemoveSkinAnimMesh(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_skinAnimMeshList.size()) || m_skinAnimMeshList.at(id) == nullptr)
    {
        return false;
    }

    SAFE_DELETE(m_skinAnimMeshList.at(id));
    return true;
}

int Render::AddMeshInstansing(const std::wstring& filePath,
                              const D3DXVECTOR3& pos,
                              const D3DXVECTOR3& rot,
                              const float scale)
{
    if (m_meshInstancingMap.find(filePath) == m_meshInstancingMap.end())
    {
        MeshInstancing* mesh = NEW MeshInstancing();
        mesh->Initialize();

        m_meshInstancingMap[filePath] = mesh;
    }

    m_meshInstancingMap[filePath]->AddInstance(pos);
    return 0;
}

bool Render::RemoveMeshInstancing(const std::wstring& filePath)
{
    const auto found = m_meshInstancingMap.find(filePath);
    if (found == m_meshInstancingMap.end())
    {
        return false;
    }

    SAFE_DELETE(found->second);
    m_meshInstancingMap.erase(found);
    return true;
}

int Render::AddMeshMix(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius,
                       const bool useParallaxOcclusionMapping,
                       const bool useNormalMapping,
                       const bool async)
{
    auto param = GetMeshParamPreset(eMeshParamPreset::GRASS);
    param.smooth = false;
    param.parallaxOcclusionMapping = useParallaxOcclusionMapping;
    param.normalMapping = useNormalMapping;
    param.saturateShadow = m_meshMixSaturateShadowEnabled;
    param.saturateShadowIntensity = m_meshMixSaturateShadowIntensity;
    param.shadowDarkness = m_meshMixShadowDarkness;
    param.specularIntensity = m_meshMixSpecularIntensity;
    param.specularEdge = m_meshMixSpecularEdge;
    param.specularIntensityOverrideEnabled = m_meshMixSpecularIntensityOverrideEnabled;
    param.specularEdgeOverrideEnabled = m_meshMixSpecularEdgeOverrideEnabled;
    param.sss = m_meshMixSSSEnabled;
    param.sssIntensity = m_meshMixSSSIntensity;
    param.sssColor = m_meshMixSSSColor;
    auto mesh = MeshMixManager(filePath, pos, rot, scale, param);
    m_meshMixList.push_back(std::move(mesh));
    m_meshMixList.rbegin()->Initialize(async);

    return static_cast<int>(m_meshMixList.size()) - 1;
}

bool Render::RemoveMeshMix(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixList.size()))
    {
        return false;
    }

    m_meshMixList.at(id).SetEnabled(false);
    return true;
}

void Render::SetMeshMixPos(const int id, const D3DXVECTOR3& pos)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixList.size()))
    {
        return;
    }

    m_meshMixList.at(id).SetPos(pos);
}

int Render::AddMeshMixSkinAnim(const std::wstring& filePath,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rot,
                               const float scale,
                               const AnimSetMap& animSetMap,
                               const float radius,
                               const bool useParallaxOcclusionMapping,
                               const bool useNormalMapping)
{
    auto param = GetMeshParamPreset(eMeshParamPreset::GRASS);
    param.smooth = false;
    param.parallaxOcclusionMapping = useParallaxOcclusionMapping;
    param.normalMapping = useNormalMapping;
    param.saturateShadow = m_meshMixSaturateShadowEnabled;
    param.saturateShadowIntensity = m_meshMixSaturateShadowIntensity;
    param.shadowDarkness = m_meshMixShadowDarkness;
    param.specularIntensity = m_meshMixSpecularIntensity;
    param.specularEdge = m_meshMixSpecularEdge;
    param.specularIntensityOverrideEnabled = m_meshMixSpecularIntensityOverrideEnabled;
    param.specularEdgeOverrideEnabled = m_meshMixSpecularEdgeOverrideEnabled;

    MeshMixSkinAnim* mesh = NEW MeshMixSkinAnim(filePath, pos, rot, scale, param, animSetMap);
    try
    {
        mesh->Initialize();
        m_meshMixSkinAnimList.push_back(mesh);
    }
    catch (...)
    {
        SAFE_DELETE(mesh);
        throw;
    }

    return static_cast<int>(m_meshMixSkinAnimList.size()) - 1;
}

bool Render::RemoveMeshMixSkinAnim(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixSkinAnimList.size()) || m_meshMixSkinAnimList.at(id) == nullptr)
    {
        return false;
    }

    m_meshMixSkinAnimList.at(id)->SetEnabled(false);
    return true;
}

void Render::SetMeshMixSaturateShadow(const bool enabled)
{
    m_meshMixSaturateShadowEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSaturateShadow(enabled);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSaturateShadow(enabled);
        }
    }
}

void Render::SetMeshMixSaturateShadowIntensity(const float intensity)
{
    m_meshMixSaturateShadowIntensity = intensity;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSaturateShadowIntensity(intensity);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSaturateShadowIntensity(intensity);
        }
    }
}

void Render::SetMeshMixShadowDarkness(const float darkness)
{
    m_meshMixShadowDarkness = darkness;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetShadowDarkness(darkness);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetShadowDarkness(darkness);
        }
    }
}

void Render::SetMeshMixSpecularIntensity(const float intensity)
{
    m_meshMixSpecularIntensity = intensity;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularIntensity(intensity);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularIntensity(intensity);
        }
    }
}

void Render::SetMeshMixSpecularEdge(const float edge)
{
    m_meshMixSpecularEdge = edge;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularEdge(edge);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularEdge(edge);
        }
    }
}

void Render::SetMeshMixSpecularIntensityOverrideEnabled(const bool enabled)
{
    m_meshMixSpecularIntensityOverrideEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularIntensityOverrideEnabled(enabled);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularIntensityOverrideEnabled(enabled);
        }
    }
}

void Render::SetMeshMixSpecularEdgeOverrideEnabled(const bool enabled)
{
    m_meshMixSpecularEdgeOverrideEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularEdgeOverrideEnabled(enabled);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularEdgeOverrideEnabled(enabled);
        }
    }
}

void Render::SetMeshMixSSS(const bool enabled)
{
    m_meshMixSSSEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSSS(enabled);
    }
}

void Render::SetMeshMixSSSIntensity(const float intensity)
{
    m_meshMixSSSIntensity = intensity;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSSSIntensity(intensity);
    }
}

void Render::SetMeshMixSSSColor(const DWORD color)
{
    m_meshMixSSSColor = color;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSSSColor(color);
    }
}

void Render::SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt)
{
    Camera::SetEyePos(pos);
    Camera::SetLookAtPos(lookAt);
}

void Render::MoveCamera(const D3DXVECTOR3& pos)
{
    auto eyePos = Camera::GetEyePos();
    Camera::SetEyePos(eyePos + pos);

    auto lookAtPos = Camera::GetLookAtPos();
    Camera::SetLookAtPos(lookAtPos + pos);
}

void Render::SetCameraClipPlanes(const float nearPlane, const float farPlane)
{
    Camera::SetClipPlanes(nearPlane, farPlane);
    const float positionRange = GBuffer::ComputePositionRange(nearPlane, farPlane);
    m_postEffectFog.SetFogDepthRange(nearPlane, farPlane);
    m_postEffectHeightFog.SetPositionRange(positionRange);
}

void Render::SetGBufferClipPlanes(const float nearPlane, const float farPlane)
{
    const float positionRange = GBuffer::ComputePositionRange(nearPlane, farPlane);
    m_GBuffer.SetDepthRange(nearPlane, farPlane);
    m_postEffectSSAO2.SetDepthRange(nearPlane, farPlane);
    m_postEffectFog.SetDepthDecodeRange(nearPlane, farPlane);
    m_postEffectGodRay.SetDepthRange(nearPlane, farPlane);
    m_postEffectDepthOfField.SetPositionRange(positionRange);
}

D3DXVECTOR3 Render::GetLookAtPos()
{
    return Camera::GetLookAtPos();
}

D3DXVECTOR3 Render::GetCameraPos()
{
    return Camera::GetEyePos();
}

D3DXVECTOR3 Render::GetCameraRotate()
{
    auto eyePos = Camera::GetEyePos();
    auto lookAtPos = Camera::GetLookAtPos();
    auto dir(lookAtPos - eyePos);
    D3DXVec3Normalize(&dir, &dir);
    return dir;
}

int Render::SetUpFont(const std::wstring& fontName,
                                const int fontSize,
                                const UINT fontColor)
{
    Font* font = NEW Font();
    font->Initialize(fontName, fontSize, fontColor);
    m_fontList.push_back(font);

    return (int)(m_fontList.size() - 1);
}

void Render::DrawText_(const int fontId,
                                 const std::wstring& text,
                                 const int X,
                                 const int Y)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddText(text, X, Y);
}

void Render::DrawText_(const int fontId,
                                 const std::wstring& text,
                                 const int X,
                                 const int Y,
                                 const UINT color)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddText(text, X, Y, color);
}

void Render::DrawTextCenter(const int fontId,
                                      const std::wstring& text,
                                      const int X,
                                      const int Y,
                                      const int Width,
                                      const int Height)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddTextCenter(text, X, Y, Width, Height);
}

void Render::DrawTextCenter(const int fontId,
                                      const std::wstring& text,
                                      const int X,
                                      const int Y,
                                      const int Width,
                                      const int Height,
                                      const UINT color)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddTextCenter(text, X, Y, Width, Height, color);
}

void Render::DrawImage(const std::wstring& text,
                                 const int X,
                                 const int Y,
                                 const int transparency)
{
    m_sprite.LoadImage_(text);
    m_sprite.PlaceImage(text, X, Y, transparency);
}

void Render::SetPostEffectSaturate(const float level)
{
    m_postEffectSaturateEnabled = (level < 0.9999f || level > 1.0001f);
    m_postEffectSaturate.SetPostEffectSaturate(level);
    if (m_postEffectSaturateEnabled)
    {
        EnsurePostEffectSaturateInitialized();
    }
}

void Render::SetPostEffectSaturateEnable(const bool arg)
{
    m_postEffectSaturateEnabled = arg;
    if (m_postEffectSaturateEnabled)
    {
        EnsurePostEffectSaturateInitialized();
    }
}

void Render::SetPostEffectGaussianFilter(const bool arg)
{
    m_postEffectGaussEnabled = arg;
    if (m_postEffectGaussEnabled)
    {
        EnsurePostEffectGaussInitialized();
    }
}

void Render::SetPostEffectGaussianSampleSize(const int sampleSize)
{
    m_gaussianSampleSize = NormalizeGaussianSampleSize(sampleSize);
    m_postEffectGauss.SetSampleSize(m_gaussianSampleSize);
    m_postEffectMaskedGauss.SetSampleSize(m_gaussianSampleSize);
}

void Render::SetPostEffectMaskedGaussianFilter(const bool arg)
{
    m_postEffectMaskedGaussEnabled = arg;
    if (m_postEffectMaskedGaussEnabled)
    {
        EnsurePostEffectMaskedGaussInitialized();
    }
}

void Render::SetPostEffectMaskedGaussianSampleSize(const int sampleSize)
{
    m_gaussianSampleSize = NormalizeGaussianSampleSize(sampleSize);
    m_postEffectMaskedGauss.SetSampleSize(m_gaussianSampleSize);
}

void Render::SetPostEffectMaskedGaussianMaskPath(const std::wstring& maskPath)
{
    m_maskedGaussianMaskPath = maskPath;
    m_postEffectMaskedGauss.SetMaskPath(maskPath);
}

void Render::SetPostEffectFXAA(const bool arg)
{
    m_postEffectFXAAEnabled = arg;
    if (m_postEffectFXAAEnabled)
    {
        EnsurePostEffectFXAAInitialized();
    }
}

void Render::SetPostEffectFXAAQuality(const int quality)
{
    m_fxaaQuality = NormalizeFXAAQuality(quality);
    m_postEffectFXAA.SetQuality(m_fxaaQuality);
}

void Render::SetPostEffectMotionBlurCamera(const bool arg)
{
    m_postEffectMotionBlurCameraEnabled = arg;
    if (m_postEffectMotionBlurCameraEnabled)
    {
        EnsurePostEffectMotionBlurCameraInitialized();
    }
}

void Render::SetPostEffectMotionBlurCameraQuality(const int quality)
{
    m_motionBlurCameraQuality = NormalizeMotionBlurCameraQuality(quality);
    m_postEffectMotionBlurCamera.SetQuality(m_motionBlurCameraQuality);
    m_motionBlurCameraMaxBlurPixels = m_postEffectMotionBlurCamera.GetMaxBlurPixels();
    m_motionBlurCameraSampleCount = m_postEffectMotionBlurCamera.GetSampleCount();
}

void Render::SetPostEffectMotionBlurCameraMaxBlurPixels(const float maxBlurPixels)
{
    m_motionBlurCameraMaxBlurPixels = NormalizeMotionBlurCameraMaxBlurPixels(maxBlurPixels);
    m_postEffectMotionBlurCamera.SetMaxBlurPixels(m_motionBlurCameraMaxBlurPixels);
}

void Render::SetPostEffectMotionBlurCameraSampleCount(const int sampleCount)
{
    m_motionBlurCameraSampleCount = NormalizeMotionBlurCameraSampleCount(sampleCount);
    m_postEffectMotionBlurCamera.SetSampleCount(m_motionBlurCameraSampleCount);
}

void Render::SetPostEffectDepthBufferShadow(const bool arg)
{
    m_postEffectZShadowEnabled = arg;
    if (m_postEffectZShadowEnabled)
    {
        EnsurePostEffectZShadowInitialized();
    }
}

void Render::SetPostEffectDepthBufferShadowIntensity(const float intensity)
{
    m_postEffectZShadow.SetShadowIntensity(intensity);
}

void Render::SetPostEffectDepthBufferShadowSaturationBoost(const float saturationBoost)
{
    m_postEffectZShadow.SetShadowSaturationBoost(saturationBoost);
}

void Render::SetPostEffectDepthBufferShadowCoverage(const float coverage)
{
    m_postEffectZShadow.SetCoverage(coverage);
}

void Render::SetPostEffectDepthBufferShadowPcfTapCount(const int tapCount)
{
    m_postEffectZShadow.SetPcfTapCount(tapCount);
}

void Render::SetPostEffectDepthBufferShadowCompositeTapCount(const int tapCount)
{
    m_postEffectZShadow.SetCompositeTapCount(tapCount);
}

void Render::EnsurePostEffectSaturateInitialized()
{
    m_postEffectSaturate.Initialize();
}

void Render::EnsurePostEffectGaussInitialized()
{
    m_postEffectGauss.Initialize();
}

void Render::EnsurePostEffectMaskedGaussInitialized()
{
    m_postEffectMaskedGauss.Initialize();
}

void Render::EnsurePostEffectFXAAInitialized()
{
    m_postEffectFXAA.Initialize();
}

void Render::EnsurePostEffectMotionBlurCameraInitialized()
{
    m_postEffectMotionBlurCamera.Initialize();
}

void Render::EnsurePostEffectZShadowInitialized()
{
    m_postEffectZShadow.Initialize();
}

void Render::EnsurePostEffectSSAO2Initialized()
{
    m_postEffectSSAO2.Initialize();
}

void Render::EnsurePostEffectFogInitialized()
{
    m_postEffectFog.Initialize();
}

void Render::EnsurePostEffectHeightFogInitialized()
{
    m_postEffectHeightFog.Initialize();
}

void Render::EnsurePostEffectBloomInitialized()
{
    m_PostEffectBloom.Initialize();
}

void Render::EnsurePostEffectDepthOfFieldInitialized()
{
    m_postEffectDepthOfField.Initialize();
}

void Render::EnsurePostEffectStarBurstInitialized()
{
    m_postEffectStarBurst.Initialize();
}

void Render::EnsurePostEffectGodRayInitialized()
{
    m_postEffectGodRay.Initialize();
}

void Render::SetPostEffectSSAO2(const bool arg)
{
    m_postEffectSSAO2Enabled = arg;
    if (m_postEffectSSAO2Enabled)
    {
        EnsurePostEffectSSAO2Initialized();
    }
}

void Render::SetPostEffectSSAO2Blur(const bool arg)
{
    m_postEffectSSAO2.SetBlurEnabled(arg);
}

void Render::SetPostEffectSSAO2ShadowStrength(const float shadowStrength)
{
    m_postEffectSSAO2.SetShadowStrength(shadowStrength);
}

void Render::SetPostEffectSSAO2SaturationBoost(const float saturationBoost)
{
    m_postEffectSSAO2.SetSaturationBoost(saturationBoost);
}

void Render::SetPostEffectSSAO2SampleCount(const int sampleCount)
{
    const int normalizedSampleCount = (std::max)(1, (std::min)(sampleCount, 64));
    m_postEffectSSAO2.SetSampleCount(normalizedSampleCount);
}

void Render::SetPostEffectSSAO2DepthScaledSampleDistance(const bool enabled)
{
    m_postEffectSSAO2.SetDepthScaledSampleDistanceEnabled(enabled);
}

void Render::SetPostEffectSSAO2SampleRadius(const float sampleRadius)
{
    m_postEffectSSAO2.SetSampleRadius(sampleRadius);
}

void Render::SetPostEffectFog(const bool arg)
{
    m_postEffectFogZEnabled = arg;
    if (m_postEffectFogZEnabled)
    {
        EnsurePostEffectFogInitialized();
    }
}

void Render::SetPostEffectFogIntensity(const float intensity)
{
    m_postEffectFog.SetIntensityZ(intensity);
}

void Render::SetPostEffectHeightFog(const bool arg)
{
    m_postEffectFogHeightEnabled = arg;
    if (m_postEffectFogHeightEnabled)
    {
        EnsurePostEffectHeightFogInitialized();
    }
}

void Render::SetPostEffectHeightFogIntensity(const float intensity)
{
    m_postEffectHeightFog.SetIntensity(intensity);
}

void Render::SetPostEffectHeightFogStart(const float start)
{
    m_postEffectHeightFog.SetStartHeight(start);
}

void Render::SetPostEffectHeightFogMax(const float maxHeight)
{
    m_postEffectHeightFog.SetMaxHeight(maxHeight);
}

void Render::SetPostEffectHeightFogDistanceStart(const float distanceStart)
{
    m_postEffectHeightFog.SetDistanceStart(distanceStart);
}

void Render::SetPostEffectHeightFogDistanceMax(const float distanceMax)
{
    m_postEffectHeightFog.SetDistanceMax(distanceMax);
}

void Render::SetPostEffectFogHeightEnable(const bool arg)
{
    SetPostEffectHeightFog(arg);
}

void Render::SetPostEffectFogHeightIntensity(const float intensity)
{
    SetPostEffectHeightFogIntensity(intensity);
}

void Render::SetPostEffectFogHeightStart(const float start)
{
    SetPostEffectHeightFogStart(start);
}

void Render::SetPostEffectBloom(const bool arg)
{
    m_postEffectBloomEnabled = arg;
    if (m_postEffectBloomEnabled)
    {
        EnsurePostEffectBloomInitialized();
    }
}

void Render::SetPostEffectBloomThreshold(const float threshold)
{
    m_PostEffectBloom.SetThreshold(threshold);
}

void Render::SetPostEffectDepthOfField(const bool arg)
{
    if (arg)
    {
        SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Enabled);
    }
    else
    {
        SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
    }
}

void Render::SetPostEffectDepthOfFieldMode(const DepthOfFieldMode mode)
{
    m_postEffectDepthOfFieldMode = mode;
    if (mode != DepthOfFieldMode::Disabled)
    {
        EnsurePostEffectDepthOfFieldInitialized();
    }
    if (mode == DepthOfFieldMode::Disabled)
    {
        m_postEffectDepthOfField.SetBlend(0.0f);
    }
    else if (mode == DepthOfFieldMode::Enabled)
    {
        m_postEffectDepthOfField.SetBlend(1.0f);
    }
}

void Render::SetPostEffectDepthOfFieldFocalDistance(const float distance)
{
    m_postEffectDepthOfField.SetFocalDistance(distance);
}

void Render::SetPostEffectDepthOfFieldMaxBlurDistance(const float distance)
{
    m_postEffectDepthOfField.SetMaxBlurDistance(distance);
}

void Render::SetPostEffectDepthOfFieldAutoActivationDistance(const float distance)
{
    m_postEffectDepthOfField.SetAutoActivationDistance(distance);
}

void Render::SetPostEffectStarBurst(const bool arg)
{
    m_postEffectStarBurstEnabled = arg;
    if (m_postEffectStarBurstEnabled)
    {
        EnsurePostEffectStarBurstInitialized();
    }
}

void Render::SetPostEffectStarBurstThreshold(const float threshold)
{
    m_postEffectStarBurst.SetThreshold(threshold);
}

void Render::SetPostEffectGodRay(const bool arg)
{
    m_postEffectGodRayEnabled = arg;
    if (m_postEffectGodRayEnabled)
    {
        EnsurePostEffectGodRayInitialized();
    }
}

void Render::SetPostEffectGodRayLightPos(const D3DXVECTOR3& pos)
{
    m_postEffectGodRay.SetLightPos(pos);
}

void Render::SetPostEffectGodRayReverseSampling(const bool arg)
{
    m_postEffectGodRay.SetReverseSampling(arg);
}

void Render::SetPostEffectGodRayRayLength(const float arg)
{
    m_postEffectGodRay.SetRayLength(arg);
}

void Render::SetPostEffectGodRayIntensity(const float arg)
{
    m_postEffectGodRay.SetRayIntensity(arg);
}

void Render::SetPostEffectGodRayVirtualProximityStrength(const float arg)
{
    m_postEffectGodRay.SetVirtualProximityStrength(arg);
}

void Render::SetPostEffectGodRayOcclusionFalloff(const float arg)
{
    m_postEffectGodRay.SetOcclusionFalloff(arg);
}

void Render::SetPostEffectGodRayLightColor(const D3DXVECTOR3& color)
{
    m_postEffectGodRay.SetLightColor(color);
}

void Render::SetDebugGBufferView(const DebugGBufferView view)
{
    m_debugGBufferView = view;
}

void Render::SetShowFPS(const bool arg)
{
    m_bShowFPS = arg;
}

std::vector<std::pair<int, int>> Render::GetResolutionList()
{
    return m_windowManager.GetResolutionList();
}

void Render::SetLightDir(const D3DXVECTOR3& dir)
{
    D3DXVECTOR4 normal(dir, 0.f);
    D3DXVec4Normalize(&normal, &normal);
    Light::SetLightDir(normal);
}

void Render::SetLightColor(const D3DXCOLOR& color)
{
    Light::SetLightColor(color);
}

void Render::SetLightBrightness(const float brightness)
{
    Light::SetBrightness(brightness);
}

void Render::SetAmbientLightColor(const D3DXCOLOR& color)
{
    Light::SetAmbientColor(color);
}

void Render::SetAmbientLightBrightness(const float brightness)
{
    Light::SetAmbientBrightness(brightness);
}

void Render::AddPointLight(const D3DXVECTOR3& pos,
                           const float brightness,
                           const D3DXCOLOR color,
                           const PointLightShape shape,
                           const float lineLength,
                           const float squareWidth,
                           const float squareHeight,
                           const D3DXVECTOR3& rotation)
{
    Light::AddPointLight(pos, color, brightness, shape, lineLength, squareWidth, squareHeight, rotation);
}

void Render::RotateCamera(const D3DXVECTOR3& rot)
{
    D3DXVECTOR3 lookAt = Camera::GetLookAtPos();
    D3DXVECTOR3 eye = Camera::GetEyePos();

    // 注視点から見た相対位置
    D3DXVECTOR3 rel = eye - lookAt;

    // 現在の距離
    float r = D3DXVec3Length(&rel);

    // 現在の角度を求める（spherical座標）

    // 水平方向
    float yaw = atan2f(rel.x, rel.z);

    // 上下方向
    float pitch = asinf(rel.y / r);

    // 回転を加える
    yaw += rot.y;
    pitch += rot.x;

    //---------------------------------------------------------
    // ピッチ角を制限する
    //---------------------------------------------------------

    // 真上/真下を少し手前で止める
    const float limit = D3DXToRadian(89.0f);
    if (pitch > limit)
    {
        pitch = limit;
    }

    if (pitch < -limit)
    {
        pitch = -limit;
    }

    // 極座標 → デカルト座標に戻す
    D3DXVECTOR3 newRel;
    newRel.x = r * cosf(pitch) * sinf(yaw);
    newRel.y = r * sinf(pitch);
    newRel.z = r * cosf(pitch) * cosf(yaw);

    // 新しいeye位置をセット
    D3DXVECTOR3 newEye = lookAt + newRel;

    Camera::SetEyePos(newEye);
    Camera::SetLookAtPos(lookAt);
}

void Render::DrawPass1(const bool renderToSceneRenderTargets)
{
    HRESULT hResult = E_FAIL;

    LPDIRECT3DSURFACE9 surfaceOld = NULL;
    LPDIRECT3DSURFACE9 surfaceRenderTarget0 = NULL;
    LPDIRECT3DSURFACE9 surfaceRenderTarget1 = NULL;

    if (renderToSceneRenderTargets)
    {
        hResult = Common::D3DDevice()->GetRenderTarget(0, &surfaceOld);
        assert(hResult == S_OK);

        hResult = m_pRenderTarget1->GetSurfaceLevel(0, &surfaceRenderTarget0);
        assert(hResult == S_OK);

        hResult = m_pRenderTarget2->GetSurfaceLevel(0, &surfaceRenderTarget1);
        assert(hResult == S_OK);

        hResult = Common::D3DDevice()->SetRenderTarget(0, surfaceRenderTarget0);
        assert(hResult == S_OK);

        hResult = Common::D3DDevice()->SetRenderTarget(1, surfaceRenderTarget1);
        assert(hResult == S_OK);
    }
    else
    {
        hResult = Common::D3DDevice()->SetRenderTarget(1, NULL);
        assert(hResult == S_OK);
    }

    hResult = Common::D3DDevice()->Clear(0,
                                         NULL,
                                         D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                         D3DCOLOR_RGBA(200, 200, 200, 255),
                                         1.0f,
                                         0);

    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->BeginScene();
    assert(hResult == S_OK);

    for (size_t i = 0; i < m_meshList.size(); ++i)
    {
        if (i < m_meshEnabledList.size() && m_meshEnabledList[i])
        {
            m_meshList[i].Render();
        }
    }

    for (auto& elem : m_meshSmoothList)
    {
        elem.Draw();
    }

    for (auto& elem : m_meshSSSLikeList)
    {
        elem.Draw();
    }

    for (size_t i = 0; i < m_meshSSSList.size(); ++i)
    {
        if (i < m_meshSSSEnabledList.size() && m_meshSSSEnabledList[i])
        {
            m_meshSSSList[i].Render();
        }
    }

    for (size_t i = 0; i < m_meshPointLightList.size(); ++i)
    {
        if (i < m_meshPointLightEnabledList.size() && m_meshPointLightEnabledList[i])
        {
            m_meshPointLightList[i].Draw();
        }
    }

    for (size_t i = 0; i < m_meshNormalMapList.size(); ++i)
    {
        if (i < m_meshNormalMapEnabledList.size() && m_meshNormalMapEnabledList[i])
        {
            m_meshNormalMapList[i].Draw();
        }
    }

    for (size_t i = 0; i < m_meshPOMList.size(); ++i)
    {
        if (i < m_meshPOMEnabledList.size() && m_meshPOMEnabledList[i])
        {
            m_meshPOMList[i].Draw();
        }
    }

    for (auto& elem : m_animMeshList)
    {
        if (elem != nullptr)
        {
            elem->Render();
        }
    }

    for (auto& elem : m_skinAnimMeshList)
    {
        if (elem != nullptr)
        {
            elem->Render(Camera::GetViewMatrix(),
                         Camera::GetProjMatrix(),
                         Light::GetLightDir(),
                         Light::GetBrightness());
        }
    }

    for (auto& elem : m_meshMixSkinAnimList)
    {
        if (elem != nullptr)
        {
            elem->Render();
        }
    }

    for (auto& elem : m_meshInstancingMap)
    {
        elem.second->Draw();
    }

    for (auto& elem : m_meshMixList)
    {
        elem.Render();
    }

    hResult = Common::D3DDevice()->EndScene();
    assert(hResult == S_OK);

    if (renderToSceneRenderTargets)
    {
        hResult = Common::D3DDevice()->SetRenderTarget(1, NULL);
        assert(hResult == S_OK);

        hResult = Common::D3DDevice()->SetRenderTarget(0, surfaceOld);
        assert(hResult == S_OK);
    }

    SAFE_RELEASE(surfaceRenderTarget0);
    SAFE_RELEASE(surfaceRenderTarget1);
    SAFE_RELEASE(surfaceOld);
}

float Render::CalcFPS()
{
    //--------------------------------------------------------
    // 毎フレーム、現在時刻を記録し、リストの末尾に追加する。
    // 先頭ほど古い時刻が記録される。
    // リストの先頭から、記録された時刻と現在時刻の差を比較していくと、
    // 最初は1秒以上の差があるが、やがて1秒以下の要素が見つかる。（Aとする）
    // そのときの、A以降の要素の総数がFPSである
    //--------------------------------------------------------

    using ClockType = std::chrono::steady_clock;

    const int timeRecordCapacity = 300;

    ClockType::time_point nowTime = ClockType::now();

    m_vecTime.push_back(nowTime);

    const int overflowCount = (int)m_vecTime.size() - timeRecordCapacity;
    if (overflowCount > 0)
    {
        m_vecTime.erase(m_vecTime.begin(), m_vecTime.begin() + overflowCount);
    }

    ClockType::time_point oneSecondAgo = nowTime - std::chrono::seconds(1);

    auto windowBegin = std::lower_bound(m_vecTime.begin(), m_vecTime.end(), oneSecondAgo);
    int framesInWindow = (int)std::distance(windowBegin, m_vecTime.end());

    if (framesInWindow <= 1)
    {
        return 0.0f;
    }

    double secondsSpan = std::chrono::duration<double>(m_vecTime.back() - *windowBegin).count();
    if (secondsSpan <= 0.0)
    {
        return 0.0f;
    }

    float fpsFloat = (float)((framesInWindow - 1) / secondsSpan);
    return fpsFloat;
}

void Render::ShowFPS(const float arg)
{
    if (m_fontID == -1)
    {
        m_fontID = SetUpFont(L"BIZ UDゴシック",
                             20,
                             D3DCOLOR_RGBA(0, 255, 0, 255));
    }

    wchar_t buffer[64];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", arg);

    std::wstring fps(buffer);

    DrawText_(m_fontID, fps, 10, 10);
}

void Render::Draw2D()
{
    for (auto& elem : m_fontList)
    {
        elem->Draw();
    }

    m_sprite.Draw();

}

void Render::OnDeviceLost()
{
    SAFE_RELEASE(m_pRenderTarget1);
    SAFE_RELEASE(m_pRenderTarget2);
    m_sprite.OnDeviceLost();
}

void Render::OnDeviceReset()
{
    CreateTexture();
    m_sprite.OnDeviceReset();
}

void Render::CreateTexture()
{
    HRESULT hr = E_FAIL;

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A16B16G16R16F,
                           //D3DFMT_A8R8G8B8,
                           D3DPOOL_DEFAULT,
                           &m_pRenderTarget1);
    assert(hr == S_OK);

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A8R8G8B8,
                           D3DPOOL_DEFAULT,
                           &m_pRenderTarget2);
    assert(hr == S_OK);

}

}
