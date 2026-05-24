#include "AppState.h"

#include <algorithm>
#include <cmath>
#include <cwchar>

#include "SettingsDialog.h"

namespace
{
float ClampSaturateLevel(const float level)
{
    return (std::max)(SATURATE_MIN, (std::min)(level, SATURATE_MAX));
}

float ClampFogIntensity(const float intensity)
{
    return (std::max)(FOG_INTENSITY_MIN, (std::min)(intensity, FOG_INTENSITY_MAX));
}

float ClampFogColor(const float value)
{
    return (std::max)(FOG_COLOR_MIN, (std::min)(value, FOG_COLOR_MAX));
}

float ClampHeightFogIntensity(const float intensity)
{
    return (std::max)(HEIGHT_FOG_INTENSITY_MIN, (std::min)(intensity, HEIGHT_FOG_INTENSITY_MAX));
}

float ClampHeightFogHeight(const float height)
{
    return (std::max)(HEIGHT_FOG_HEIGHT_MIN, (std::min)(height, HEIGHT_FOG_HEIGHT_MAX));
}

float ClampHeightFogDistance(const float distance)
{
    return (std::max)(HEIGHT_FOG_DISTANCE_MIN, (std::min)(distance, HEIGHT_FOG_DISTANCE_MAX));
}

float ClampSunLightIntensity(const float intensity)
{
    return (std::max)(SUN_LIGHT_INTENSITY_MIN, (std::min)(intensity, SUN_LIGHT_INTENSITY_MAX));
}

float ClampSunLightColor(const float value)
{
    return (std::max)(SUN_LIGHT_COLOR_MIN, (std::min)(value, SUN_LIGHT_COLOR_MAX));
}

float ClampAmbientLightIntensity(const float intensity)
{
    return (std::max)(AMBIENT_LIGHT_INTENSITY_MIN, (std::min)(intensity, AMBIENT_LIGHT_INTENSITY_MAX));
}

float ClampAmbientLightColor(const float value)
{
    return (std::max)(AMBIENT_LIGHT_COLOR_MIN, (std::min)(value, AMBIENT_LIGHT_COLOR_MAX));
}

float ClampShadowIntensity(const float intensity)
{
    return (std::max)(SHADOW_INTENSITY_MIN, (std::min)(intensity, SHADOW_INTENSITY_MAX));
}

float ClampShadowCoverage(const float coverage)
{
    return (std::max)(SHADOW_COVERAGE_MIN, (std::min)(coverage, SHADOW_COVERAGE_MAX));
}

float ClampShadowSaturationBoost(const float boost)
{
    return (std::max)(SHADOW_SATURATION_BOOST_MIN, (std::min)(boost, SHADOW_SATURATION_BOOST_MAX));
}

float ClampSSAOShadowStrength(const float shadowStrength)
{
    return (std::max)(SSAO_SHADOW_STRENGTH_MIN,
                      (std::min)(shadowStrength, SSAO_SHADOW_STRENGTH_MAX));
}

float ClampSSAOShadowSaturationBoost(const float boost)
{
    return (std::max)(SSAO_SHADOW_SATURATION_BOOST_MIN,
                      (std::min)(boost, SSAO_SHADOW_SATURATION_BOOST_MAX));
}

int ClampSSAOSampleCount(const int sampleCount)
{
    if (sampleCount <= 6)
    {
        return 4;
    }

    if (sampleCount <= 12)
    {
        return 8;
    }

    if (sampleCount <= 24)
    {
        return 16;
    }

    if (sampleCount <= 48)
    {
        return 32;
    }

    return 64;
}

float ClampSSAOSampleRadius(const float sampleRadius)
{
    return (std::max)(SSAO_SAMPLE_RADIUS_MIN, (std::min)(sampleRadius, SSAO_SAMPLE_RADIUS_MAX));
}

int ClampSSAOBlurKernelSize(const int kernelSize)
{
    if (kernelSize <= 4)
    {
        return 3;
    }

    if (kernelSize <= 8)
    {
        return 5;
    }

    if (kernelSize <= 16)
    {
        return 11;
    }

    return 21;
}

float ClampCameraNearPlane(const float nearPlane)
{
    return (std::max)(CAMERA_NEAR_MIN, (std::min)(nearPlane, CAMERA_NEAR_MAX));
}

float ClampCameraFarPlane(const float farPlane)
{
    return (std::max)(CAMERA_FAR_MIN, (std::min)(farPlane, CAMERA_FAR_MAX));
}

float ClampGBufferNearPlane(const float nearPlane)
{
    return (std::max)(GBUFFER_NEAR_MIN, (std::min)(nearPlane, GBUFFER_NEAR_MAX));
}

float ClampGBufferFarPlane(const float farPlane)
{
    return (std::max)(GBUFFER_FAR_MIN, (std::min)(farPlane, GBUFFER_FAR_MAX));
}

float ClampHalfLambertShadowSaturation(const float boost)
{
    return (std::max)(HALF_LAMBERT_SHADOW_SATURATION_MIN,
                      (std::min)(boost, HALF_LAMBERT_SHADOW_SATURATION_MAX));
}

float ClampShadowDarkness(const float darkness)
{
    return (std::max)(SHADOW_DARKNESS_MIN, (std::min)(darkness, SHADOW_DARKNESS_MAX));
}

float ClampSpecularIntensity(const float intensity)
{
    return (std::max)(SPECULAR_INTENSITY_MIN, (std::min)(intensity, SPECULAR_INTENSITY_MAX));
}

float ClampSpecularEdge(const float edge)
{
    return (std::max)(SPECULAR_EDGE_MIN, (std::min)(edge, SPECULAR_EDGE_MAX));
}

float ClampFresnelIntensity(const float intensity)
{
    return (std::max)(FRESNEL_INTENSITY_MIN, (std::min)(intensity, FRESNEL_INTENSITY_MAX));
}

float ClampEnvMapBlend(const float blend)
{
    return (std::max)(ENV_MAP_BLEND_MIN, (std::min)(blend, ENV_MAP_BLEND_MAX));
}

float ClampTAAHistoryWeight(const float historyWeight)
{
    return (std::max)(TAA_HISTORY_WEIGHT_MIN, (std::min)(historyWeight, TAA_HISTORY_WEIGHT_MAX));
}

float ClampPBRRoughness(const float roughness)
{
    return (std::max)(PBR_ROUGHNESS_MIN, (std::min)(roughness, PBR_ROUGHNESS_MAX));
}

float ClampPBRMetallic(const float metallic)
{
    return (std::max)(PBR_METALLIC_MIN, (std::min)(metallic, PBR_METALLIC_MAX));
}

float ClampPBREnvReflectionIntensity(const float intensity)
{
    return (std::max)(PBR_ENV_REFLECTION_INTENSITY_MIN,
                      (std::min)(intensity, PBR_ENV_REFLECTION_INTENSITY_MAX));
}

float ClampPBREnvMaxMipLevel(const float mipLevel)
{
    return (std::max)(PBR_ENV_MAX_MIP_LEVEL_MIN,
                      (std::min)(mipLevel, PBR_ENV_MAX_MIP_LEVEL_MAX));
}

float ClampPBREnvDiffuseIntensity(const float intensity)
{
    return (std::max)(PBR_ENV_DIFFUSE_INTENSITY_MIN,
                      (std::min)(intensity, PBR_ENV_DIFFUSE_INTENSITY_MAX));
}

float ClampPBREnvDiffuseMipLevel(const float mipLevel)
{
    return (std::max)(PBR_ENV_DIFFUSE_MIP_LEVEL_MIN,
                      (std::min)(mipLevel, PBR_ENV_DIFFUSE_MIP_LEVEL_MAX));
}

float ClampSSSIntensity(const float intensity)
{
    return (std::max)(SSS_INTENSITY_MIN, (std::min)(intensity, SSS_INTENSITY_MAX));
}

float ClampSSSColor(const float value)
{
    return (std::max)(SSS_COLOR_MIN, (std::min)(value, SSS_COLOR_MAX));
}

DWORD SSSColorToDWORD(const D3DXCOLOR& color)
{
    const DWORD r = static_cast<DWORD>(std::lround(ClampSSSColor(color.r) * 255.0f));
    const DWORD g = static_cast<DWORD>(std::lround(ClampSSSColor(color.g) * 255.0f));
    const DWORD b = static_cast<DWORD>(std::lround(ClampSSSColor(color.b) * 255.0f));
    return (r << 16) | (g << 8) | b;
}

float ClampBloomThreshold(const float threshold)
{
    return (std::max)(BLOOM_THRESHOLD_MIN, (std::min)(threshold, BLOOM_THRESHOLD_MAX));
}

float ClampBloomWeightSum(const float weightSum)
{
    return (std::max)(BLOOM_WEIGHT_SUM_MIN, (std::min)(weightSum, BLOOM_WEIGHT_SUM_MAX));
}

float ClampDepthOfFieldFocalDistance(const float distance)
{
    return (std::max)(DOF_FOCAL_DISTANCE_MIN, (std::min)(distance, DOF_FOCAL_DISTANCE_MAX));
}

float ClampDepthOfFieldStartNear(const float distance)
{
    return (std::max)(DOF_START_NEAR_MIN, (std::min)(distance, DOF_START_NEAR_MAX));
}

float ClampDepthOfFieldMaxBlurDistance(const float distance)
{
    return (std::max)(DOF_MAX_BLUR_DISTANCE_MIN, (std::min)(distance, DOF_MAX_BLUR_DISTANCE_MAX));
}

float ClampDepthOfFieldAutoActivationDistance(const float distance)
{
    return (std::max)(DOF_AUTO_ACTIVATION_DISTANCE_MIN,
                      (std::min)(distance, DOF_AUTO_ACTIVATION_DISTANCE_MAX));
}

float ClampStarBurstThreshold(const float threshold)
{
    return (std::max)(STARBURST_THRESHOLD_MIN, (std::min)(threshold, STARBURST_THRESHOLD_MAX));
}

float ClampStarBurstDistanceFade(const float fade)
{
    return (std::max)(STARBURST_DISTANCE_FADE_MIN, (std::min)(fade, STARBURST_DISTANCE_FADE_MAX));
}

float ClampModelLoadScale(const float scale)
{
    return (std::max)(MODEL_LOAD_SCALE_MIN, (std::min)(scale, MODEL_LOAD_SCALE_MAX));
}

float ClampPointLightColor(const float value)
{
    return (std::max)(POINT_LIGHT_COLOR_MIN, (std::min)(value, POINT_LIGHT_COLOR_MAX));
}

float ClampPointLightBrightness(const float brightness)
{
    return (std::max)(POINT_LIGHT_BRIGHTNESS_MIN, (std::min)(brightness, POINT_LIGHT_BRIGHTNESS_MAX));
}

float ClampPointLightLineLength(const float lineLength)
{
    return (std::max)(POINT_LIGHT_LINE_LENGTH_MIN, (std::min)(lineLength, POINT_LIGHT_LINE_LENGTH_MAX));
}

float ClampPointLightSquareSize(const float size)
{
    return (std::max)(POINT_LIGHT_SQUARE_SIZE_MIN, (std::min)(size, POINT_LIGHT_SQUARE_SIZE_MAX));
}

float ClampPointLightRotationDegrees(const float degrees)
{
    return (std::max)(POINT_LIGHT_ROTATION_MIN_DEGREES,
                      (std::min)(degrees, POINT_LIGHT_ROTATION_MAX_DEGREES));
}

int NormalizeGaussianSampleSizeLocal(const int sampleSize)
{
    int normalized = (std::max)(GAUSSIAN_SAMPLE_MIN, (std::min)(sampleSize, GAUSSIAN_SAMPLE_MAX));
    if ((normalized % 2) == 0)
    {
        --normalized;
    }
    return (std::max)(GAUSSIAN_SAMPLE_MIN, normalized);
}

int NormalizeFontExGaussianSampleSizeLocal(const int sampleSize)
{
    return (std::max)(FONTEX_GAUSSIAN_SAMPLE_MIN, (std::min)(sampleSize, FONTEX_GAUSSIAN_SAMPLE_MAX));
}

float NormalizeSSGISampleRadiusLocal(const float sampleRadius)
{
    return (std::max)(SSGI_SAMPLE_RADIUS_MIN, (std::min)(sampleRadius, SSGI_SAMPLE_RADIUS_MAX));
}

int NormalizeFXAAQualityLocal(const int quality)
{
    return (std::max)(FXAA_QUALITY_MIN, (std::min)(quality, FXAA_QUALITY_MAX));
}

int NormalizeMotionBlurCameraQualityLocal(const int quality)
{
    return (std::max)(MOTION_BLUR_CAMERA_QUALITY_MIN, (std::min)(quality, MOTION_BLUR_CAMERA_QUALITY_MAX));
}

float NormalizeMotionBlurCameraMaxBlurPixelsLocal(const float maxBlurPixels)
{
    return (std::max)(MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_MIN,
                      (std::min)(maxBlurPixels, MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_MAX));
}

int NormalizeMotionBlurCameraSampleCountLocal(const int sampleCount)
{
    return (std::max)(MOTION_BLUR_CAMERA_SAMPLE_COUNT_MIN,
                      (std::min)(sampleCount, MOTION_BLUR_CAMERA_SAMPLE_COUNT_MAX));
}

int NormalizeShadowBlurTapCountLocal(const int tapCount)
{
    int normalized = (std::max)(SHADOW_BLUR_TAP_COUNT_MIN, (std::min)(tapCount, SHADOW_BLUR_TAP_COUNT_MAX));
    if ((normalized % 2) == 0)
    {
        --normalized;
    }
    return (std::max)(SHADOW_BLUR_TAP_COUNT_MIN, normalized);
}

bool IsWeekdayBusinessHours()
{
    SYSTEMTIME localTime { };
    GetLocalTime(&localTime);

    const bool isWeekday = localTime.wDayOfWeek >= 1 && localTime.wDayOfWeek <= 5;
    const bool isBusinessHours = localTime.wHour >= 8 && localTime.wHour < 19;
    return isWeekday && isBusinessHours;
}

static const std::wstring GODRAY_MARKER_PATH = L"..\\..\\Sample\\res\\model2\\cube.x";

D3DXVECTOR3 GetEffectiveGodRayLightPos()
{
    const D3DXVECTOR3 cameraPos = g_Render.GetCameraPos();
    const D3DXVECTOR3 toLight = g_godRayLightPos - cameraPos;
    return cameraPos - toLight;
}

bool IsGodRayLightBehindCamera()
{
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const D3DXVECTOR3 cameraPos = g_Render.GetCameraPos();
    D3DXVECTOR3 toLight = g_godRayLightPos - cameraPos;
    if (D3DXVec3LengthSq(&toLight) <= 0.000001f)
    {
        return false;
    }
    D3DXVec3Normalize(&toLight, &toLight);

    return D3DXVec3Dot(&forward, &toLight) < 0.0f;
}

D3DXVECTOR3 GetGodRaySourceMarkerPos()
{
    return g_godRayLightPos + D3DXVECTOR3(0.0f, 1.0f, 0.0f);
}

D3DXVECTOR3 GetGodRayEffectiveMarkerPos()
{
    return GetEffectiveGodRayLightPos() + D3DXVECTOR3(0.0f, 1.0f, 0.0f);
}

int AddGodRayMarker(const D3DXVECTOR3& pos)
{
    return g_Render.AddMeshMix(GODRAY_MARKER_PATH,
                               pos,
                               D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                               1.0f,
                               1.0f,
                               false,
                               false);
}
}

void ApplySaturateLevel()
{
    g_saturateLevel = ClampSaturateLevel(g_saturateLevel);
    g_Render.SetPostEffectSaturate(g_saturateLevel);
}

void ApplyPostEffectToggleSettings()
{
    g_Render.SetPostEffectSSAO(g_bSSAO);
    g_Render.SetPostEffectSSGI(g_bSSGI);
    ApplySSAOBlur();
    ApplySSGIBlur();
    g_Render.SetPostEffectFog(g_bFog);
    g_Render.SetPostEffectHeightFog(g_bHeightFog);
    g_Render.SetPostEffectSaturateEnable(g_bSaturateFilter);
    g_Render.SetPostEffectGaussianFilter(g_bGaussianFilter);
    g_Render.SetPostEffectMaskedGaussianFilter(g_bMaskedGaussianFilter);
    g_Render.SetPostEffectAA(g_bPostEffectAA);
    g_Render.SetPostEffectFXAA(g_bFXAA);
    g_Render.SetPostEffectTAA(g_bTAA);
    ApplyTAAHistoryWeight();
    g_Render.SetPostEffectMotionBlurCamera(g_bMotionBlurCamera);
    g_Render.SetMeshMixSSS(g_bSSS);
    g_Render.SetPostEffectBloom(g_bBloom);
    ApplyHalo();
    ApplyDepthOfFieldMode();
    g_Render.SetPostEffectStarBurst(g_bStarBurst);
    ApplyGodRay();
}

void ApplyMaskedGaussianMaskPath()
{
    g_Render.SetPostEffectMaskedGaussianMaskPath(g_selectedMaskedGaussianMaskPath);
    RefreshSettingsDialogState();
}

void ApplyFogIntensity()
{
    g_fogIntensity = ClampFogIntensity(g_fogIntensity);
    g_Render.SetPostEffectFogIntensity(g_fogIntensity);
}

void ApplyFogColor()
{
    g_fogColor.r = ClampFogColor(g_fogColor.r);
    g_fogColor.g = ClampFogColor(g_fogColor.g);
    g_fogColor.b = ClampFogColor(g_fogColor.b);
    g_fogColor.a = 1.0f;
    g_Render.SetPostEffectFogColor(g_fogColor);
}

void ApplyHeightFogIntensity()
{
    g_heightFogIntensity = ClampHeightFogIntensity(g_heightFogIntensity);
    g_Render.SetPostEffectHeightFogIntensity(g_heightFogIntensity);
}

void ApplyHeightFogStart()
{
    g_heightFogStart = ClampHeightFogHeight(g_heightFogStart);
    g_Render.SetPostEffectHeightFogStart(g_heightFogStart);
}

void ApplyHeightFogMax()
{
    g_heightFogMax = ClampHeightFogHeight(g_heightFogMax);
    g_Render.SetPostEffectHeightFogMax(g_heightFogMax);
}

void ApplyHeightFogDistanceStart()
{
    g_heightFogDistanceStart = ClampHeightFogDistance(g_heightFogDistanceStart);
    g_Render.SetPostEffectHeightFogDistanceStart(g_heightFogDistanceStart);
}

void ApplyHeightFogDistanceMax()
{
    g_heightFogDistanceMax = ClampHeightFogDistance(g_heightFogDistanceMax);
    g_Render.SetPostEffectHeightFogDistanceMax(g_heightFogDistanceMax);
}

void ApplySunLightIntensity()
{
    g_sunLightIntensity = ClampSunLightIntensity(g_sunLightIntensity);
    g_Render.SetLightBrightness(g_sunLightIntensity);
}

void ApplySunLightColor()
{
    g_sunLightColor.r = ClampSunLightColor(g_sunLightColor.r);
    g_sunLightColor.g = ClampSunLightColor(g_sunLightColor.g);
    g_sunLightColor.b = ClampSunLightColor(g_sunLightColor.b);
    g_Render.SetLightColor(g_sunLightColor);
}

void ApplyAmbientLightIntensity()
{
    g_ambientLightIntensity = ClampAmbientLightIntensity(g_ambientLightIntensity);
    g_Render.SetAmbientLightBrightness(g_ambientLightIntensity);
}

void ApplyAmbientLightColor()
{
    g_ambientLightColor.r = ClampAmbientLightColor(g_ambientLightColor.r);
    g_ambientLightColor.g = ClampAmbientLightColor(g_ambientLightColor.g);
    g_ambientLightColor.b = ClampAmbientLightColor(g_ambientLightColor.b);
    g_Render.SetAmbientLightColor(g_ambientLightColor);
}

void ApplyShadowIntensity()
{
    g_shadowIntensity = ClampShadowIntensity(g_shadowIntensity);
    g_Render.SetPostEffectDepthBufferShadowIntensity(g_shadowIntensity);
}

void ApplyShadowCoverage()
{
    g_shadowCoverage = ClampShadowCoverage(g_shadowCoverage);
    g_Render.SetPostEffectDepthBufferShadowCoverage(g_shadowCoverage);
}

void ApplyShadowSaturationBoost()
{
    g_shadowSaturationBoost = ClampShadowSaturationBoost(g_shadowSaturationBoost);
    g_Render.SetPostEffectDepthBufferShadowSaturationBoost(g_shadowSaturationBoost);
}

void ApplyShadowPcfTapCount()
{
    g_shadowPcfTapCount = NormalizeShadowBlurTapCountLocal(g_shadowPcfTapCount);
    g_Render.SetPostEffectDepthBufferShadowPcfTapCount(g_shadowPcfTapCount);
}

void ApplyShadowCompositeTapCount()
{
    g_shadowCompositeTapCount = NormalizeShadowBlurTapCountLocal(g_shadowCompositeTapCount);
    g_Render.SetPostEffectDepthBufferShadowCompositeTapCount(g_shadowCompositeTapCount);
}

void ApplyZShadowTexSize()
{
    if (g_zShadowTexSizeDivisor != 2 &&
        g_zShadowTexSizeDivisor != 4 &&
        g_zShadowTexSizeDivisor != 8 &&
        g_zShadowTexSizeDivisor != 16)
    {
        g_zShadowTexSizeDivisor = 1;
    }

    g_Render.SetPostEffectDepthBufferShadowTexSizeDivisor(g_zShadowTexSizeDivisor);
}

void ApplySSAOShadowStrength()
{
    g_ssaoShadowStrength = ClampSSAOShadowStrength(g_ssaoShadowStrength);
    g_Render.SetPostEffectSSAOShadowStrength(g_ssaoShadowStrength);
}

void ApplySSAOShadowSaturationBoost()
{
    g_ssaoShadowSaturationBoost = ClampSSAOShadowSaturationBoost(g_ssaoShadowSaturationBoost);
    g_Render.SetPostEffectSSAOSaturationBoost(g_ssaoShadowSaturationBoost);
}

void ApplySSAOSampleCount()
{
    g_ssaoSampleCount = ClampSSAOSampleCount(g_ssaoSampleCount);
    g_Render.SetPostEffectSSAOSampleCount(g_ssaoSampleCount);
}

void ApplySSAORandomSamplingDirection()
{
    g_Render.SetPostEffectSSAORandomSamplingDirection(g_bSSAORandomSamplingDirection);
}

void ApplySSAODepthScaledSampleDistance()
{
    g_Render.SetPostEffectSSAODepthScaledSampleDistance(g_bSSAODepthScaledSampleDistance);
}

void ApplySSAOMaxDarknessClamp()
{
    g_Render.SetPostEffectSSAOMaxDarknessClamp(g_bSSAOMaxDarknessClamp);
}

void ApplySSAOSampleRadius()
{
    g_ssaoSampleRadius = ClampSSAOSampleRadius(g_ssaoSampleRadius);
    g_Render.SetPostEffectSSAOSampleRadius(g_ssaoSampleRadius);
}

void ApplySSAOBlurKernelSize()
{
    g_ssaoBlurKernelSize = ClampSSAOBlurKernelSize(g_ssaoBlurKernelSize);
    g_Render.SetPostEffectSSAOBlurKernelSize(g_ssaoBlurKernelSize);
}

void ApplySSAOSeparableBlur()
{
    g_Render.SetPostEffectSSAOSeparableBlur(g_bSSAOSeparableBlur);
}

void ApplySSAOTexSize()
{
    if (g_ssaoTexSizeDivisor != 2 &&
        g_ssaoTexSizeDivisor != 4)
    {
        g_ssaoTexSizeDivisor = 1;
    }

    g_Render.SetPostEffectSSAOTexSizeDivisor(g_ssaoTexSizeDivisor);
}

void ApplySSAOCompositeGaussian3x3()
{
    g_Render.SetPostEffectSSAOCompositeGaussian3x3(g_bSSAOCompositeGaussian3x3);
}

void ApplySSGIBlur()
{
    g_Render.SetPostEffectSSGIBlur(g_bSSGIBlur);
}

void ApplySSGISeparableBlur()
{
    g_Render.SetPostEffectSSGISeparableBlur(g_bSSGISeparableBlur);
}

void ApplySSGISampleCount()
{
    g_ssgiSampleCount = ClampSSAOSampleCount(g_ssgiSampleCount);
    g_Render.SetPostEffectSSGISampleCount(g_ssgiSampleCount);
}

void ApplySSGISampleRadius()
{
    g_ssgiSampleRadius = NormalizeSSGISampleRadiusLocal(g_ssgiSampleRadius);
    g_Render.SetPostEffectSSGISampleRadius(g_ssgiSampleRadius);
}

void ApplySSGIBlurKernelSize()
{
    g_ssgiBlurKernelSize =
        ComboIndexToSSGIBlurKernelSize(SSGIBlurKernelSizeToComboIndex(g_ssgiBlurKernelSize));
    g_Render.SetPostEffectSSGIBlurKernelSize(g_ssgiBlurKernelSize);
}

void ApplySSGIIndirectLightStrength()
{
    float indirectLightStrength = g_ssgiIndirectLightStrength;
    if (indirectLightStrength < 0.0f)
    {
        indirectLightStrength = 0.0f;
    }
    if (indirectLightStrength > 5.0f)
    {
        indirectLightStrength = 5.0f;
    }
    g_ssgiIndirectLightStrength = indirectLightStrength;

    g_Render.SetPostEffectSSGIIndirectLightStrength(indirectLightStrength);
}

void ApplySSGIIndirectLightMax()
{
    float maxContribution = g_ssgiIndirectLightMaxContribution;
    if (maxContribution < 0.0f)
    {
        maxContribution = 0.0f;
    }
    if (maxContribution > 1.0f)
    {
        maxContribution = 1.0f;
    }
    g_ssgiIndirectLightMaxContribution = maxContribution;

    g_Render.SetPostEffectSSGIIndirectLightMaxContribution(maxContribution);
}

void ApplyCameraClipPlanes()
{
    g_cameraFarPlane = ClampCameraFarPlane(g_cameraFarPlane);
    if (g_cameraFarPlane <= g_cameraNearPlane)
    {
        g_cameraFarPlane = g_cameraNearPlane + 0.01f;
    }
    g_Render.SetCameraClipPlanes(g_cameraNearPlane, g_cameraFarPlane);
}

void ApplyGBufferEnable()
{
    g_Render.SetGBufferEnable(g_bGBuffer);
}

void ApplyRenderingQuality()
{
    g_renderingQualitySettings = g_Render.SetRenderQuality(g_renderingQualitySettings.quality);
    g_bGBuffer = g_renderingQualitySettings.gBufferEnabled;
    g_bSaturateFilter = g_renderingQualitySettings.saturateEnabled;
    g_bGaussianFilter = g_renderingQualitySettings.gaussianEnabled;
    g_bMaskedGaussianFilter = g_renderingQualitySettings.maskedGaussianEnabled;
    g_bPostEffectAA = g_renderingQualitySettings.postEffectAAEnabled;
    g_bFXAA = g_renderingQualitySettings.fxaaEnabled;
    g_bTAA = g_renderingQualitySettings.taaEnabled;
    g_bMotionBlurCamera = g_renderingQualitySettings.motionBlurCameraEnabled;
    g_bDepthBufferShadow = g_renderingQualitySettings.depthBufferShadowEnabled;
    g_bSSAO = g_renderingQualitySettings.ssaoEnabled;
    g_bSSGI = g_renderingQualitySettings.ssgiEnabled;
    g_bFog = g_renderingQualitySettings.fogEnabled;
    g_bHeightFog = g_renderingQualitySettings.heightFogEnabled;
    g_bBloom = g_renderingQualitySettings.bloomEnabled;
    g_bHalo = g_renderingQualitySettings.haloEnabled;
    g_depthOfFieldMode = g_renderingQualitySettings.depthOfFieldMode;
    g_bStarBurst = g_renderingQualitySettings.starBurstEnabled;
    g_bGodRay = g_renderingQualitySettings.godRayEnabled;
}

void ApplyGBufferClipPlanes()
{
    g_gbufferNearPlane = ClampGBufferNearPlane(g_gbufferNearPlane);
    g_gbufferFarPlane = ClampGBufferFarPlane(g_gbufferFarPlane);
    if (g_gbufferFarPlane <= g_gbufferNearPlane)
    {
        g_gbufferFarPlane = g_gbufferNearPlane + 0.01f;
    }
    g_Render.SetGBufferClipPlanes(g_gbufferNearPlane, g_gbufferFarPlane);
}

void ApplySSAOBlur()
{
    g_Render.SetPostEffectSSAOBlur(g_bSSAOBlur);
}

void ApplyHalfLambertShadowSaturation()
{
    g_halfLambertShadowSaturation = ClampHalfLambertShadowSaturation(g_halfLambertShadowSaturation);
    g_Render.SetMeshMixSaturateShadow(g_halfLambertShadowSaturation > 0.0f);
    g_Render.SetMeshMixSaturateShadowIntensity(g_halfLambertShadowSaturation);
}

void ApplyShadowDarkness()
{
    g_shadowDarkness = ClampShadowDarkness(g_shadowDarkness);
    g_Render.SetMeshMixShadowDarkness(g_shadowDarkness);
}

void ApplySpecularIntensity()
{
    g_specularIntensity = ClampSpecularIntensity(g_specularIntensity);
    g_Render.SetMeshMixSpecularIntensity(g_specularIntensity);
}

void ApplySpecularIntensityOverride()
{
    g_Render.SetMeshMixSpecularIntensityOverrideEnabled(g_bUseSpecularIntensityOverride);
    RefreshSettingsDialogState();
}

void ApplySpecularEdge()
{
    g_specularEdge = ClampSpecularEdge(g_specularEdge);
    g_Render.SetMeshMixSpecularEdge(g_specularEdge);
}

void ApplyFresnelIntensity()
{
    g_fresnelIntensity = ClampFresnelIntensity(g_fresnelIntensity);
    g_Render.SetMeshMixFresnelIntensity(g_fresnelIntensity);
}

void ApplySpecularEdgeOverride()
{
    g_Render.SetMeshMixSpecularEdgeOverrideEnabled(g_bUseSpecularEdgeOverride);
    RefreshSettingsDialogState();
}

void ApplyPhongTreatTextureAsWhite()
{
    g_Render.SetPhongTreatTextureAsWhite(g_bPhongTreatTextureAsWhite);
    RefreshSettingsDialogState();
}

void ApplyEnvMapBlend()
{
    g_envMapBlend = ClampEnvMapBlend(g_envMapBlend);
    g_Render.SetMeshMixEnvMapBlend(g_envMapBlend);
}

void ApplyPBRRoughness()
{
    g_pbrRoughness = ClampPBRRoughness(g_pbrRoughness);
    g_Render.SetMeshPBRRoughness(g_pbrRoughness);
}

void ApplyPBRMetallic()
{
    g_pbrMetallic = ClampPBRMetallic(g_pbrMetallic);
    g_Render.SetMeshPBRMetallic(g_pbrMetallic);
}

void ApplyPBREnvReflectionIntensity()
{
    g_pbrEnvReflectionIntensity = ClampPBREnvReflectionIntensity(g_pbrEnvReflectionIntensity);
    g_Render.SetMeshPBREnvReflectionIntensity(g_pbrEnvReflectionIntensity);
}

void ApplyPBREnvMaxMipLevel()
{
    g_pbrEnvMaxMipLevel = ClampPBREnvMaxMipLevel(g_pbrEnvMaxMipLevel);
    g_Render.SetMeshPBREnvMaxMipLevel(g_pbrEnvMaxMipLevel);
}

void ApplyPBREnvDiffuseIntensity()
{
    g_pbrEnvDiffuseIntensity = ClampPBREnvDiffuseIntensity(g_pbrEnvDiffuseIntensity);
    g_Render.SetMeshPBREnvDiffuseIntensity(g_pbrEnvDiffuseIntensity);
}

void ApplyPBREnvDiffuseMipLevel()
{
    g_pbrEnvDiffuseMipLevel = ClampPBREnvDiffuseMipLevel(g_pbrEnvDiffuseMipLevel);
    g_Render.SetMeshPBREnvDiffuseMipLevel(g_pbrEnvDiffuseMipLevel);
}

bool ApplyPBREnvMapTexturePath()
{
    return g_Render.SetMeshPBREnvMapTexturePath(g_selectedPbrEnvMapPath);
}

void ApplySSS()
{
    g_Render.SetMeshMixSSS(g_bSSS);
    RefreshSettingsDialogState();
}

void ApplySSSIntensity()
{
    g_sssIntensity = ClampSSSIntensity(g_sssIntensity);
    g_Render.SetMeshMixSSSIntensity(g_sssIntensity);
}

void ApplySSSColor()
{
    g_sssColor.r = ClampSSSColor(g_sssColor.r);
    g_sssColor.g = ClampSSSColor(g_sssColor.g);
    g_sssColor.b = ClampSSSColor(g_sssColor.b);
    g_sssColor.a = 1.0f;
    g_Render.SetMeshMixSSSColor(SSSColorToDWORD(g_sssColor));
}

void ApplyBloomThreshold()
{
    g_bloomThreshold = ClampBloomThreshold(g_bloomThreshold);
    g_Render.SetPostEffectBloomThreshold(g_bloomThreshold);
}

float ClampGaussianStrength(const float strength)
{
    return (std::max)(GAUSSIAN_STRENGTH_MIN, (std::min)(strength, GAUSSIAN_STRENGTH_MAX));
}

void ApplyBloomWeightSum()
{
    g_bloomWeightSum = ClampBloomWeightSum(g_bloomWeightSum);
    g_Render.SetPostEffectBloomWeightSum(g_bloomWeightSum);
}

void ApplyHalo()
{
    g_Render.SetPostEffectHalo(g_bHalo);
    RefreshSettingsDialogState();
}

void ApplyDepthOfFieldMode()
{
    g_Render.SetPostEffectDepthOfFieldMode(g_depthOfFieldMode);
}

void ApplyDepthOfFieldFocalDistance()
{
    g_dofFocalDistance = ClampDepthOfFieldFocalDistance(g_dofFocalDistance);
    g_Render.SetPostEffectDepthOfFieldFocalDistance(g_dofFocalDistance);
}

void ApplyDepthOfFieldStartNear()
{
    g_dofStartNear = ClampDepthOfFieldStartNear(g_dofStartNear);
    g_Render.SetPostEffectDepthOfFieldStartNear(g_dofStartNear);
}

void ApplyDepthOfFieldMaxBlurDistance()
{
    g_dofMaxBlurDistance = ClampDepthOfFieldMaxBlurDistance(g_dofMaxBlurDistance);
    g_Render.SetPostEffectDepthOfFieldMaxBlurDistance(g_dofMaxBlurDistance);
}

void ApplyDepthOfFieldAutoActivationDistance()
{
    g_dofAutoActivationDistance = ClampDepthOfFieldAutoActivationDistance(g_dofAutoActivationDistance);
    g_Render.SetPostEffectDepthOfFieldAutoActivationDistance(g_dofAutoActivationDistance);
}

void ApplyStarBurstThreshold()
{
    g_starBurstThreshold = ClampStarBurstThreshold(g_starBurstThreshold);
    g_Render.SetPostEffectStarBurstThreshold(g_starBurstThreshold);
}

void ApplyStarBurstDistanceFade()
{
    g_starBurstDistanceFade = ClampStarBurstDistanceFade(g_starBurstDistanceFade);
    g_Render.SetPostEffectStarBurstDistanceFade(g_starBurstDistanceFade);
}

void ApplyModelLoadScale()
{
    g_modelLoadScale = ClampModelLoadScale(g_modelLoadScale);
}

void ApplyMeshInstancingRenderMode()
{
    g_Render.SetMeshInstancingHighQuality(g_bMeshInstancingHighQuality);
}

void ApplyPointLightColor()
{
    g_pointLightColor.r = ClampPointLightColor(g_pointLightColor.r);
    g_pointLightColor.g = ClampPointLightColor(g_pointLightColor.g);
    g_pointLightColor.b = ClampPointLightColor(g_pointLightColor.b);
    g_pointLightColor.a = 1.0f;
}

void ApplyPointLightBrightness()
{
    g_pointLightBrightness = ClampPointLightBrightness(g_pointLightBrightness);
}

void ApplyPointLightShape()
{
    switch (g_pointLightShape)
    {
    case NSRender::PointLightShape::Point:
    case NSRender::PointLightShape::Line:
    case NSRender::PointLightShape::Square:
    case NSRender::PointLightShape::Cube:
    case NSRender::PointLightShape::Sphere:
        break;
    default:
        g_pointLightShape = NSRender::PointLightShape::Point;
        break;
    }
}

void ApplyPointLightLineSettings()
{
    g_pointLightLineLength = ClampPointLightLineLength(g_pointLightLineLength);
    g_pointLightRotationDegrees.x = ClampPointLightRotationDegrees(g_pointLightRotationDegrees.x);
    g_pointLightRotationDegrees.y = ClampPointLightRotationDegrees(g_pointLightRotationDegrees.y);
    g_pointLightRotationDegrees.z = ClampPointLightRotationDegrees(g_pointLightRotationDegrees.z);
}

void ApplyPointLightSquareSettings()
{
    g_pointLightSquareWidth = ClampPointLightSquareSize(g_pointLightSquareWidth);
    g_pointLightSquareHeight = ClampPointLightSquareSize(g_pointLightSquareHeight);
}

void ApplyGaussianSampleSize()
{
    g_gaussianSampleSize = NormalizeGaussianSampleSizeLocal(g_gaussianSampleSize);
    g_Render.SetPostEffectGaussianSampleSize(g_gaussianSampleSize);
}

void ApplyGaussianStrength()
{
    g_gaussianStrength = ClampGaussianStrength(g_gaussianStrength);
    g_Render.SetPostEffectGaussianStrength(g_gaussianStrength);
}

void ApplyFontExGaussianSampleSize()
{
    g_fontExGaussianSampleSize = NormalizeFontExGaussianSampleSizeLocal(g_fontExGaussianSampleSize);
    g_Render.SetPostEffectFontSampleSize(g_fontExGaussianSampleSize);
}

void ApplyFXAAQuality()
{
    g_fxaaQuality = NormalizeFXAAQualityLocal(g_fxaaQuality);
    g_Render.SetPostEffectFXAAQuality(g_fxaaQuality);
}

void ApplyTAAHistoryWeight()
{
    g_taaHistoryWeight = ClampTAAHistoryWeight(g_taaHistoryWeight);
    g_Render.SetPostEffectTAAHistoryWeight(g_taaHistoryWeight);
}

void ApplyMotionBlurCameraSettings()
{
    g_motionBlurCameraQuality = NormalizeMotionBlurCameraQualityLocal(g_motionBlurCameraQuality);
    g_motionBlurCameraMaxBlurPixels = NormalizeMotionBlurCameraMaxBlurPixelsLocal(g_motionBlurCameraMaxBlurPixels);
    g_motionBlurCameraSampleCount = NormalizeMotionBlurCameraSampleCountLocal(g_motionBlurCameraSampleCount);

    g_Render.SetPostEffectMotionBlurCameraQuality(g_motionBlurCameraQuality);
    g_Render.SetPostEffectMotionBlurCameraMaxBlurPixels(g_motionBlurCameraMaxBlurPixels);
    g_Render.SetPostEffectMotionBlurCameraSampleCount(g_motionBlurCameraSampleCount);
}

void ApplyResolution()
{
    if (g_resolutionWidth <= 0 || g_resolutionHeight <= 0)
    {
        return;
    }

    g_Render.ChangeResolution(g_resolutionWidth, g_resolutionHeight);
}

void ApplyWindowMode()
{
    g_Render.ChangeWindowMode(g_windowMode);
}

void InitializeRemoteDesktopDefault()
{
    const bool isRemoteDesktopDefault = IsWeekdayBusinessHours();
    g_bRemoteDesktop = isRemoteDesktopDefault;

    if (isRemoteDesktopDefault)
    {
        g_resolutionWidth = 1600;
        g_resolutionHeight = 900;
        return;
    }

    g_resolutionWidth = 1920;
    g_resolutionHeight = 1080;
}

int SaturateLevelToSliderValue(const float level)
{
    // ダイアログは整数スライダーしか扱えないため、内部値を step 単位へ変換する。
    return static_cast<int>(std::lround(ClampSaturateLevel(level) / SATURATE_STEP));
}

float SliderValueToSaturateLevel(const int sliderValue)
{
    return ClampSaturateLevel(static_cast<float>(sliderValue) * SATURATE_STEP);
}

int FogIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampFogIntensity(intensity) / FOG_INTENSITY_STEP));
}

float SliderValueToFogIntensity(const int sliderValue)
{
    return ClampFogIntensity(static_cast<float>(sliderValue) * FOG_INTENSITY_STEP);
}

int FogColorToSliderValue(const float value)
{
    return static_cast<int>(std::lround(ClampFogColor(value) / FOG_COLOR_STEP));
}

float SliderValueToFogColor(const int sliderValue)
{
    return ClampFogColor(static_cast<float>(sliderValue) * FOG_COLOR_STEP);
}

int HeightFogIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampHeightFogIntensity(intensity) / HEIGHT_FOG_INTENSITY_STEP));
}

float SliderValueToHeightFogIntensity(const int sliderValue)
{
    return ClampHeightFogIntensity(static_cast<float>(sliderValue) * HEIGHT_FOG_INTENSITY_STEP);
}

int HeightFogHeightToSliderValue(const float height)
{
    return static_cast<int>(std::lround((ClampHeightFogHeight(height) - HEIGHT_FOG_HEIGHT_MIN) / HEIGHT_FOG_HEIGHT_STEP));
}

float SliderValueToHeightFogHeight(const int sliderValue)
{
    return ClampHeightFogHeight(HEIGHT_FOG_HEIGHT_MIN + static_cast<float>(sliderValue) * HEIGHT_FOG_HEIGHT_STEP);
}

int HeightFogDistanceToSliderValue(const float distance)
{
    return static_cast<int>(std::lround((ClampHeightFogDistance(distance) - HEIGHT_FOG_DISTANCE_MIN) / HEIGHT_FOG_DISTANCE_STEP));
}

float SliderValueToHeightFogDistance(const int sliderValue)
{
    return ClampHeightFogDistance(HEIGHT_FOG_DISTANCE_MIN + static_cast<float>(sliderValue) * HEIGHT_FOG_DISTANCE_STEP);
}

int SunLightIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampSunLightIntensity(intensity) / SUN_LIGHT_INTENSITY_STEP));
}

float SliderValueToSunLightIntensity(const int sliderValue)
{
    return ClampSunLightIntensity(static_cast<float>(sliderValue) * SUN_LIGHT_INTENSITY_STEP);
}

int SunLightColorToSliderValue(const float value)
{
    return static_cast<int>(std::lround(ClampSunLightColor(value) / SUN_LIGHT_COLOR_STEP));
}

float SliderValueToSunLightColor(const int sliderValue)
{
    return ClampSunLightColor(static_cast<float>(sliderValue) * SUN_LIGHT_COLOR_STEP);
}

int AmbientLightIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampAmbientLightIntensity(intensity) / AMBIENT_LIGHT_INTENSITY_STEP));
}

float SliderValueToAmbientLightIntensity(const int sliderValue)
{
    return ClampAmbientLightIntensity(static_cast<float>(sliderValue) * AMBIENT_LIGHT_INTENSITY_STEP);
}

int ShadowIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampShadowIntensity(intensity) / SHADOW_INTENSITY_STEP));
}

float SliderValueToShadowIntensity(const int sliderValue)
{
    return ClampShadowIntensity(static_cast<float>(sliderValue) * SHADOW_INTENSITY_STEP);
}

int ShadowCoverageToSliderValue(const float coverage)
{
    return static_cast<int>(std::lround(ClampShadowCoverage(coverage) / SHADOW_COVERAGE_STEP));
}

float SliderValueToShadowCoverage(const int sliderValue)
{
    return ClampShadowCoverage(static_cast<float>(sliderValue) * SHADOW_COVERAGE_STEP);
}

int ShadowSaturationBoostToSliderValue(const float boost)
{
    return static_cast<int>(std::lround(ClampShadowSaturationBoost(boost) / SHADOW_SATURATION_BOOST_STEP));
}

float SliderValueToShadowSaturationBoost(const int sliderValue)
{
    return ClampShadowSaturationBoost(static_cast<float>(sliderValue) * SHADOW_SATURATION_BOOST_STEP);
}

int ShadowTapCountToComboIndex(const int tapCount)
{
    return ((NormalizeShadowBlurTapCountLocal(tapCount) - 1) / 2);
}

int ComboIndexToShadowTapCount(const int comboIndex)
{
    return NormalizeShadowBlurTapCountLocal((comboIndex * 2) + 1);
}

int SSAOShadowStrengthToSliderValue(const float shadowStrength)
{
    return static_cast<int>(std::lround(ClampSSAOShadowStrength(shadowStrength) / SSAO_SHADOW_STRENGTH_STEP));
}

float SliderValueToSSAOShadowStrength(const int sliderValue)
{
    return ClampSSAOShadowStrength(static_cast<float>(sliderValue) * SSAO_SHADOW_STRENGTH_STEP);
}

int SSAOShadowSaturationBoostToSliderValue(const float boost)
{
    return static_cast<int>(std::lround(ClampSSAOShadowSaturationBoost(boost) / SSAO_SHADOW_SATURATION_BOOST_STEP));
}

float SliderValueToSSAOShadowSaturationBoost(const int sliderValue)
{
    return ClampSSAOShadowSaturationBoost(static_cast<float>(sliderValue) * SSAO_SHADOW_SATURATION_BOOST_STEP);
}

int SSAOSampleCountToComboIndex(const int sampleCount)
{
    const int normalizedSampleCount = ClampSSAOSampleCount(sampleCount);

    if (normalizedSampleCount == 4)
    {
        return 0;
    }

    if (normalizedSampleCount == 8)
    {
        return 1;
    }

    if (normalizedSampleCount == 16)
    {
        return 2;
    }

    if (normalizedSampleCount == 32)
    {
        return 3;
    }

    return 4;
}

int ComboIndexToSSAOSampleCount(const int comboIndex)
{
    if (comboIndex == 0)
    {
        return 4;
    }

    if (comboIndex == 1)
    {
        return 8;
    }

    if (comboIndex == 2)
    {
        return 16;
    }

    if (comboIndex == 3)
    {
        return 32;
    }

    return 64;
}

int SSAOSampleRadiusToSliderValue(const float sampleRadius)
{
    return static_cast<int>(std::lround((ClampSSAOSampleRadius(sampleRadius) - SSAO_SAMPLE_RADIUS_MIN) / SSAO_SAMPLE_RADIUS_STEP));
}

float SliderValueToSSAOSampleRadius(const int sliderValue)
{
    return ClampSSAOSampleRadius(SSAO_SAMPLE_RADIUS_MIN + static_cast<float>(sliderValue) * SSAO_SAMPLE_RADIUS_STEP);
}

int SSAOBlurKernelSizeToComboIndex(const int kernelSize)
{
    const int normalizedKernelSize = ClampSSAOBlurKernelSize(kernelSize);

    if (normalizedKernelSize == 3)
    {
        return 0;
    }

    if (normalizedKernelSize == 5)
    {
        return 1;
    }

    if (normalizedKernelSize == 11)
    {
        return 2;
    }

    return 3;
}

int ComboIndexToSSAOBlurKernelSize(const int comboIndex)
{
    if (comboIndex == 0)
    {
        return 3;
    }

    if (comboIndex == 1)
    {
        return 5;
    }

    if (comboIndex == 2)
    {
        return 11;
    }

    return 21;
}

int SSGISampleCountToComboIndex(const int sampleCount)
{
    const int normalizedSampleCount = ClampSSAOSampleCount(sampleCount);

    if (normalizedSampleCount == 4)
    {
        return 0;
    }

    if (normalizedSampleCount == 8)
    {
        return 1;
    }

    if (normalizedSampleCount == 16)
    {
        return 2;
    }

    if (normalizedSampleCount == 32)
    {
        return 3;
    }

    return 4;
}

int ComboIndexToSSGISampleCount(const int comboIndex)
{
    if (comboIndex == 0)
    {
        return 4;
    }

    if (comboIndex == 1)
    {
        return 8;
    }

    if (comboIndex == 2)
    {
        return 16;
    }

    if (comboIndex == 3)
    {
        return 32;
    }

    return 64;
}

int SSGISampleRadiusToSliderValue(const float sampleRadius)
{
    return static_cast<int>(std::lround((NormalizeSSGISampleRadiusLocal(sampleRadius) - SSGI_SAMPLE_RADIUS_MIN) / SSGI_SAMPLE_RADIUS_STEP));
}

float SliderValueToSSGISampleRadius(const int sliderValue)
{
    return NormalizeSSGISampleRadiusLocal(SSGI_SAMPLE_RADIUS_MIN + static_cast<float>(sliderValue) * SSGI_SAMPLE_RADIUS_STEP);
}

int CameraShakeDurationToSliderValue(const float durationSeconds)
{
    return static_cast<int>(std::lround((durationSeconds - CAMERA_SHAKE_DURATION_MIN) / CAMERA_SHAKE_DURATION_STEP));
}

float SliderValueToCameraShakeDuration(const int sliderValue)
{
    return CAMERA_SHAKE_DURATION_MIN + static_cast<float>(sliderValue) * CAMERA_SHAKE_DURATION_STEP;
}

int CameraShakeIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(intensity / CAMERA_SHAKE_INTENSITY_STEP));
}

float SliderValueToCameraShakeIntensity(const int sliderValue)
{
    return static_cast<float>(sliderValue) * CAMERA_SHAKE_INTENSITY_STEP;
}

int SSGIBlurKernelSizeToComboIndex(const int kernelSize)
{
    if (kernelSize <= 8)
    {
        return 0;
    }

    if (kernelSize <= 16)
    {
        return 1;
    }

    return 2;
}

int ComboIndexToSSGIBlurKernelSize(const int comboIndex)
{
    if (comboIndex == 0)
    {
        return 5;
    }

    if (comboIndex == 1)
    {
        return 11;
    }

    return 21;
}

int HalfLambertShadowSaturationToSliderValue(const float boost)
{
    return static_cast<int>(std::lround(ClampHalfLambertShadowSaturation(boost) / HALF_LAMBERT_SHADOW_SATURATION_STEP));
}

float SliderValueToHalfLambertShadowSaturation(const int sliderValue)
{
    return ClampHalfLambertShadowSaturation(static_cast<float>(sliderValue) * HALF_LAMBERT_SHADOW_SATURATION_STEP);
}

int ShadowDarknessToSliderValue(const float darkness)
{
    return static_cast<int>(std::lround(ClampShadowDarkness(darkness) / SHADOW_DARKNESS_STEP));
}

float SliderValueToShadowDarkness(const int sliderValue)
{
    return ClampShadowDarkness(static_cast<float>(sliderValue) * SHADOW_DARKNESS_STEP);
}

int SpecularIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampSpecularIntensity(intensity) / SPECULAR_INTENSITY_STEP));
}

float SliderValueToSpecularIntensity(const int sliderValue)
{
    return ClampSpecularIntensity(static_cast<float>(sliderValue) * SPECULAR_INTENSITY_STEP);
}

int SpecularEdgeToSliderValue(const float edge)
{
    return static_cast<int>(std::lround(ClampSpecularEdge(edge) / SPECULAR_EDGE_STEP));
}

float SliderValueToSpecularEdge(const int sliderValue)
{
    return ClampSpecularEdge(static_cast<float>(sliderValue) * SPECULAR_EDGE_STEP);
}

int FresnelIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampFresnelIntensity(intensity) / FRESNEL_INTENSITY_STEP));
}

float SliderValueToFresnelIntensity(const int sliderValue)
{
    return ClampFresnelIntensity(static_cast<float>(sliderValue) * FRESNEL_INTENSITY_STEP);
}

int EnvMapBlendToSliderValue(const float blend)
{
    return static_cast<int>(std::lround(ClampEnvMapBlend(blend) / ENV_MAP_BLEND_STEP));
}

float SliderValueToEnvMapBlend(const int sliderValue)
{
    return ClampEnvMapBlend(static_cast<float>(sliderValue) * ENV_MAP_BLEND_STEP);
}

int TAAHistoryWeightToSliderValue(const float historyWeight)
{
    return static_cast<int>(std::lround(ClampTAAHistoryWeight(historyWeight) / TAA_HISTORY_WEIGHT_STEP));
}

float SliderValueToTAAHistoryWeight(const int sliderValue)
{
    return ClampTAAHistoryWeight(static_cast<float>(sliderValue) * TAA_HISTORY_WEIGHT_STEP);
}

int PBRRoughnessToSliderValue(const float roughness)
{
    return static_cast<int>(std::lround((ClampPBRRoughness(roughness) - PBR_ROUGHNESS_MIN) / PBR_ROUGHNESS_STEP));
}

float SliderValueToPBRRoughness(const int sliderValue)
{
    return ClampPBRRoughness(PBR_ROUGHNESS_MIN + static_cast<float>(sliderValue) * PBR_ROUGHNESS_STEP);
}

int PBRMetallicToSliderValue(const float metallic)
{
    return static_cast<int>(std::lround(ClampPBRMetallic(metallic) / PBR_METALLIC_STEP));
}

float SliderValueToPBRMetallic(const int sliderValue)
{
    return ClampPBRMetallic(static_cast<float>(sliderValue) * PBR_METALLIC_STEP);
}

int PBREnvReflectionIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampPBREnvReflectionIntensity(intensity) / PBR_ENV_REFLECTION_INTENSITY_STEP));
}

float SliderValueToPBREnvReflectionIntensity(const int sliderValue)
{
    return ClampPBREnvReflectionIntensity(static_cast<float>(sliderValue) * PBR_ENV_REFLECTION_INTENSITY_STEP);
}

int PBREnvMaxMipLevelToSliderValue(const float mipLevel)
{
    return static_cast<int>(std::lround(ClampPBREnvMaxMipLevel(mipLevel) / PBR_ENV_MAX_MIP_LEVEL_STEP));
}

float SliderValueToPBREnvMaxMipLevel(const int sliderValue)
{
    return ClampPBREnvMaxMipLevel(static_cast<float>(sliderValue) * PBR_ENV_MAX_MIP_LEVEL_STEP);
}

int PBREnvDiffuseIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampPBREnvDiffuseIntensity(intensity) / PBR_ENV_DIFFUSE_INTENSITY_STEP));
}

float SliderValueToPBREnvDiffuseIntensity(const int sliderValue)
{
    return ClampPBREnvDiffuseIntensity(static_cast<float>(sliderValue) * PBR_ENV_DIFFUSE_INTENSITY_STEP);
}

int PBREnvDiffuseMipLevelToSliderValue(const float mipLevel)
{
    return static_cast<int>(std::lround(ClampPBREnvDiffuseMipLevel(mipLevel) / PBR_ENV_DIFFUSE_MIP_LEVEL_STEP));
}

float SliderValueToPBREnvDiffuseMipLevel(const int sliderValue)
{
    return ClampPBREnvDiffuseMipLevel(static_cast<float>(sliderValue) * PBR_ENV_DIFFUSE_MIP_LEVEL_STEP);
}

int SSSIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampSSSIntensity(intensity) / SSS_INTENSITY_STEP));
}

float SliderValueToSSSIntensity(const int sliderValue)
{
    return ClampSSSIntensity(static_cast<float>(sliderValue) * SSS_INTENSITY_STEP);
}

int SSSColorToSliderValue(const float value)
{
    return static_cast<int>(std::lround(ClampSSSColor(value) / SSS_COLOR_STEP));
}

float SliderValueToSSSColor(const int sliderValue)
{
    return ClampSSSColor(static_cast<float>(sliderValue) * SSS_COLOR_STEP);
}

int BloomThresholdToSliderValue(const float threshold)
{
    return static_cast<int>(std::lround(ClampBloomThreshold(threshold) / BLOOM_THRESHOLD_STEP));
}

float SliderValueToBloomThreshold(const int sliderValue)
{
    return ClampBloomThreshold(static_cast<float>(sliderValue) * BLOOM_THRESHOLD_STEP);
}

int BloomWeightSumToSliderValue(const float weightSum)
{
    return static_cast<int>(std::lround(ClampBloomWeightSum(weightSum)));
}

float SliderValueToBloomWeightSum(const int sliderValue)
{
    return ClampBloomWeightSum(static_cast<float>(sliderValue));
}

int DepthOfFieldFocalDistanceToSliderValue(const float distance)
{
    return static_cast<int>(std::lround((ClampDepthOfFieldFocalDistance(distance) - DOF_FOCAL_DISTANCE_MIN) / DOF_FOCAL_DISTANCE_STEP));
}

float SliderValueToDepthOfFieldFocalDistance(const int sliderValue)
{
    return ClampDepthOfFieldFocalDistance(DOF_FOCAL_DISTANCE_MIN + static_cast<float>(sliderValue) * DOF_FOCAL_DISTANCE_STEP);
}

int DepthOfFieldStartNearToSliderValue(const float distance)
{
    return static_cast<int>(std::lround((ClampDepthOfFieldStartNear(distance) - DOF_START_NEAR_MIN) / DOF_START_NEAR_STEP));
}

float SliderValueToDepthOfFieldStartNear(const int sliderValue)
{
    return ClampDepthOfFieldStartNear(DOF_START_NEAR_MIN + static_cast<float>(sliderValue) * DOF_START_NEAR_STEP);
}

int DepthOfFieldMaxBlurDistanceToSliderValue(const float distance)
{
    return static_cast<int>(std::lround((ClampDepthOfFieldMaxBlurDistance(distance) - DOF_MAX_BLUR_DISTANCE_MIN) / DOF_MAX_BLUR_DISTANCE_STEP));
}

float SliderValueToDepthOfFieldMaxBlurDistance(const int sliderValue)
{
    return ClampDepthOfFieldMaxBlurDistance(DOF_MAX_BLUR_DISTANCE_MIN + static_cast<float>(sliderValue) * DOF_MAX_BLUR_DISTANCE_STEP);
}

int DepthOfFieldAutoActivationDistanceToSliderValue(const float distance)
{
    return static_cast<int>(std::lround(
        (ClampDepthOfFieldAutoActivationDistance(distance) - DOF_AUTO_ACTIVATION_DISTANCE_MIN) /
        DOF_AUTO_ACTIVATION_DISTANCE_STEP));
}

float SliderValueToDepthOfFieldAutoActivationDistance(const int sliderValue)
{
    return ClampDepthOfFieldAutoActivationDistance(
        DOF_AUTO_ACTIVATION_DISTANCE_MIN + static_cast<float>(sliderValue) * DOF_AUTO_ACTIVATION_DISTANCE_STEP);
}

int StarBurstThresholdToSliderValue(const float threshold)
{
    return static_cast<int>(std::lround(ClampStarBurstThreshold(threshold) / STARBURST_THRESHOLD_STEP));
}

float SliderValueToStarBurstThreshold(const int sliderValue)
{
    return ClampStarBurstThreshold(static_cast<float>(sliderValue) * STARBURST_THRESHOLD_STEP);
}

int StarBurstDistanceFadeToSliderValue(const float fade)
{
    return static_cast<int>(std::lround(ClampStarBurstDistanceFade(fade) / STARBURST_DISTANCE_FADE_STEP));
}

float SliderValueToStarBurstDistanceFade(const int sliderValue)
{
    return ClampStarBurstDistanceFade(static_cast<float>(sliderValue) * STARBURST_DISTANCE_FADE_STEP);
}

int ModelLoadScaleToSliderValue(const float scale)
{
    return static_cast<int>(std::lround((ClampModelLoadScale(scale) - MODEL_LOAD_SCALE_MIN) / MODEL_LOAD_SCALE_STEP));
}

float SliderValueToModelLoadScale(const int sliderValue)
{
    return ClampModelLoadScale(MODEL_LOAD_SCALE_MIN + static_cast<float>(sliderValue) * MODEL_LOAD_SCALE_STEP);
}

int PointLightColorToSliderValue(const float value)
{
    return static_cast<int>(std::lround(ClampPointLightColor(value) / POINT_LIGHT_COLOR_STEP));
}

float SliderValueToPointLightColor(const int sliderValue)
{
    return ClampPointLightColor(static_cast<float>(sliderValue) * POINT_LIGHT_COLOR_STEP);
}

int PointLightBrightnessToSliderValue(const float brightness)
{
    return static_cast<int>(std::lround(ClampPointLightBrightness(brightness) / POINT_LIGHT_BRIGHTNESS_STEP));
}

float SliderValueToPointLightBrightness(const int sliderValue)
{
    return ClampPointLightBrightness(static_cast<float>(sliderValue) * POINT_LIGHT_BRIGHTNESS_STEP);
}

int GaussianSampleSizeToSliderValue(const int sampleSize)
{
    return (NormalizeGaussianSampleSizeLocal(sampleSize) + 1) / 2;
}

int SliderValueToGaussianSampleSize(const int sliderValue)
{
    return NormalizeGaussianSampleSizeLocal(sliderValue * 2 - 1);
}

int GaussianStrengthToSliderValue(const float strength)
{
    return static_cast<int>(std::lround(ClampGaussianStrength(strength) / GAUSSIAN_STRENGTH_STEP));
}

float SliderValueToGaussianStrength(const int sliderValue)
{
    return ClampGaussianStrength(static_cast<float>(sliderValue) * GAUSSIAN_STRENGTH_STEP);
}

int FontExGaussianSampleSizeToSliderValue(const int sampleSize)
{
    return NormalizeFontExGaussianSampleSizeLocal(sampleSize);
}

int SliderValueToFontExGaussianSampleSize(const int sliderValue)
{
    return NormalizeFontExGaussianSampleSizeLocal(sliderValue);
}

int FXAAQualityToSliderValue(const int quality)
{
    return NormalizeFXAAQualityLocal(quality);
}

int SliderValueToFXAAQuality(const int sliderValue)
{
    return NormalizeFXAAQualityLocal(sliderValue);
}

int MotionBlurCameraQualityToSliderValue(const int quality)
{
    return NormalizeMotionBlurCameraQualityLocal(quality);
}

int SliderValueToMotionBlurCameraQuality(const int sliderValue)
{
    return NormalizeMotionBlurCameraQualityLocal(sliderValue);
}

int MotionBlurCameraMaxBlurPixelsToSliderValue(const float maxBlurPixels)
{
    return static_cast<int>(NormalizeMotionBlurCameraMaxBlurPixelsLocal(maxBlurPixels));
}

float SliderValueToMotionBlurCameraMaxBlurPixels(const int sliderValue)
{
    return NormalizeMotionBlurCameraMaxBlurPixelsLocal(static_cast<float>(sliderValue));
}

int MotionBlurCameraSampleCountToSliderValue(const int sampleCount)
{
    return NormalizeMotionBlurCameraSampleCountLocal(sampleCount);
}

int SliderValueToMotionBlurCameraSampleCount(const int sliderValue)
{
    return NormalizeMotionBlurCameraSampleCountLocal(sliderValue);
}

void ApplyGodRay()
{
    g_Render.SetPostEffectGodRay(g_bGodRay);

    if (g_bGodRay)
    {
        if (g_godRaySourceMarkerMeshId == -1)
        {
            g_godRaySourceMarkerMeshId = AddGodRayMarker(GetGodRaySourceMarkerPos());
        }

        if (g_godRayEffectiveMarkerMeshId == -1)
        {
            g_godRayEffectiveMarkerMeshId = AddGodRayMarker(GetGodRayEffectiveMarkerPos());
        }
    }
    else
    {
        if (g_godRaySourceMarkerMeshId != -1)
        {
            g_Render.RemoveMeshMix(g_godRaySourceMarkerMeshId);
            g_godRaySourceMarkerMeshId = -1;
        }

        if (g_godRayEffectiveMarkerMeshId != -1)
        {
            g_Render.RemoveMeshMix(g_godRayEffectiveMarkerMeshId);
            g_godRayEffectiveMarkerMeshId = -1;
        }
    }
}

void ApplyGodRayLightColor()
{
    g_godRayLightColor.x = (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(g_godRayLightColor.x, GODRAY_LIGHT_COLOR_MAX));
    g_godRayLightColor.y = (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(g_godRayLightColor.y, GODRAY_LIGHT_COLOR_MAX));
    g_godRayLightColor.z = (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(g_godRayLightColor.z, GODRAY_LIGHT_COLOR_MAX));
    g_Render.SetPostEffectGodRayLightColor(g_godRayLightColor);
}

void ApplyGodRayIntensity()
{
    g_godRayIntensity = (std::max)(GODRAY_INTENSITY_MIN, (std::min)(g_godRayIntensity, GODRAY_INTENSITY_MAX));
    g_Render.SetPostEffectGodRayIntensity(g_godRayIntensity);
}

void ApplyGodRayVirtualProximityStrength()
{
    g_godRayVirtualProximityStrength = (std::max)(
        GODRAY_VIRTUAL_PROXIMITY_MIN,
        (std::min)(g_godRayVirtualProximityStrength, GODRAY_VIRTUAL_PROXIMITY_MAX));
    g_Render.SetPostEffectGodRayVirtualProximityStrength(g_godRayVirtualProximityStrength);
}

void ApplyGodRayLightPos()
{
    const bool useVirtualLight = IsGodRayLightBehindCamera();
    D3DXVECTOR3 lightPos = g_godRayLightPos;
    if (useVirtualLight)
    {
        lightPos = GetEffectiveGodRayLightPos();
    }
    g_Render.SetPostEffectGodRayLightPos(lightPos);
    g_Render.SetPostEffectGodRayReverseSampling(useVirtualLight);

    if (g_godRaySourceMarkerMeshId != -1)
    {
        g_Render.SetMeshMixPos(g_godRaySourceMarkerMeshId, GetGodRaySourceMarkerPos());
    }

    if (g_godRayEffectiveMarkerMeshId != -1)
    {
        g_Render.SetMeshMixPos(g_godRayEffectiveMarkerMeshId, GetGodRayEffectiveMarkerPos());
    }
}

int GodRayLightColorToSliderValue(const float value)
{
    return static_cast<int>(std::lround(
        (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(value, GODRAY_LIGHT_COLOR_MAX)) / GODRAY_LIGHT_COLOR_STEP));
}

float SliderValueToGodRayLightColor(const int sliderValue)
{
    return (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(static_cast<float>(sliderValue) * GODRAY_LIGHT_COLOR_STEP, GODRAY_LIGHT_COLOR_MAX));
}

int GodRayIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(
        (std::max)(GODRAY_INTENSITY_MIN, (std::min)(intensity, GODRAY_INTENSITY_MAX)) / GODRAY_INTENSITY_STEP));
}

float SliderValueToGodRayIntensity(const int sliderValue)
{
    return (std::max)(GODRAY_INTENSITY_MIN, (std::min)(static_cast<float>(sliderValue) * GODRAY_INTENSITY_STEP, GODRAY_INTENSITY_MAX));
}

int GodRayVirtualProximityStrengthToSliderValue(const float strength)
{
    return static_cast<int>(std::lround(
        (std::max)(GODRAY_VIRTUAL_PROXIMITY_MIN,
                   (std::min)(strength, GODRAY_VIRTUAL_PROXIMITY_MAX)) /
        GODRAY_VIRTUAL_PROXIMITY_STEP));
}

float SliderValueToGodRayVirtualProximityStrength(const int sliderValue)
{
    return (std::max)(GODRAY_VIRTUAL_PROXIMITY_MIN,
                      (std::min)(static_cast<float>(sliderValue) * GODRAY_VIRTUAL_PROXIMITY_STEP,
                                 GODRAY_VIRTUAL_PROXIMITY_MAX));
}

int GodRayLightPosToSliderValue(const float pos)
{
    return static_cast<int>(std::lround(
        (std::max)(GODRAY_LIGHT_POS_MIN, (std::min)(pos, GODRAY_LIGHT_POS_MAX)) / GODRAY_LIGHT_POS_STEP));
}

float SliderValueToGodRayLightPos(const int sliderValue)
{
    return (std::max)(GODRAY_LIGHT_POS_MIN, (std::min)(static_cast<float>(sliderValue) * GODRAY_LIGHT_POS_STEP, GODRAY_LIGHT_POS_MAX));
}

