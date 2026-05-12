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
    return (std::max)(SSAO_SAMPLE_COUNT_MIN, (std::min)(sampleCount, SSAO_SAMPLE_COUNT_MAX));
}

float ClampSSAOSampleRadius(const float sampleRadius)
{
    return (std::max)(SSAO_SAMPLE_RADIUS_MIN, (std::min)(sampleRadius, SSAO_SAMPLE_RADIUS_MAX));
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

float ClampDepthOfFieldFocalDistance(const float distance)
{
    return (std::max)(DOF_FOCAL_DISTANCE_MIN, (std::min)(distance, DOF_FOCAL_DISTANCE_MAX));
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
    const bool isBusinessHours = localTime.wHour >= 9 && localTime.wHour < 18;
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
    ApplySSAOBlur();
    g_Render.SetPostEffectFog(g_bFog);
    g_Render.SetPostEffectHeightFog(g_bHeightFog);
    g_Render.SetPostEffectSaturateEnable(g_bSaturateFilter);
    g_Render.SetPostEffectGaussianFilter(g_bGaussianFilter);
    g_Render.SetPostEffectMaskedGaussianFilter(g_bMaskedGaussianFilter);
    g_Render.SetPostEffectFXAA(g_bFXAA);
    g_Render.SetPostEffectMotionBlurCamera(g_bMotionBlurCamera);
    g_Render.SetMeshMixSSS(g_bSSS);
    g_Render.SetPostEffectBloom(g_bBloom);
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

void ApplySSAODepthScaledSampleDistance()
{
    g_Render.SetPostEffectSSAODepthScaledSampleDistance(g_bSSAODepthScaledSampleDistance);
}

void ApplySSAOSampleRadius()
{
    g_ssaoSampleRadius = ClampSSAOSampleRadius(g_ssaoSampleRadius);
    g_Render.SetPostEffectSSAOSampleRadius(g_ssaoSampleRadius);
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

void ApplySpecularEdgeOverride()
{
    g_Render.SetMeshMixSpecularEdgeOverrideEnabled(g_bUseSpecularEdgeOverride);
    RefreshSettingsDialogState();
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

void ApplyDepthOfFieldMode()
{
    g_Render.SetPostEffectDepthOfFieldMode(g_depthOfFieldMode);
}

void ApplyDepthOfFieldFocalDistance()
{
    g_dofFocalDistance = ClampDepthOfFieldFocalDistance(g_dofFocalDistance);
    g_Render.SetPostEffectDepthOfFieldFocalDistance(g_dofFocalDistance);
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

void ApplyModelLoadScale()
{
    g_modelLoadScale = ClampModelLoadScale(g_modelLoadScale);
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

void ApplyFXAAQuality()
{
    g_fxaaQuality = NormalizeFXAAQualityLocal(g_fxaaQuality);
    g_Render.SetPostEffectFXAAQuality(g_fxaaQuality);
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
    g_bRemoteDesktop = IsWeekdayBusinessHours();
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

int ShadowTapCountToSliderValue(const int tapCount)
{
    return ((NormalizeShadowBlurTapCountLocal(tapCount) - 1) / 2);
}

int SliderValueToShadowTapCount(const int sliderValue)
{
    return NormalizeShadowBlurTapCountLocal((sliderValue * 2) + 1);
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

int SSAOSampleCountToSliderValue(const int sampleCount)
{
    return ClampSSAOSampleCount(sampleCount);
}

int SliderValueToSSAOSampleCount(const int sliderValue)
{
    return ClampSSAOSampleCount(sliderValue);
}

int SSAOSampleRadiusToSliderValue(const float sampleRadius)
{
    return static_cast<int>(std::lround((ClampSSAOSampleRadius(sampleRadius) - SSAO_SAMPLE_RADIUS_MIN) / SSAO_SAMPLE_RADIUS_STEP));
}

float SliderValueToSSAOSampleRadius(const int sliderValue)
{
    return ClampSSAOSampleRadius(SSAO_SAMPLE_RADIUS_MIN + static_cast<float>(sliderValue) * SSAO_SAMPLE_RADIUS_STEP);
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

int DepthOfFieldFocalDistanceToSliderValue(const float distance)
{
    return static_cast<int>(std::lround((ClampDepthOfFieldFocalDistance(distance) - DOF_FOCAL_DISTANCE_MIN) / DOF_FOCAL_DISTANCE_STEP));
}

float SliderValueToDepthOfFieldFocalDistance(const int sliderValue)
{
    return ClampDepthOfFieldFocalDistance(DOF_FOCAL_DISTANCE_MIN + static_cast<float>(sliderValue) * DOF_FOCAL_DISTANCE_STEP);
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

