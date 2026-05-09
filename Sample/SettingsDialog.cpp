#include "SettingsDialog.h"

#include <cassert>
#include <commctrl.h>
#include <string>
#include <cwchar>
#include <windowsx.h>

#include "AppState.h"
#include "../Render/Light.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
constexpr int SATURATE_SLIDER_MIN = 0;
constexpr int SATURATE_SLIDER_MAX = static_cast<int>(SATURATE_MAX / SATURATE_STEP);
constexpr int FOG_SLIDER_MIN = 0;
constexpr int FOG_SLIDER_MAX = static_cast<int>(FOG_INTENSITY_MAX / FOG_INTENSITY_STEP);
constexpr int HEIGHT_FOG_INTENSITY_SLIDER_MIN = 0;
constexpr int HEIGHT_FOG_INTENSITY_SLIDER_MAX = static_cast<int>(HEIGHT_FOG_INTENSITY_MAX / HEIGHT_FOG_INTENSITY_STEP);
constexpr int HEIGHT_FOG_HEIGHT_SLIDER_MIN = 0;
constexpr int HEIGHT_FOG_HEIGHT_SLIDER_MAX = static_cast<int>((HEIGHT_FOG_HEIGHT_MAX - HEIGHT_FOG_HEIGHT_MIN) / HEIGHT_FOG_HEIGHT_STEP);
constexpr int HEIGHT_FOG_DISTANCE_SLIDER_MIN = 0;
constexpr int HEIGHT_FOG_DISTANCE_SLIDER_MAX = static_cast<int>((HEIGHT_FOG_DISTANCE_MAX - HEIGHT_FOG_DISTANCE_MIN) / HEIGHT_FOG_DISTANCE_STEP);
constexpr int SUN_LIGHT_INTENSITY_SLIDER_MIN = 0;
constexpr int SUN_LIGHT_INTENSITY_SLIDER_MAX = static_cast<int>(SUN_LIGHT_INTENSITY_MAX / SUN_LIGHT_INTENSITY_STEP);
constexpr int SUN_LIGHT_COLOR_SLIDER_MIN = 0;
constexpr int SUN_LIGHT_COLOR_SLIDER_MAX = static_cast<int>(SUN_LIGHT_COLOR_MAX / SUN_LIGHT_COLOR_STEP);
constexpr int AMBIENT_LIGHT_INTENSITY_SLIDER_MIN = 0;
constexpr int AMBIENT_LIGHT_INTENSITY_SLIDER_MAX = static_cast<int>(AMBIENT_LIGHT_INTENSITY_MAX / AMBIENT_LIGHT_INTENSITY_STEP);
constexpr int AMBIENT_LIGHT_COLOR_SLIDER_MIN = 0;
constexpr int AMBIENT_LIGHT_COLOR_SLIDER_MAX = static_cast<int>(AMBIENT_LIGHT_COLOR_MAX / AMBIENT_LIGHT_COLOR_STEP);
constexpr int SHADOW_SLIDER_MIN = 0;
constexpr int SHADOW_SLIDER_MAX = static_cast<int>(SHADOW_INTENSITY_MAX / SHADOW_INTENSITY_STEP);
constexpr int SHADOW_COVERAGE_SLIDER_MIN = 0;
constexpr int SHADOW_COVERAGE_SLIDER_MAX = static_cast<int>(SHADOW_COVERAGE_MAX / SHADOW_COVERAGE_STEP);
constexpr int SHADOW_SATURATION_BOOST_SLIDER_MIN = 0;
constexpr int SHADOW_SATURATION_BOOST_SLIDER_MAX = static_cast<int>(SHADOW_SATURATION_BOOST_MAX / SHADOW_SATURATION_BOOST_STEP);
constexpr int HALF_LAMBERT_SHADOW_SATURATION_SLIDER_MIN = 0;
constexpr int HALF_LAMBERT_SHADOW_SATURATION_SLIDER_MAX = static_cast<int>(HALF_LAMBERT_SHADOW_SATURATION_MAX / HALF_LAMBERT_SHADOW_SATURATION_STEP);
constexpr int SHADOW_DARKNESS_SLIDER_MIN = 0;
constexpr int SHADOW_DARKNESS_SLIDER_MAX = static_cast<int>(SHADOW_DARKNESS_MAX / SHADOW_DARKNESS_STEP);
constexpr int SPECULAR_INTENSITY_SLIDER_MIN = 0;
constexpr int SPECULAR_INTENSITY_SLIDER_MAX = static_cast<int>(SPECULAR_INTENSITY_MAX / SPECULAR_INTENSITY_STEP);
constexpr int SPECULAR_EDGE_SLIDER_MIN = 0;
constexpr int SPECULAR_EDGE_SLIDER_MAX = static_cast<int>(SPECULAR_EDGE_MAX / SPECULAR_EDGE_STEP);
constexpr int SSS_INTENSITY_SLIDER_MIN = 0;
constexpr int SSS_INTENSITY_SLIDER_MAX = static_cast<int>(SSS_INTENSITY_MAX / SSS_INTENSITY_STEP);
constexpr int SSS_COLOR_SLIDER_MIN = 0;
constexpr int SSS_COLOR_SLIDER_MAX = static_cast<int>(SSS_COLOR_MAX / SSS_COLOR_STEP);
constexpr int SSAO_BRIGHTNESS_SLIDER_MIN = 0;
constexpr int SSAO_BRIGHTNESS_SLIDER_MAX = static_cast<int>((SSAO_BRIGHTNESS_MAX - SSAO_BRIGHTNESS_MIN) / SSAO_BRIGHTNESS_STEP);
constexpr int SSAO_SAMPLE_RADIUS_SLIDER_MIN = 0;
constexpr int SSAO_SAMPLE_RADIUS_SLIDER_MAX = static_cast<int>((SSAO_SAMPLE_RADIUS_MAX - SSAO_SAMPLE_RADIUS_MIN) / SSAO_SAMPLE_RADIUS_STEP);
constexpr int SSAO_SATURATION_BOOST_SLIDER_MIN = 0;
constexpr int SSAO_SATURATION_BOOST_SLIDER_MAX = static_cast<int>(SSAO_SATURATION_BOOST_MAX / SSAO_SATURATION_BOOST_STEP);
constexpr int BLOOM_THRESHOLD_SLIDER_MIN = 0;
constexpr int BLOOM_THRESHOLD_SLIDER_MAX = static_cast<int>(BLOOM_THRESHOLD_MAX / BLOOM_THRESHOLD_STEP);
constexpr int DOF_FOCAL_DISTANCE_SLIDER_MIN = 0;
constexpr int DOF_FOCAL_DISTANCE_SLIDER_MAX = static_cast<int>((DOF_FOCAL_DISTANCE_MAX - DOF_FOCAL_DISTANCE_MIN) / DOF_FOCAL_DISTANCE_STEP);
constexpr int DOF_MAX_BLUR_DISTANCE_SLIDER_MIN = 0;
constexpr int DOF_MAX_BLUR_DISTANCE_SLIDER_MAX = static_cast<int>((DOF_MAX_BLUR_DISTANCE_MAX - DOF_MAX_BLUR_DISTANCE_MIN) / DOF_MAX_BLUR_DISTANCE_STEP);
constexpr int DOF_AUTO_ACTIVATION_DISTANCE_SLIDER_MIN = 0;
constexpr int DOF_AUTO_ACTIVATION_DISTANCE_SLIDER_MAX = static_cast<int>((DOF_AUTO_ACTIVATION_DISTANCE_MAX - DOF_AUTO_ACTIVATION_DISTANCE_MIN) / DOF_AUTO_ACTIVATION_DISTANCE_STEP);
constexpr int STARBURST_THRESHOLD_SLIDER_MIN = 0;
constexpr int STARBURST_THRESHOLD_SLIDER_MAX = static_cast<int>(STARBURST_THRESHOLD_MAX / STARBURST_THRESHOLD_STEP);
constexpr int MODEL_LOAD_SCALE_SLIDER_MIN = 0;
constexpr int MODEL_LOAD_SCALE_SLIDER_MAX = static_cast<int>((MODEL_LOAD_SCALE_MAX - MODEL_LOAD_SCALE_MIN) / MODEL_LOAD_SCALE_STEP);
constexpr int POINT_LIGHT_COLOR_SLIDER_MIN = 0;
constexpr int POINT_LIGHT_COLOR_SLIDER_MAX = static_cast<int>(POINT_LIGHT_COLOR_MAX / POINT_LIGHT_COLOR_STEP);
constexpr int POINT_LIGHT_BRIGHTNESS_SLIDER_MIN = 0;
constexpr int POINT_LIGHT_BRIGHTNESS_SLIDER_MAX = static_cast<int>(POINT_LIGHT_BRIGHTNESS_MAX / POINT_LIGHT_BRIGHTNESS_STEP);
constexpr int GAUSSIAN_SLIDER_MIN = 1;
constexpr int GAUSSIAN_SLIDER_MAX = (GAUSSIAN_SAMPLE_MAX + 1) / 2;
constexpr int FXAA_QUALITY_SLIDER_MIN = FXAA_QUALITY_MIN;
constexpr int FXAA_QUALITY_SLIDER_MAX = FXAA_QUALITY_MAX;
constexpr int MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_SLIDER_MIN =
    static_cast<int>(MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_MIN / MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_STEP);
constexpr int MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_SLIDER_MAX =
    static_cast<int>(MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_MAX / MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_STEP);
constexpr int MOTION_BLUR_CAMERA_SAMPLE_COUNT_SLIDER_MIN = MOTION_BLUR_CAMERA_SAMPLE_COUNT_MIN;
constexpr int MOTION_BLUR_CAMERA_SAMPLE_COUNT_SLIDER_MAX = MOTION_BLUR_CAMERA_SAMPLE_COUNT_MAX;
constexpr int SHADOW_BLUR_TAP_SLIDER_MIN = 0;
constexpr int SHADOW_BLUR_TAP_SLIDER_MAX = (SHADOW_BLUR_TAP_COUNT_MAX - 1) / 2;
constexpr int GODRAY_COLOR_SLIDER_MIN = 0;
constexpr int GODRAY_COLOR_SLIDER_MAX = static_cast<int>(GODRAY_LIGHT_COLOR_MAX / GODRAY_LIGHT_COLOR_STEP);
constexpr int GODRAY_INTENSITY_SLIDER_MIN = 0;
constexpr int GODRAY_INTENSITY_SLIDER_MAX = static_cast<int>(GODRAY_INTENSITY_MAX / GODRAY_INTENSITY_STEP);
constexpr int GODRAY_VIRTUAL_PROXIMITY_SLIDER_MIN = 0;
constexpr int GODRAY_VIRTUAL_PROXIMITY_SLIDER_MAX = static_cast<int>(GODRAY_VIRTUAL_PROXIMITY_MAX / GODRAY_VIRTUAL_PROXIMITY_STEP);
constexpr int GODRAY_POS_SLIDER_MIN = static_cast<int>(GODRAY_LIGHT_POS_MIN / GODRAY_LIGHT_POS_STEP);
constexpr int GODRAY_POS_SLIDER_MAX = static_cast<int>(GODRAY_LIGHT_POS_MAX / GODRAY_LIGHT_POS_STEP);
constexpr int SETTINGS_DIALOG_CONTENT_HEIGHT_DLU = 1218;
constexpr int SETTINGS_DIALOG_WHEEL_STEP_PX = 36;
constexpr UINT ID_POPUP_EXPORT_BINARY = 60001;
constexpr UINT ID_POPUP_REMOVE_MODEL = 60002;
constexpr UINT ID_POPUP_REMOVE_POINT_LIGHT = 60003;

int g_settingsDialogScrollPos = 0;

void RefreshSaturateControls(HWND hDlg);
void RefreshDepthOfFieldControls(HWND hDlg);
void RefreshDepthOfFieldMaxBlurControls(HWND hDlg);
void RefreshDepthOfFieldAutoActivationControls(HWND hDlg);
void RefreshGaussianControls(HWND hDlg);
void RefreshFXAAControls(HWND hDlg);
void RefreshMotionBlurCameraControls(HWND hDlg);
void RefreshFogControls(HWND hDlg);
void RefreshHeightFogControls(HWND hDlg);
void RefreshHeightFogIntensityControls(HWND hDlg);
void RefreshHeightFogStartControls(HWND hDlg);
void RefreshHeightFogMaxControls(HWND hDlg);
void RefreshHeightFogDistanceStartControls(HWND hDlg);
void RefreshHeightFogDistanceMaxControls(HWND hDlg);
void RefreshSunLightIntensityControls(HWND hDlg);
void RefreshSunLightColorControls(HWND hDlg);
void RefreshAmbientLightControls(HWND hDlg);
void RefreshShadowControls(HWND hDlg);
void RefreshShadowSaturationBoostControls(HWND hDlg);
void RefreshShadowCoverageControls(HWND hDlg);
void RefreshShadowPcfTapControls(HWND hDlg);
void RefreshShadowCompositeTapControls(HWND hDlg);
void RefreshHalfLambertShadowSaturationControls(HWND hDlg);
void RefreshShadowDarknessControls(HWND hDlg);
void RefreshSpecularIntensityControls(HWND hDlg);
void RefreshSpecularEdgeControls(HWND hDlg);
void RefreshSSSControls(HWND hDlg);
void RefreshSSAOBrightnessControls(HWND hDlg);
void RefreshSSAOSampleRadiusControls(HWND hDlg);
void RefreshSSAOSaturationBoostControls(HWND hDlg);
void RefreshSSAOModeControls(HWND hDlg);
void RefreshBloomThresholdControls(HWND hDlg);
void RefreshStarBurstThresholdControls(HWND hDlg);
void RefreshModelLoadScaleControls(HWND hDlg);
void RefreshPointLightControls(HWND hDlg);
void RefreshGodRayControls(HWND hDlg);

const wchar_t* PointLightShapeToDisplayString(const NSRender::PointLightShape shape)
{
    switch (shape)
    {
    case NSRender::PointLightShape::Point:
        return L"Point";
    case NSRender::PointLightShape::Line:
        return L"Line";
    case NSRender::PointLightShape::Square:
        return L"Square";
    case NSRender::PointLightShape::Cube:
        return L"Cube";
    case NSRender::PointLightShape::Sphere:
        return L"Sphere";
    default:
        return L"Point";
    }
}

int PointLightShapeToComboIndex(const NSRender::PointLightShape shape)
{
    switch (shape)
    {
    case NSRender::PointLightShape::Point:
        return 0;
    case NSRender::PointLightShape::Line:
        return 1;
    case NSRender::PointLightShape::Square:
        return 2;
    case NSRender::PointLightShape::Cube:
        return 3;
    case NSRender::PointLightShape::Sphere:
        return 4;
    default:
        return 0;
    }
}

NSRender::PointLightShape ComboIndexToPointLightShape(const int comboIndex)
{
    switch (comboIndex)
    {
    case 1:
        return NSRender::PointLightShape::Line;
    case 2:
        return NSRender::PointLightShape::Square;
    case 3:
        return NSRender::PointLightShape::Cube;
    case 4:
        return NSRender::PointLightShape::Sphere;
    default:
        return NSRender::PointLightShape::Point;
    }
}

void PopulatePointLightTypeCombo(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_POINT_LIGHT_TYPE);
    if (combo == NULL)
    {
        return;
    }

    SendMessage(combo, CB_RESETCONTENT, 0, 0);
    SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Point"));
    SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Line"));
    SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Square"));
    SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Cube"));
    SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sphere"));
}

void PopulateSSAOModeCombo(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SSAO_MODE);
    if (combo == NULL)
    {
        return;
    }

    SendMessage(combo, CB_RESETCONTENT, 0, 0);
    SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Legacy"));
    SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"SSAO2"));
}

bool TryParseEditFloat(HWND hDlg, const int controlId, float& value)
{
    wchar_t buffer[64] { };
    GetDlgItemText(hDlg, controlId, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));

    try
    {
        value = std::stof(buffer);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool TryParseEditInt(HWND hDlg, const int controlId, int& value)
{
    wchar_t buffer[64] { };
    GetDlgItemText(hDlg, controlId, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));

    try
    {
        value = std::stoi(buffer);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void InitializeEditableNumericFields(HWND hDlg)
{
    const int controlIds[] =
    {
        IDC_EDIT_SATURATE_LEVEL,
        IDC_EDIT_DOF_FOCAL_DISTANCE,
        IDC_EDIT_DOF_MAX_BLUR_DISTANCE,
        IDC_EDIT_DOF_AUTO_ACTIVATION_DISTANCE,
        IDC_EDIT_GAUSSIAN_SAMPLE_SIZE,
        IDC_EDIT_FXAA_QUALITY,
        IDC_EDIT_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS,
        IDC_EDIT_MOTION_BLUR_CAMERA_SAMPLE_COUNT,
        IDC_EDIT_FOG_INTENSITY,
        IDC_EDIT_HEIGHT_FOG_INTENSITY,
        IDC_EDIT_HEIGHT_FOG_START,
        IDC_EDIT_HEIGHT_FOG_MAX,
        IDC_EDIT_HEIGHT_FOG_DISTANCE_START,
        IDC_EDIT_HEIGHT_FOG_DISTANCE_MAX,
        IDC_EDIT_SUN_LIGHT_INTENSITY,
        IDC_EDIT_SUN_LIGHT_COLOR_R,
        IDC_EDIT_SUN_LIGHT_COLOR_G,
        IDC_EDIT_SUN_LIGHT_COLOR_B,
        IDC_EDIT_AMBIENT_LIGHT_INTENSITY,
        IDC_EDIT_AMBIENT_LIGHT_COLOR_R,
        IDC_EDIT_AMBIENT_LIGHT_COLOR_G,
        IDC_EDIT_AMBIENT_LIGHT_COLOR_B,
        IDC_EDIT_SHADOW_INTENSITY,
        IDC_EDIT_SHADOW_SATURATION_BOOST,
        IDC_EDIT_SHADOW_COVERAGE,
        IDC_EDIT_SHADOW_PCF_TAPS,
        IDC_EDIT_SHADOW_COMPOSITE_TAPS,
        IDC_EDIT_HALF_LAMBERT_SHADOW_SATURATION,
        IDC_EDIT_SHADOW_DARKNESS,
        IDC_EDIT_SPECULAR_INTENSITY,
        IDC_EDIT_SPECULAR_EDGE,
        IDC_EDIT_SSS_INTENSITY,
        IDC_EDIT_SSS_COLOR_R,
        IDC_EDIT_SSS_COLOR_G,
        IDC_EDIT_SSS_COLOR_B,
        IDC_EDIT_SSAO_BRIGHTNESS,
        IDC_EDIT_SSAO_SAMPLE_RADIUS,
        IDC_EDIT_SSAO_SATURATION_BOOST,
        IDC_EDIT_BLOOM_THRESHOLD,
        IDC_EDIT_STARBURST_THRESHOLD,
        IDC_EDIT_MODEL_LOAD_SCALE,
        IDC_EDIT_POINT_LIGHT_COLOR_R,
        IDC_EDIT_POINT_LIGHT_COLOR_G,
        IDC_EDIT_POINT_LIGHT_COLOR_B,
        IDC_EDIT_POINT_LIGHT_BRIGHTNESS,
        IDC_EDIT_POINT_LIGHT_LINE_LENGTH,
        IDC_EDIT_POINT_LIGHT_SQUARE_WIDTH,
        IDC_EDIT_POINT_LIGHT_SQUARE_HEIGHT,
        IDC_EDIT_POINT_LIGHT_ROT_X,
        IDC_EDIT_POINT_LIGHT_ROT_Y,
        IDC_EDIT_POINT_LIGHT_ROT_Z,
        IDC_EDIT_GODRAY_COLOR_R,
        IDC_EDIT_GODRAY_COLOR_G,
        IDC_EDIT_GODRAY_COLOR_B,
        IDC_EDIT_GODRAY_INTENSITY,
        IDC_EDIT_GODRAY_VIRTUAL_PROXIMITY,
        IDC_EDIT_GODRAY_POS_X,
        IDC_EDIT_GODRAY_POS_Y,
        IDC_EDIT_GODRAY_POS_Z,
    };

    for (const int controlId : controlIds)
    {
        SendDlgItemMessage(hDlg, controlId, EM_SETREADONLY, FALSE, 0);
    }
}

bool HandleNumericEditCommit(HWND hDlg, const WORD commandId)
{
    float floatValue = 0.0f;
    int intValue = 0;

    switch (commandId)
    {
    case IDC_EDIT_SATURATE_LEVEL:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_saturateLevel = floatValue;
            ApplySaturateLevel();
        }
        RefreshSaturateControls(hDlg);
        return true;
    case IDC_EDIT_DOF_FOCAL_DISTANCE:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_dofFocalDistance = floatValue;
            ApplyDepthOfFieldFocalDistance();
        }
        RefreshDepthOfFieldControls(hDlg);
        return true;
    case IDC_EDIT_DOF_MAX_BLUR_DISTANCE:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_dofMaxBlurDistance = floatValue;
            ApplyDepthOfFieldMaxBlurDistance();
        }
        RefreshDepthOfFieldMaxBlurControls(hDlg);
        return true;
    case IDC_EDIT_DOF_AUTO_ACTIVATION_DISTANCE:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_dofAutoActivationDistance = floatValue;
            ApplyDepthOfFieldAutoActivationDistance();
        }
        RefreshDepthOfFieldAutoActivationControls(hDlg);
        return true;
    case IDC_EDIT_GAUSSIAN_SAMPLE_SIZE:
        if (TryParseEditInt(hDlg, commandId, intValue))
        {
            g_gaussianSampleSize = intValue;
            ApplyGaussianSampleSize();
        }
        RefreshGaussianControls(hDlg);
        return true;
    case IDC_EDIT_FXAA_QUALITY:
        if (TryParseEditInt(hDlg, commandId, intValue))
        {
            g_fxaaQuality = intValue;
            ApplyFXAAQuality();
        }
        RefreshFXAAControls(hDlg);
        return true;
    case IDC_EDIT_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_motionBlurCameraMaxBlurPixels = floatValue;
            ApplyMotionBlurCameraSettings();
        }
        RefreshMotionBlurCameraControls(hDlg);
        return true;
    case IDC_EDIT_MOTION_BLUR_CAMERA_SAMPLE_COUNT:
        if (TryParseEditInt(hDlg, commandId, intValue))
        {
            g_motionBlurCameraSampleCount = intValue;
            ApplyMotionBlurCameraSettings();
        }
        RefreshMotionBlurCameraControls(hDlg);
        return true;
    case IDC_EDIT_FOG_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_fogIntensity = floatValue;
            ApplyFogIntensity();
        }
        RefreshFogControls(hDlg);
        return true;
    case IDC_EDIT_HEIGHT_FOG_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_heightFogIntensity = floatValue;
            ApplyHeightFogIntensity();
        }
        RefreshHeightFogIntensityControls(hDlg);
        return true;
    case IDC_EDIT_HEIGHT_FOG_START:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_heightFogStart = floatValue;
            ApplyHeightFogStart();
        }
        RefreshHeightFogStartControls(hDlg);
        return true;
    case IDC_EDIT_HEIGHT_FOG_MAX:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_heightFogMax = floatValue;
            ApplyHeightFogMax();
        }
        RefreshHeightFogMaxControls(hDlg);
        return true;
    case IDC_EDIT_HEIGHT_FOG_DISTANCE_START:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_heightFogDistanceStart = floatValue;
            ApplyHeightFogDistanceStart();
        }
        RefreshHeightFogDistanceStartControls(hDlg);
        return true;
    case IDC_EDIT_HEIGHT_FOG_DISTANCE_MAX:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_heightFogDistanceMax = floatValue;
            ApplyHeightFogDistanceMax();
        }
        RefreshHeightFogDistanceMaxControls(hDlg);
        return true;
    case IDC_EDIT_SUN_LIGHT_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sunLightIntensity = floatValue;
            ApplySunLightIntensity();
        }
        RefreshSunLightIntensityControls(hDlg);
        return true;
    case IDC_EDIT_SUN_LIGHT_COLOR_R:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sunLightColor.r = floatValue;
            ApplySunLightColor();
        }
        RefreshSunLightColorControls(hDlg);
        return true;
    case IDC_EDIT_SUN_LIGHT_COLOR_G:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sunLightColor.g = floatValue;
            ApplySunLightColor();
        }
        RefreshSunLightColorControls(hDlg);
        return true;
    case IDC_EDIT_SUN_LIGHT_COLOR_B:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sunLightColor.b = floatValue;
            ApplySunLightColor();
        }
        RefreshSunLightColorControls(hDlg);
        return true;
    case IDC_EDIT_AMBIENT_LIGHT_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_ambientLightIntensity = floatValue;
            ApplyAmbientLightIntensity();
        }
        RefreshAmbientLightControls(hDlg);
        return true;
    case IDC_EDIT_AMBIENT_LIGHT_COLOR_R:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_ambientLightColor.r = floatValue;
            ApplyAmbientLightColor();
        }
        RefreshAmbientLightControls(hDlg);
        return true;
    case IDC_EDIT_AMBIENT_LIGHT_COLOR_G:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_ambientLightColor.g = floatValue;
            ApplyAmbientLightColor();
        }
        RefreshAmbientLightControls(hDlg);
        return true;
    case IDC_EDIT_AMBIENT_LIGHT_COLOR_B:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_ambientLightColor.b = floatValue;
            ApplyAmbientLightColor();
        }
        RefreshAmbientLightControls(hDlg);
        return true;
    case IDC_EDIT_SHADOW_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_shadowIntensity = floatValue;
            ApplyShadowIntensity();
        }
        RefreshShadowControls(hDlg);
        return true;
    case IDC_EDIT_SHADOW_SATURATION_BOOST:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_shadowSaturationBoost = floatValue;
            ApplyShadowSaturationBoost();
        }
        RefreshShadowSaturationBoostControls(hDlg);
        return true;
    case IDC_EDIT_SHADOW_COVERAGE:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_shadowCoverage = floatValue;
            ApplyShadowCoverage();
        }
        RefreshShadowCoverageControls(hDlg);
        return true;
    case IDC_EDIT_SHADOW_PCF_TAPS:
        if (TryParseEditInt(hDlg, commandId, intValue))
        {
            g_shadowPcfTapCount = intValue;
            ApplyShadowPcfTapCount();
        }
        RefreshShadowPcfTapControls(hDlg);
        return true;
    case IDC_EDIT_SHADOW_COMPOSITE_TAPS:
        if (TryParseEditInt(hDlg, commandId, intValue))
        {
            g_shadowCompositeTapCount = intValue;
            ApplyShadowCompositeTapCount();
        }
        RefreshShadowCompositeTapControls(hDlg);
        return true;
    case IDC_EDIT_HALF_LAMBERT_SHADOW_SATURATION:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_halfLambertShadowSaturation = floatValue;
            ApplyHalfLambertShadowSaturation();
        }
        RefreshHalfLambertShadowSaturationControls(hDlg);
        return true;
    case IDC_EDIT_SHADOW_DARKNESS:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_shadowDarkness = floatValue;
            ApplyShadowDarkness();
        }
        RefreshShadowDarknessControls(hDlg);
        return true;
    case IDC_EDIT_SPECULAR_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_specularIntensity = floatValue;
            ApplySpecularIntensity();
        }
        RefreshSpecularIntensityControls(hDlg);
        return true;
    case IDC_EDIT_SPECULAR_EDGE:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_specularEdge = floatValue;
            ApplySpecularEdge();
        }
        RefreshSpecularEdgeControls(hDlg);
        return true;
    case IDC_EDIT_SSS_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sssIntensity = floatValue;
            ApplySSSIntensity();
        }
        RefreshSSSControls(hDlg);
        return true;
    case IDC_EDIT_SSS_COLOR_R:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sssColor.r = floatValue;
            ApplySSSColor();
        }
        RefreshSSSControls(hDlg);
        return true;
    case IDC_EDIT_SSS_COLOR_G:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sssColor.g = floatValue;
            ApplySSSColor();
        }
        RefreshSSSControls(hDlg);
        return true;
    case IDC_EDIT_SSS_COLOR_B:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_sssColor.b = floatValue;
            ApplySSSColor();
        }
        RefreshSSSControls(hDlg);
        return true;
    case IDC_EDIT_SSAO_BRIGHTNESS:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_ssaoBrightness = floatValue;
            ApplySSAOBrightness();
        }
        RefreshSSAOBrightnessControls(hDlg);
        return true;
    case IDC_EDIT_SSAO_SAMPLE_RADIUS:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_ssaoSampleRadius = floatValue;
            ApplySSAOSampleRadius();
        }
        RefreshSSAOSampleRadiusControls(hDlg);
        return true;
    case IDC_EDIT_SSAO_SATURATION_BOOST:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_ssaoSaturationBoost = floatValue;
            ApplySSAOSaturationBoost();
        }
        RefreshSSAOSaturationBoostControls(hDlg);
        return true;
    case IDC_EDIT_BLOOM_THRESHOLD:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_bloomThreshold = floatValue;
            ApplyBloomThreshold();
        }
        RefreshBloomThresholdControls(hDlg);
        return true;
    case IDC_EDIT_STARBURST_THRESHOLD:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_starBurstThreshold = floatValue;
            ApplyStarBurstThreshold();
        }
        RefreshStarBurstThresholdControls(hDlg);
        return true;
    case IDC_EDIT_MODEL_LOAD_SCALE:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_modelLoadScale = floatValue;
            ApplyModelLoadScale();
        }
        RefreshModelLoadScaleControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_COLOR_R:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightColor.r = floatValue;
            ApplyPointLightColor();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_COLOR_G:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightColor.g = floatValue;
            ApplyPointLightColor();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_COLOR_B:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightColor.b = floatValue;
            ApplyPointLightColor();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_BRIGHTNESS:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightBrightness = floatValue;
            ApplyPointLightBrightness();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_LINE_LENGTH:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightLineLength = floatValue;
            ApplyPointLightLineSettings();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_SQUARE_WIDTH:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightSquareWidth = floatValue;
            ApplyPointLightSquareSettings();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_SQUARE_HEIGHT:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightSquareHeight = floatValue;
            ApplyPointLightSquareSettings();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_ROT_X:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightRotationDegrees.x = floatValue;
            ApplyPointLightLineSettings();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_ROT_Y:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightRotationDegrees.y = floatValue;
            ApplyPointLightLineSettings();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_POINT_LIGHT_ROT_Z:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_pointLightRotationDegrees.z = floatValue;
            ApplyPointLightLineSettings();
        }
        RefreshPointLightControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_COLOR_R:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayLightColor.x = floatValue;
            ApplyGodRayLightColor();
        }
        RefreshGodRayControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_COLOR_G:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayLightColor.y = floatValue;
            ApplyGodRayLightColor();
        }
        RefreshGodRayControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_COLOR_B:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayLightColor.z = floatValue;
            ApplyGodRayLightColor();
        }
        RefreshGodRayControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_INTENSITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayIntensity = floatValue;
            ApplyGodRayIntensity();
        }
        RefreshGodRayControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_VIRTUAL_PROXIMITY:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayVirtualProximityStrength = floatValue;
            ApplyGodRayVirtualProximityStrength();
        }
        RefreshGodRayControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_POS_X:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayLightPos.x = floatValue;
            ApplyGodRayLightPos();
        }
        RefreshGodRayControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_POS_Y:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayLightPos.y = floatValue;
            ApplyGodRayLightPos();
        }
        RefreshGodRayControls(hDlg);
        return true;
    case IDC_EDIT_GODRAY_POS_Z:
        if (TryParseEditFloat(hDlg, commandId, floatValue))
        {
            g_godRayLightPos.z = floatValue;
            ApplyGodRayLightPos();
        }
        RefreshGodRayControls(hDlg);
        return true;
    default:
        return false;
    }
}

std::wstring FormatResolutionLabel(const int width, const int height)
{
    return std::to_wstring(width) + L" x " + std::to_wstring(height);
}

std::wstring FormatLoadedModelScale(const float scale)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", scale);
    return buffer;
}

std::wstring FormatLoadedModelPos(const D3DXVECTOR3& pos)
{
    wchar_t buffer[96];
    std::swprintf(buffer,
                  sizeof(buffer) / sizeof(buffer[0]),
                  L"(%.1f, %.1f, %.1f)",
                  pos.x,
                  pos.y,
                  pos.z);
    return buffer;
}

std::wstring FormatPointLightColor(const D3DXCOLOR& color)
{
    wchar_t buffer[96];
    std::swprintf(buffer,
                  sizeof(buffer) / sizeof(buffer[0]),
                  L"(%.2f, %.2f, %.2f)",
                  color.r,
                  color.g,
                  color.b);
    return buffer;
}

void MoveDialogToRightOfParent(HWND hDlg)
{
    HWND parent = GetParent(hDlg);
    if (parent == NULL)
    {
        return;
    }

    RECT parentRect { };
    RECT dialogRect { };

    if (!GetWindowRect(parent, &parentRect) || !GetWindowRect(hDlg, &dialogRect))
    {
        return;
    }

    const int dialogW = dialogRect.right - dialogRect.left;
    const int dialogH = dialogRect.bottom - dialogRect.top;
    const int gap = 8;

    SetWindowPos(hDlg,
                 NULL,
                 parentRect.right + gap,
                 parentRect.top,
                 dialogW,
                 dialogH,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

int GetSettingsDialogContentHeightPx(HWND hDlg)
{
    RECT dialogUnits { 0, 0, 0, SETTINGS_DIALOG_CONTENT_HEIGHT_DLU };
    MapDialogRect(hDlg, &dialogUnits);
    return dialogUnits.bottom - dialogUnits.top;
}

int GetSettingsDialogClientHeightPx(HWND hDlg)
{
    RECT clientRect { };
    GetClientRect(hDlg, &clientRect);
    return clientRect.bottom - clientRect.top;
}

void UpdateSettingsDialogScrollBar(HWND hDlg)
{
    const int contentHeightPx = GetSettingsDialogContentHeightPx(hDlg);
    const int clientHeightPx = GetSettingsDialogClientHeightPx(hDlg);
    const int maxScroll = (std::max)(0, contentHeightPx - clientHeightPx);
    g_settingsDialogScrollPos = (std::max)(0, (std::min)(g_settingsDialogScrollPos, maxScroll));

    SCROLLINFO scrollInfo { };
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = contentHeightPx - 1;
    scrollInfo.nPage = static_cast<UINT>(clientHeightPx);
    scrollInfo.nPos = g_settingsDialogScrollPos;
    SetScrollInfo(hDlg, SB_VERT, &scrollInfo, TRUE);
}

void ScrollSettingsDialogTo(HWND hDlg, const int newPos)
{
    const int contentHeightPx = GetSettingsDialogContentHeightPx(hDlg);
    const int clientHeightPx = GetSettingsDialogClientHeightPx(hDlg);
    const int maxScroll = (std::max)(0, contentHeightPx - clientHeightPx);
    const int clampedPos = (std::max)(0, (std::min)(newPos, maxScroll));
    const int delta = g_settingsDialogScrollPos - clampedPos;
    if (delta == 0)
    {
        UpdateSettingsDialogScrollBar(hDlg);
        return;
    }

    g_settingsDialogScrollPos = clampedPos;
    ScrollWindowEx(hDlg,
                   0,
                   delta,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN);
    UpdateWindow(hDlg);
    UpdateSettingsDialogScrollBar(hDlg);
}

void RefreshSaturateControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_saturateLevel);
    SetDlgItemText(hDlg, IDC_EDIT_SATURATE_LEVEL, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SATURATE_LEVEL,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SaturateLevelToSliderValue(g_saturateLevel)));
}

void RefreshSelectedMeshPaths(HWND hDlg)
{
    SetDlgItemText(hDlg, IDC_EDIT_MIX_MESH_PATH, g_selectedMixMeshPath.c_str());
    SetDlgItemText(hDlg, IDC_EDIT_MESH_PATH, g_selectedMeshPath.c_str());
    SetDlgItemText(hDlg, IDC_EDIT_ANIM_MESH_PATH, g_selectedAnimMeshPath.c_str());
    SetDlgItemText(hDlg, IDC_EDIT_SKIN_ANIM_MESH_PATH, g_selectedSkinAnimMeshPath.c_str());
    SetDlgItemText(hDlg, IDC_EDIT_MIX_SKIN_ANIM_MESH_PATH, g_selectedMixSkinAnimMeshPath.c_str());
    SetDlgItemText(hDlg, IDC_EDIT_MASKED_GAUSSIAN_MASK_PATH, g_selectedMaskedGaussianMaskPath.c_str());
}

void RefreshMixMeshShaderMode(HWND hDlg)
{
    CheckDlgButton(hDlg,
                   IDC_RADIO_MIX_MESH_NONE,
                   (g_mixMeshShaderMode == MixMeshShaderMode::None) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg,
                   IDC_RADIO_MIX_MESH_POM,
                   (g_mixMeshShaderMode == MixMeshShaderMode::ParallaxOcclusionMapping) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg,
                   IDC_RADIO_MIX_MESH_NORMAL_MAP,
                   (g_mixMeshShaderMode == MixMeshShaderMode::NormalMapping) ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshResolutionControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_RESOLUTION);
    if (combo != NULL)
    {
        const std::wstring targetText = FormatResolutionLabel(g_resolutionWidth, g_resolutionHeight);
        const int count = static_cast<int>(SendMessage(combo, CB_GETCOUNT, 0, 0));
        for (int i = 0; i < count; ++i)
        {
            wchar_t buffer[64] { };
            SendMessage(combo, CB_GETLBTEXT, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(buffer));
            if (targetText == buffer)
            {
                SendMessage(combo, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
                break;
            }
        }
    }

    CheckDlgButton(hDlg,
                   IDC_RADIO_WINDOW_MODE_WINDOW,
                   (g_windowMode == NSRender::eWindowMode::WINDOW) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg,
                   IDC_RADIO_WINDOW_MODE_BORDERLESS,
                   (g_windowMode == NSRender::eWindowMode::BORDERLESS) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg,
                   IDC_RADIO_WINDOW_MODE_FULLSCREEN,
                   (g_windowMode == NSRender::eWindowMode::FULLSCREEN) ? BST_CHECKED : BST_UNCHECKED);
}

void PopulateResolutionCombo(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_RESOLUTION);
    if (combo == NULL)
    {
        return;
    }

    SendMessage(combo, CB_RESETCONTENT, 0, 0);

    const auto resolutionList = g_Render.GetResolutionList();
    for (const auto& resolution : resolutionList)
    {
        const std::wstring label = FormatResolutionLabel(resolution.first, resolution.second);
        SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
}

int GetListViewIndexFromPoint(HWND listView, POINT screenPoint)
{
    if (listView == NULL)
    {
        return -1;
    }

    if (screenPoint.x == -1 && screenPoint.y == -1)
    {
        return ListView_GetNextItem(listView, -1, LVNI_SELECTED);
    }

    POINT clientPoint = screenPoint;
    ScreenToClient(listView, &clientPoint);

    LVHITTESTINFO hitInfo { };
    hitInfo.pt = clientPoint;
    const int itemIndex = ListView_SubItemHitTest(listView, &hitInfo);
    if (itemIndex >= 0)
    {
        ListView_SetItemState(listView,
                              itemIndex,
                              LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
    }

    return itemIndex;
}

bool ShowLoadedModelContextMenu(HWND hDlg, HWND listView, POINT screenPoint)
{
    const int itemIndex = GetListViewIndexFromPoint(listView, screenPoint);
    if (itemIndex < 0)
    {
        return false;
    }

    if (screenPoint.x == -1 && screenPoint.y == -1)
    {
        RECT itemRect { };
        itemRect.left = LVIR_BOUNDS;
        if (ListView_GetItemRect(listView, itemIndex, &itemRect, LVIR_BOUNDS))
        {
            POINT popupPoint { itemRect.left, itemRect.top };
            ClientToScreen(listView, &popupPoint);
            screenPoint = popupPoint;
        }
    }

    HMENU popupMenu = CreatePopupMenu();
    if (popupMenu == NULL)
    {
        return false;
    }

    AppendMenuW(popupMenu, MF_STRING, ID_POPUP_EXPORT_BINARY, L"Export_Binary");
    AppendMenuW(popupMenu, MF_STRING, ID_POPUP_REMOVE_MODEL, L"Remove");

    const UINT command = TrackPopupMenu(popupMenu,
                                        TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                        screenPoint.x,
                                        screenPoint.y,
                                        0,
                                        hDlg,
                                        NULL);

    DestroyMenu(popupMenu);

    if (command == ID_POPUP_REMOVE_MODEL)
    {
        if (!RemoveLoadedModel(static_cast<size_t>(itemIndex)))
        {
            MessageBoxW(hDlg,
                        L"Model remove failed.",
                        L"Remove",
                        MB_ICONERROR | MB_OK);
        }
        return true;
    }

    if (command != ID_POPUP_EXPORT_BINARY)
    {
        return false;
    }

    std::wstring outputPath;
    if (!ShowSaveBinaryXFileDialog(hDlg, g_loadedModelList.at(itemIndex).m_path, outputPath))
    {
        return true;
    }

    if (!ExportLoadedModelAsBinaryX(static_cast<size_t>(itemIndex), outputPath))
    {
        MessageBoxW(hDlg,
                    L"Binary X export failed.",
                    L"Export_Binary",
                    MB_ICONERROR | MB_OK);
        return true;
    }

    MessageBoxW(hDlg,
                L"Binary X export completed.",
                L"Export_Binary",
                MB_ICONINFORMATION | MB_OK);
    return true;
}

bool ShowPointLightContextMenu(HWND hDlg, HWND listView, POINT screenPoint)
{
    const int itemIndex = GetListViewIndexFromPoint(listView, screenPoint);
    if (itemIndex < 0)
    {
        return false;
    }

    if (screenPoint.x == -1 && screenPoint.y == -1)
    {
        RECT itemRect { };
        itemRect.left = LVIR_BOUNDS;
        if (ListView_GetItemRect(listView, itemIndex, &itemRect, LVIR_BOUNDS))
        {
            POINT popupPoint { itemRect.left, itemRect.top };
            ClientToScreen(listView, &popupPoint);
            screenPoint = popupPoint;
        }
    }

    HMENU popupMenu = CreatePopupMenu();
    if (popupMenu == NULL)
    {
        return false;
    }

    AppendMenuW(popupMenu, MF_STRING, ID_POPUP_REMOVE_POINT_LIGHT, L"Remove");

    const UINT command = TrackPopupMenu(popupMenu,
                                        TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                        screenPoint.x,
                                        screenPoint.y,
                                        0,
                                        hDlg,
                                        NULL);

    DestroyMenu(popupMenu);

    if (command != ID_POPUP_REMOVE_POINT_LIGHT)
    {
        return false;
    }

    if (!NSRender::Light::RemovePointLight(static_cast<size_t>(itemIndex)))
    {
        MessageBoxW(hDlg,
                    L"Point light remove failed.",
                    L"Remove",
                    MB_ICONERROR | MB_OK);
        return true;
    }

    RefreshSettingsDialogState();
    return true;
}

void InitializeLoadedModelListView(HWND hDlg)
{
    HWND listView = GetDlgItem(hDlg, IDC_LIST_LOADED_MODELS);
    if (listView == NULL)
    {
        return;
    }

    ListView_SetExtendedListViewStyle(listView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMN column { };
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    column.cx = 70;
    column.pszText = const_cast<LPWSTR>(L"Type");
    ListView_InsertColumn(listView, 0, &column);

    column.cx = 110;
    column.pszText = const_cast<LPWSTR>(L"File");
    ListView_InsertColumn(listView, 1, &column);

    column.cx = 40;
    column.pszText = const_cast<LPWSTR>(L"Scale");
    ListView_InsertColumn(listView, 2, &column);

    column.cx = 120;
    column.pszText = const_cast<LPWSTR>(L"Pos");
    ListView_InsertColumn(listView, 3, &column);
}

void InitializePointLightListView(HWND hDlg)
{
    HWND listView = GetDlgItem(hDlg, IDC_LIST_POINT_LIGHTS);
    if (listView == NULL)
    {
        return;
    }

    ListView_SetExtendedListViewStyle(listView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMN column { };
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    column.cx = 86;
    column.pszText = const_cast<LPWSTR>(L"Pos");
    ListView_InsertColumn(listView, 0, &column);

    column.cx = 56;
    column.pszText = const_cast<LPWSTR>(L"Type");
    ListView_InsertColumn(listView, 1, &column);

    column.cx = 84;
    column.pszText = const_cast<LPWSTR>(L"Color");
    ListView_InsertColumn(listView, 2, &column);

    column.cx = 54;
    column.pszText = const_cast<LPWSTR>(L"Brightness");
    ListView_InsertColumn(listView, 3, &column);
}

void RefreshLoadedModelListView(HWND hDlg)
{
    HWND listView = GetDlgItem(hDlg, IDC_LIST_LOADED_MODELS);
    if (listView == NULL)
    {
        return;
    }

    ListView_DeleteAllItems(listView);

    for (int i = 0; i < static_cast<int>(g_loadedModelList.size()); ++i)
    {
        const auto& model = g_loadedModelList.at(i);

        LVITEM item { };
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(model.m_type.c_str());
        ListView_InsertItem(listView, &item);

        ListView_SetItemText(listView, i, 1, const_cast<LPWSTR>(model.m_path.c_str()));

        std::wstring scaleText = FormatLoadedModelScale(model.m_scale);
        ListView_SetItemText(listView, i, 2, const_cast<LPWSTR>(scaleText.c_str()));

        std::wstring posText = FormatLoadedModelPos(model.m_pos);
        ListView_SetItemText(listView, i, 3, const_cast<LPWSTR>(posText.c_str()));
    }
}

void RefreshPointLightListView(HWND hDlg)
{
    HWND listView = GetDlgItem(hDlg, IDC_LIST_POINT_LIGHTS);
    if (listView == NULL)
    {
        return;
    }

    ListView_DeleteAllItems(listView);

    const auto pointLightList = NSRender::Light::GetPointLightList();
    for (int i = 0; i < static_cast<int>(pointLightList.size()); ++i)
    {
        const auto& pointLight = pointLightList.at(i);

        LVITEM item { };
        item.mask = LVIF_TEXT;
        item.iItem = i;

        std::wstring posText = FormatLoadedModelPos(pointLight.m_pos);
        item.pszText = const_cast<LPWSTR>(posText.c_str());
        ListView_InsertItem(listView, &item);

        std::wstring typeText = PointLightShapeToDisplayString(pointLight.m_shape);
        ListView_SetItemText(listView, i, 1, const_cast<LPWSTR>(typeText.c_str()));

        std::wstring colorText = FormatPointLightColor(pointLight.m_color);
        ListView_SetItemText(listView, i, 2, const_cast<LPWSTR>(colorText.c_str()));

        wchar_t brightnessBuffer[32];
        std::swprintf(brightnessBuffer,
                      sizeof(brightnessBuffer) / sizeof(brightnessBuffer[0]),
                      L"%.2f",
                      pointLight.m_brightness);
        ListView_SetItemText(listView, i, 3, brightnessBuffer);
    }
}

void RefreshPointLightControls(HWND hDlg)
{
    wchar_t buffer[32];
    const bool isLineLight = (g_pointLightShape == NSRender::PointLightShape::Line);
    const bool isSquareLight = (g_pointLightShape == NSRender::PointLightShape::Square);
    const bool usesRotation = isLineLight || isSquareLight;

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_pointLightColor.r);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_COLOR_R, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_POINT_LIGHT_COLOR_R,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PointLightColorToSliderValue(g_pointLightColor.r)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_pointLightColor.g);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_COLOR_G, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_POINT_LIGHT_COLOR_G,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PointLightColorToSliderValue(g_pointLightColor.g)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_pointLightColor.b);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_COLOR_B, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_POINT_LIGHT_COLOR_B,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PointLightColorToSliderValue(g_pointLightColor.b)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_pointLightBrightness);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_BRIGHTNESS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_POINT_LIGHT_BRIGHTNESS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PointLightBrightnessToSliderValue(g_pointLightBrightness)));

    HWND combo = GetDlgItem(hDlg, IDC_COMBO_POINT_LIGHT_TYPE);
    if (combo != NULL)
    {
        SendMessage(combo, CB_SETCURSEL, PointLightShapeToComboIndex(g_pointLightShape), 0);
    }

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_pointLightLineLength);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_LINE_LENGTH, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_pointLightSquareWidth);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_SQUARE_WIDTH, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_pointLightSquareHeight);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_SQUARE_HEIGHT, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_pointLightRotationDegrees.x);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_ROT_X, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_pointLightRotationDegrees.y);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_ROT_Y, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_pointLightRotationDegrees.z);
    SetDlgItemText(hDlg, IDC_EDIT_POINT_LIGHT_ROT_Z, buffer);

    const int lineControlIds[] =
    {
        IDC_STATIC_POINT_LIGHT_LINE_LENGTH_LABEL,
        IDC_EDIT_POINT_LIGHT_LINE_LENGTH,
    };

    for (const int controlId : lineControlIds)
    {
        HWND control = GetDlgItem(hDlg, controlId);
        ShowWindow(control, isLineLight ? SW_SHOW : SW_HIDE);
        EnableWindow(control, isLineLight ? TRUE : FALSE);
    }

    const int rotationControlIds[] =
    {
        IDC_STATIC_POINT_LIGHT_ROT_X_LABEL,
        IDC_EDIT_POINT_LIGHT_ROT_X,
        IDC_STATIC_POINT_LIGHT_ROT_Y_LABEL,
        IDC_EDIT_POINT_LIGHT_ROT_Y,
        IDC_STATIC_POINT_LIGHT_ROT_Z_LABEL,
        IDC_EDIT_POINT_LIGHT_ROT_Z,
    };

    for (const int controlId : rotationControlIds)
    {
        HWND control = GetDlgItem(hDlg, controlId);
        ShowWindow(control, usesRotation ? SW_SHOW : SW_HIDE);
        EnableWindow(control, usesRotation ? TRUE : FALSE);
    }

    const int squareControlIds[] =
    {
        IDC_STATIC_POINT_LIGHT_SQUARE_WIDTH_LABEL,
        IDC_EDIT_POINT_LIGHT_SQUARE_WIDTH,
        IDC_STATIC_POINT_LIGHT_SQUARE_HEIGHT_LABEL,
        IDC_EDIT_POINT_LIGHT_SQUARE_HEIGHT,
    };

    for (const int controlId : squareControlIds)
    {
        HWND control = GetDlgItem(hDlg, controlId);
        ShowWindow(control, isSquareLight ? SW_SHOW : SW_HIDE);
        EnableWindow(control, isSquareLight ? TRUE : FALSE);
    }
}

void RefreshFogControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_fogIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_FOG_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FOG_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FogIntensityToSliderValue(g_fogIntensity)));
}

void RefreshHeightFogControls(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_HEIGHT_FOG, g_bHeightFog ? BST_CHECKED : BST_UNCHECKED);
    RefreshHeightFogIntensityControls(hDlg);
    RefreshHeightFogStartControls(hDlg);
    RefreshHeightFogMaxControls(hDlg);
    RefreshHeightFogDistanceStartControls(hDlg);
    RefreshHeightFogDistanceMaxControls(hDlg);
}

void RefreshHeightFogIntensityControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_heightFogIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_HEIGHT_FOG_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_HEIGHT_FOG_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(HeightFogIntensityToSliderValue(g_heightFogIntensity)));
}

void RefreshHeightFogStartControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_heightFogStart);
    SetDlgItemText(hDlg, IDC_EDIT_HEIGHT_FOG_START, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_HEIGHT_FOG_START,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(HeightFogHeightToSliderValue(g_heightFogStart)));
}

void RefreshHeightFogMaxControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_heightFogMax);
    SetDlgItemText(hDlg, IDC_EDIT_HEIGHT_FOG_MAX, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_HEIGHT_FOG_MAX,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(HeightFogHeightToSliderValue(g_heightFogMax)));
}

void RefreshHeightFogDistanceStartControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_heightFogDistanceStart);
    SetDlgItemText(hDlg, IDC_EDIT_HEIGHT_FOG_DISTANCE_START, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_HEIGHT_FOG_DISTANCE_START,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(HeightFogDistanceToSliderValue(g_heightFogDistanceStart)));
}

void RefreshHeightFogDistanceMaxControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_heightFogDistanceMax);
    SetDlgItemText(hDlg, IDC_EDIT_HEIGHT_FOG_DISTANCE_MAX, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_HEIGHT_FOG_DISTANCE_MAX,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(HeightFogDistanceToSliderValue(g_heightFogDistanceMax)));
}

void RefreshShadowControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_shadowIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_SHADOW_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SHADOW_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(ShadowIntensityToSliderValue(g_shadowIntensity)));
}

void RefreshShadowCoverageControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_shadowCoverage);
    SetDlgItemText(hDlg, IDC_EDIT_SHADOW_COVERAGE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SHADOW_COVERAGE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(ShadowCoverageToSliderValue(g_shadowCoverage)));
}

void RefreshSunLightIntensityControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_sunLightIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_SUN_LIGHT_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SUN_LIGHT_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SunLightIntensityToSliderValue(g_sunLightIntensity)));
}

void RefreshSunLightColorControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_sunLightColor.r);
    SetDlgItemText(hDlg, IDC_EDIT_SUN_LIGHT_COLOR_R, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SUN_LIGHT_COLOR_R,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SunLightColorToSliderValue(g_sunLightColor.r)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_sunLightColor.g);
    SetDlgItemText(hDlg, IDC_EDIT_SUN_LIGHT_COLOR_G, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SUN_LIGHT_COLOR_G,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SunLightColorToSliderValue(g_sunLightColor.g)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_sunLightColor.b);
    SetDlgItemText(hDlg, IDC_EDIT_SUN_LIGHT_COLOR_B, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SUN_LIGHT_COLOR_B,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SunLightColorToSliderValue(g_sunLightColor.b)));
}

void RefreshAmbientLightControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_ambientLightIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_AMBIENT_LIGHT_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_AMBIENT_LIGHT_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(AmbientLightIntensityToSliderValue(g_ambientLightIntensity)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ambientLightColor.r);
    SetDlgItemText(hDlg, IDC_EDIT_AMBIENT_LIGHT_COLOR_R, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_AMBIENT_LIGHT_COLOR_R,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SunLightColorToSliderValue(g_ambientLightColor.r)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ambientLightColor.g);
    SetDlgItemText(hDlg, IDC_EDIT_AMBIENT_LIGHT_COLOR_G, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_AMBIENT_LIGHT_COLOR_G,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SunLightColorToSliderValue(g_ambientLightColor.g)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ambientLightColor.b);
    SetDlgItemText(hDlg, IDC_EDIT_AMBIENT_LIGHT_COLOR_B, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_AMBIENT_LIGHT_COLOR_B,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SunLightColorToSliderValue(g_ambientLightColor.b)));
}

void RefreshShadowSaturationBoostControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_shadowSaturationBoost);
    SetDlgItemText(hDlg, IDC_EDIT_SHADOW_SATURATION_BOOST, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SHADOW_SATURATION_BOOST,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(ShadowSaturationBoostToSliderValue(g_shadowSaturationBoost)));
}

void RefreshHalfLambertShadowSaturationControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_halfLambertShadowSaturation);
    SetDlgItemText(hDlg, IDC_EDIT_HALF_LAMBERT_SHADOW_SATURATION, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_HALF_LAMBERT_SHADOW_SATURATION,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(HalfLambertShadowSaturationToSliderValue(g_halfLambertShadowSaturation)));
}

void RefreshShadowDarknessControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_shadowDarkness);
    SetDlgItemText(hDlg, IDC_EDIT_SHADOW_DARKNESS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SHADOW_DARKNESS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(ShadowDarknessToSliderValue(g_shadowDarkness)));
}

void RefreshSpecularIntensityControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_specularIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_SPECULAR_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SPECULAR_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SpecularIntensityToSliderValue(g_specularIntensity)));

    CheckDlgButton(hDlg,
                   IDC_CHECK_SPECULAR_INTENSITY_OVERRIDE,
                   g_bUseSpecularIntensityOverride ? BST_CHECKED : BST_UNCHECKED);

    const BOOL enabled = g_bUseSpecularIntensityOverride ? TRUE : FALSE;
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SPECULAR_INTENSITY), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SPECULAR_INTENSITY), enabled);
}

void RefreshSpecularEdgeControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_specularEdge);
    SetDlgItemText(hDlg, IDC_EDIT_SPECULAR_EDGE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SPECULAR_EDGE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SpecularEdgeToSliderValue(g_specularEdge)));

    CheckDlgButton(hDlg,
                   IDC_CHECK_SPECULAR_EDGE_OVERRIDE,
                   g_bUseSpecularEdgeOverride ? BST_CHECKED : BST_UNCHECKED);

    const BOOL enabled = g_bUseSpecularEdgeOverride ? TRUE : FALSE;
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SPECULAR_EDGE), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SPECULAR_EDGE), enabled);
}

void RefreshSSSControls(HWND hDlg)
{
    wchar_t buffer[32];
    CheckDlgButton(hDlg, IDC_CHECK_SSS, g_bSSS ? BST_CHECKED : BST_UNCHECKED);

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_sssIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_SSS_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSS_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSSIntensityToSliderValue(g_sssIntensity)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_sssColor.r);
    SetDlgItemText(hDlg, IDC_EDIT_SSS_COLOR_R, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSS_COLOR_R,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSSColorToSliderValue(g_sssColor.r)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_sssColor.g);
    SetDlgItemText(hDlg, IDC_EDIT_SSS_COLOR_G, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSS_COLOR_G,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSSColorToSliderValue(g_sssColor.g)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_sssColor.b);
    SetDlgItemText(hDlg, IDC_EDIT_SSS_COLOR_B, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSS_COLOR_B,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSSColorToSliderValue(g_sssColor.b)));

    const BOOL enabled = g_bSSS ? TRUE : FALSE;
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_INTENSITY), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_INTENSITY), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_COLOR_R), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_R), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_COLOR_G), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_G), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_COLOR_B), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_B), enabled);
}

void RefreshSSAOBrightnessControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssaoBrightness);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO_BRIGHTNESS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO_BRIGHTNESS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAOBrightnessToSliderValue(g_ssaoBrightness)));
}

void RefreshSSAOSampleRadiusControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssaoSampleRadius);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO_SAMPLE_RADIUS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO_SAMPLE_RADIUS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAOSampleRadiusToSliderValue(g_ssaoSampleRadius)));
}

void RefreshSSAOSaturationBoostControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssaoSaturationBoost);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO_SATURATION_BOOST, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO_SATURATION_BOOST,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAOSaturationBoostToSliderValue(g_ssaoSaturationBoost)));
}

void RefreshBloomThresholdControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_bloomThreshold);
    SetDlgItemText(hDlg, IDC_EDIT_BLOOM_THRESHOLD, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_BLOOM_THRESHOLD,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(BloomThresholdToSliderValue(g_bloomThreshold)));
}

void RefreshStarBurstThresholdControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_starBurstThreshold);
    SetDlgItemText(hDlg, IDC_EDIT_STARBURST_THRESHOLD, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_STARBURST_THRESHOLD,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(StarBurstThresholdToSliderValue(g_starBurstThreshold)));
}

void RefreshModelLoadScaleControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_modelLoadScale);
    SetDlgItemText(hDlg, IDC_EDIT_MODEL_LOAD_SCALE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_MODEL_LOAD_SCALE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(ModelLoadScaleToSliderValue(g_modelLoadScale)));
}

void RefreshAnimateLight(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_ANIMATE_LIGHT, g_bAnimateLight ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshRemoteDesktop(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_REMOTE_DESKTOP, g_bRemoteDesktop ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshDepthBufferShadow(HWND hDlg)
{
    CheckDlgButton(hDlg,
                   IDC_CHECK_DEPTH_BUFFER_SHADOW,
                   g_bDepthBufferShadow ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshSSAO(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_SSAO, g_bSSAO ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshSSAOModeControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SSAO_MODE);
    if (combo != NULL)
    {
        SendMessage(combo, CB_SETCURSEL, g_ssaoMode == SampleSSAOMode::SSAO2 ? 1 : 0, 0);
    }
}

void RefreshBloom(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_BLOOM, g_bBloom ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshDepthOfField(HWND hDlg)
{
    CheckDlgButton(hDlg,
                   IDC_RADIO_DEPTH_OF_FIELD_OFF,
                   g_depthOfFieldMode == NSRender::DepthOfFieldMode::Disabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg,
                   IDC_RADIO_DEPTH_OF_FIELD_ON,
                   g_depthOfFieldMode == NSRender::DepthOfFieldMode::Enabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg,
                   IDC_RADIO_DEPTH_OF_FIELD_AUTO,
                   g_depthOfFieldMode == NSRender::DepthOfFieldMode::AutoNear ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshDepthOfFieldControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_dofFocalDistance);
    SetDlgItemText(hDlg, IDC_EDIT_DOF_FOCAL_DISTANCE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_DOF_FOCAL_DISTANCE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(DepthOfFieldFocalDistanceToSliderValue(g_dofFocalDistance)));
}

void RefreshDepthOfFieldMaxBlurControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_dofMaxBlurDistance);
    SetDlgItemText(hDlg, IDC_EDIT_DOF_MAX_BLUR_DISTANCE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_DOF_MAX_BLUR_DISTANCE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(DepthOfFieldMaxBlurDistanceToSliderValue(g_dofMaxBlurDistance)));
}

void RefreshDepthOfFieldAutoActivationControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_dofAutoActivationDistance);
    SetDlgItemText(hDlg, IDC_EDIT_DOF_AUTO_ACTIVATION_DISTANCE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_DOF_AUTO_ACTIVATION_DISTANCE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(DepthOfFieldAutoActivationDistanceToSliderValue(g_dofAutoActivationDistance)));
}

void RefreshStarBurst(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_STARBURST, g_bStarBurst ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshGaussianControls(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_GAUSSIAN_FILTER, g_bGaussianFilter ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_MASKED_GAUSSIAN_FILTER, g_bMaskedGaussianFilter ? BST_CHECKED : BST_UNCHECKED);

    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_gaussianSampleSize);
    SetDlgItemText(hDlg, IDC_EDIT_GAUSSIAN_SAMPLE_SIZE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(GaussianSampleSizeToSliderValue(g_gaussianSampleSize)));
}

void RefreshFXAAControls(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_FXAA, g_bFXAA ? BST_CHECKED : BST_UNCHECKED);

    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_fxaaQuality);
    SetDlgItemText(hDlg, IDC_EDIT_FXAA_QUALITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FXAA_QUALITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FXAAQualityToSliderValue(g_fxaaQuality)));
}

void RefreshMotionBlurCameraControls(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_MOTION_BLUR_CAMERA, g_bMotionBlurCamera ? BST_CHECKED : BST_UNCHECKED);

    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.0f", g_motionBlurCameraMaxBlurPixels);
    SetDlgItemText(hDlg, IDC_EDIT_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(MotionBlurCameraMaxBlurPixelsToSliderValue(g_motionBlurCameraMaxBlurPixels)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_motionBlurCameraSampleCount);
    SetDlgItemText(hDlg, IDC_EDIT_MOTION_BLUR_CAMERA_SAMPLE_COUNT, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(MotionBlurCameraSampleCountToSliderValue(g_motionBlurCameraSampleCount)));

    const BOOL enabled = g_bMotionBlurCamera ? TRUE : FALSE;
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MOTION_BLUR_CAMERA_SAMPLE_COUNT), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT), enabled);
}

void RefreshShadowPcfTapControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_shadowPcfTapCount);
    SetDlgItemText(hDlg, IDC_EDIT_SHADOW_PCF_TAPS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SHADOW_PCF_TAPS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(ShadowTapCountToSliderValue(g_shadowPcfTapCount)));
}

void RefreshShadowCompositeTapControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_shadowCompositeTapCount);
    SetDlgItemText(hDlg, IDC_EDIT_SHADOW_COMPOSITE_TAPS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SHADOW_COMPOSITE_TAPS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(ShadowTapCountToSliderValue(g_shadowCompositeTapCount)));
}

void RefreshGodRayControls(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_GODRAY, g_bGodRay ? BST_CHECKED : BST_UNCHECKED);

    wchar_t buffer[32];

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_godRayLightColor.x);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_COLOR_R, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_R, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayLightColorToSliderValue(g_godRayLightColor.x)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_godRayLightColor.y);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_COLOR_G, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_G, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayLightColorToSliderValue(g_godRayLightColor.y)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_godRayLightColor.z);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_COLOR_B, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_B, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayLightColorToSliderValue(g_godRayLightColor.z)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_godRayIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_INTENSITY, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_INTENSITY, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayIntensityToSliderValue(g_godRayIntensity)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_godRayVirtualProximityStrength);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_VIRTUAL_PROXIMITY, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_VIRTUAL_PROXIMITY, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayVirtualProximityStrengthToSliderValue(g_godRayVirtualProximityStrength)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_godRayLightPos.x);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_POS_X, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_X, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayLightPosToSliderValue(g_godRayLightPos.x)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_godRayLightPos.y);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_POS_Y, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Y, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayLightPosToSliderValue(g_godRayLightPos.y)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_godRayLightPos.z);
    SetDlgItemText(hDlg, IDC_EDIT_GODRAY_POS_Z, buffer);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Z, TBM_SETPOS, TRUE,
                       static_cast<LPARAM>(GodRayLightPosToSliderValue(g_godRayLightPos.z)));
}

void RefreshAllControls(HWND hDlg)
{
    RefreshSaturateControls(hDlg);
    RefreshSelectedMeshPaths(hDlg);
    RefreshMixMeshShaderMode(hDlg);
    RefreshResolutionControls(hDlg);
    RefreshLoadedModelListView(hDlg);
    RefreshPointLightListView(hDlg);
    RefreshPointLightControls(hDlg);
    RefreshAnimateLight(hDlg);
    RefreshRemoteDesktop(hDlg);
    RefreshDepthBufferShadow(hDlg);
    RefreshSSAO(hDlg);
    RefreshSSAOModeControls(hDlg);
    RefreshBloom(hDlg);
    RefreshDepthOfField(hDlg);
    RefreshStarBurst(hDlg);
    RefreshFogControls(hDlg);
    RefreshHeightFogControls(hDlg);
    RefreshSunLightIntensityControls(hDlg);
    RefreshSunLightColorControls(hDlg);
    RefreshAmbientLightControls(hDlg);
    RefreshShadowControls(hDlg);
    RefreshShadowCoverageControls(hDlg);
    RefreshShadowSaturationBoostControls(hDlg);
    RefreshHalfLambertShadowSaturationControls(hDlg);
    RefreshShadowDarknessControls(hDlg);
    RefreshSpecularIntensityControls(hDlg);
    RefreshSpecularEdgeControls(hDlg);
    RefreshSSSControls(hDlg);
    RefreshSSAOBrightnessControls(hDlg);
    RefreshSSAOSampleRadiusControls(hDlg);
    RefreshSSAOSaturationBoostControls(hDlg);
    RefreshBloomThresholdControls(hDlg);
    RefreshDepthOfFieldControls(hDlg);
    RefreshDepthOfFieldMaxBlurControls(hDlg);
    RefreshDepthOfFieldAutoActivationControls(hDlg);
    RefreshStarBurstThresholdControls(hDlg);
    RefreshModelLoadScaleControls(hDlg);
    RefreshGaussianControls(hDlg);
    RefreshFXAAControls(hDlg);
    RefreshMotionBlurCameraControls(hDlg);
    RefreshShadowPcfTapControls(hDlg);
    RefreshShadowCompositeTapControls(hDlg);
    RefreshGodRayControls(hDlg);
}

void InitializeTrackbars(HWND hDlg)
{
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETRANGEMIN, FALSE, SATURATE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETRANGEMAX, FALSE, SATURATE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_FOG_INTENSITY, TBM_SETRANGEMIN, FALSE, FOG_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_FOG_INTENSITY, TBM_SETRANGEMAX, FALSE, FOG_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_FOG_INTENSITY, TBM_SETTICFREQ, 10, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_FOG_INTENSITY, TBM_SETPAGESIZE, 0, 10);

    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_INTENSITY, TBM_SETRANGEMIN, FALSE, HEIGHT_FOG_INTENSITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_INTENSITY, TBM_SETRANGEMAX, FALSE, HEIGHT_FOG_INTENSITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_INTENSITY, TBM_SETTICFREQ, 10, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_INTENSITY, TBM_SETPAGESIZE, 0, 10);

    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_START, TBM_SETRANGEMIN, FALSE, HEIGHT_FOG_HEIGHT_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_START, TBM_SETRANGEMAX, FALSE, HEIGHT_FOG_HEIGHT_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_START, TBM_SETTICFREQ, 20, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_START, TBM_SETPAGESIZE, 0, 10);

    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_MAX, TBM_SETRANGEMIN, FALSE, HEIGHT_FOG_HEIGHT_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_MAX, TBM_SETRANGEMAX, FALSE, HEIGHT_FOG_HEIGHT_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_MAX, TBM_SETTICFREQ, 20, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_MAX, TBM_SETPAGESIZE, 0, 10);

    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_START, TBM_SETRANGEMIN, FALSE, HEIGHT_FOG_DISTANCE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_START, TBM_SETRANGEMAX, FALSE, HEIGHT_FOG_DISTANCE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_START, TBM_SETTICFREQ, 20, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_START, TBM_SETPAGESIZE, 0, 10);

    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_MAX, TBM_SETRANGEMIN, FALSE, HEIGHT_FOG_DISTANCE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_MAX, TBM_SETRANGEMAX, FALSE, HEIGHT_FOG_DISTANCE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_MAX, TBM_SETTICFREQ, 20, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_MAX, TBM_SETPAGESIZE, 0, 10);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETRANGEMIN, FALSE, SUN_LIGHT_INTENSITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETRANGEMAX, FALSE, SUN_LIGHT_INTENSITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_R, TBM_SETRANGEMIN, FALSE, SUN_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_R, TBM_SETRANGEMAX, FALSE, SUN_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_R, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_R, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_G, TBM_SETRANGEMIN, FALSE, SUN_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_G, TBM_SETRANGEMAX, FALSE, SUN_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_G, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_G, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_B, TBM_SETRANGEMIN, FALSE, SUN_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_B, TBM_SETRANGEMAX, FALSE, SUN_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_B, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_B, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_INTENSITY, TBM_SETRANGEMIN, FALSE, AMBIENT_LIGHT_INTENSITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_INTENSITY, TBM_SETRANGEMAX, FALSE, AMBIENT_LIGHT_INTENSITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_INTENSITY, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_INTENSITY, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_R, TBM_SETRANGEMIN, FALSE, AMBIENT_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_R, TBM_SETRANGEMAX, FALSE, AMBIENT_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_R, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_R, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_G, TBM_SETRANGEMIN, FALSE, AMBIENT_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_G, TBM_SETRANGEMAX, FALSE, AMBIENT_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_G, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_G, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_B, TBM_SETRANGEMIN, FALSE, AMBIENT_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_B, TBM_SETRANGEMAX, FALSE, AMBIENT_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_B, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_B, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETRANGEMIN, FALSE, SHADOW_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETRANGEMAX, FALSE, SHADOW_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COVERAGE, TBM_SETRANGEMIN, FALSE, SHADOW_COVERAGE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COVERAGE, TBM_SETRANGEMAX, FALSE, SHADOW_COVERAGE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COVERAGE, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COVERAGE, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_SATURATION_BOOST, TBM_SETRANGEMIN, FALSE, SHADOW_SATURATION_BOOST_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_SATURATION_BOOST, TBM_SETRANGEMAX, FALSE, SHADOW_SATURATION_BOOST_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_SATURATION_BOOST, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_SATURATION_BOOST, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_HALF_LAMBERT_SHADOW_SATURATION, TBM_SETRANGEMIN, FALSE, HALF_LAMBERT_SHADOW_SATURATION_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HALF_LAMBERT_SHADOW_SATURATION, TBM_SETRANGEMAX, FALSE, HALF_LAMBERT_SHADOW_SATURATION_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HALF_LAMBERT_SHADOW_SATURATION, TBM_SETTICFREQ, 4, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_HALF_LAMBERT_SHADOW_SATURATION, TBM_SETPAGESIZE, 0, 4);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_DARKNESS, TBM_SETRANGEMIN, FALSE, SHADOW_DARKNESS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_DARKNESS, TBM_SETRANGEMAX, FALSE, SHADOW_DARKNESS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_DARKNESS, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_DARKNESS, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_INTENSITY, TBM_SETRANGEMIN, FALSE, SPECULAR_INTENSITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_INTENSITY, TBM_SETRANGEMAX, FALSE, SPECULAR_INTENSITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_INTENSITY, TBM_SETTICFREQ, 4, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_INTENSITY, TBM_SETPAGESIZE, 0, 4);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_EDGE, TBM_SETRANGEMIN, FALSE, SPECULAR_EDGE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_EDGE, TBM_SETRANGEMAX, FALSE, SPECULAR_EDGE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_EDGE, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SPECULAR_EDGE, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_INTENSITY, TBM_SETRANGEMIN, FALSE, SSS_INTENSITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_INTENSITY, TBM_SETRANGEMAX, FALSE, SSS_INTENSITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_INTENSITY, TBM_SETTICFREQ, 4, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_INTENSITY, TBM_SETPAGESIZE, 0, 4);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_R, TBM_SETRANGEMIN, FALSE, SSS_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_R, TBM_SETRANGEMAX, FALSE, SSS_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_R, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_R, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_G, TBM_SETRANGEMIN, FALSE, SSS_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_G, TBM_SETRANGEMAX, FALSE, SSS_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_G, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_G, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_B, TBM_SETRANGEMIN, FALSE, SSS_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_B, TBM_SETRANGEMAX, FALSE, SSS_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_B, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSS_COLOR_B, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETRANGEMIN, FALSE, SSAO_BRIGHTNESS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETRANGEMAX, FALSE, SSAO_BRIGHTNESS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SAMPLE_RADIUS, TBM_SETRANGEMIN, FALSE, SSAO_SAMPLE_RADIUS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SAMPLE_RADIUS, TBM_SETRANGEMAX, FALSE, SSAO_SAMPLE_RADIUS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SAMPLE_RADIUS, TBM_SETTICFREQ, 10, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SAMPLE_RADIUS, TBM_SETPAGESIZE, 0, 10);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SATURATION_BOOST, TBM_SETRANGEMIN, FALSE, SSAO_SATURATION_BOOST_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SATURATION_BOOST, TBM_SETRANGEMAX, FALSE, SSAO_SATURATION_BOOST_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SATURATION_BOOST, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_SATURATION_BOOST, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETRANGEMIN, FALSE, BLOOM_THRESHOLD_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETRANGEMAX, FALSE, BLOOM_THRESHOLD_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_FOCAL_DISTANCE, TBM_SETRANGEMIN, FALSE, DOF_FOCAL_DISTANCE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_FOCAL_DISTANCE, TBM_SETRANGEMAX, FALSE, DOF_FOCAL_DISTANCE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_FOCAL_DISTANCE, TBM_SETTICFREQ, 20, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_FOCAL_DISTANCE, TBM_SETPAGESIZE, 0, 20);

    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_MAX_BLUR_DISTANCE, TBM_SETRANGEMIN, FALSE, DOF_MAX_BLUR_DISTANCE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_MAX_BLUR_DISTANCE, TBM_SETRANGEMAX, FALSE, DOF_MAX_BLUR_DISTANCE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_MAX_BLUR_DISTANCE, TBM_SETTICFREQ, 20, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_MAX_BLUR_DISTANCE, TBM_SETPAGESIZE, 0, 20);

    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_AUTO_ACTIVATION_DISTANCE, TBM_SETRANGEMIN, FALSE, DOF_AUTO_ACTIVATION_DISTANCE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_AUTO_ACTIVATION_DISTANCE, TBM_SETRANGEMAX, FALSE, DOF_AUTO_ACTIVATION_DISTANCE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_AUTO_ACTIVATION_DISTANCE, TBM_SETTICFREQ, 20, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_DOF_AUTO_ACTIVATION_DISTANCE, TBM_SETPAGESIZE, 0, 20);

    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETRANGEMIN, FALSE, STARBURST_THRESHOLD_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETRANGEMAX, FALSE, STARBURST_THRESHOLD_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETRANGEMIN, FALSE, MODEL_LOAD_SCALE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETRANGEMAX, FALSE, MODEL_LOAD_SCALE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETTICFREQ, 50, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETPAGESIZE, 0, 50);

    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_R, TBM_SETRANGEMIN, FALSE, POINT_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_R, TBM_SETRANGEMAX, FALSE, POINT_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_R, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_R, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_G, TBM_SETRANGEMIN, FALSE, POINT_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_G, TBM_SETRANGEMAX, FALSE, POINT_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_G, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_G, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_B, TBM_SETRANGEMIN, FALSE, POINT_LIGHT_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_B, TBM_SETRANGEMAX, FALSE, POINT_LIGHT_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_B, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_B, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_BRIGHTNESS, TBM_SETRANGEMIN, FALSE, POINT_LIGHT_BRIGHTNESS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_BRIGHTNESS, TBM_SETRANGEMAX, FALSE, POINT_LIGHT_BRIGHTNESS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_BRIGHTNESS, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_POINT_LIGHT_BRIGHTNESS, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE, TBM_SETRANGEMIN, FALSE, GAUSSIAN_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE, TBM_SETRANGEMAX, FALSE, GAUSSIAN_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_FXAA_QUALITY, TBM_SETRANGEMIN, FALSE, FXAA_QUALITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_FXAA_QUALITY, TBM_SETRANGEMAX, FALSE, FXAA_QUALITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_FXAA_QUALITY, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_FXAA_QUALITY, TBM_SETPAGESIZE, 0, 1);

    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS, TBM_SETRANGEMIN, FALSE, MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS, TBM_SETRANGEMAX, FALSE, MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS, TBM_SETTICFREQ, 4, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS, TBM_SETPAGESIZE, 0, 4);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT, TBM_SETRANGEMIN, FALSE, MOTION_BLUR_CAMERA_SAMPLE_COUNT_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT, TBM_SETRANGEMAX, FALSE, MOTION_BLUR_CAMERA_SAMPLE_COUNT_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT, TBM_SETPAGESIZE, 0, 1);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_PCF_TAPS, TBM_SETRANGEMIN, FALSE, SHADOW_BLUR_TAP_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_PCF_TAPS, TBM_SETRANGEMAX, FALSE, SHADOW_BLUR_TAP_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_PCF_TAPS, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_PCF_TAPS, TBM_SETPAGESIZE, 0, 1);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COMPOSITE_TAPS, TBM_SETRANGEMIN, FALSE, SHADOW_BLUR_TAP_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COMPOSITE_TAPS, TBM_SETRANGEMAX, FALSE, SHADOW_BLUR_TAP_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COMPOSITE_TAPS, TBM_SETTICFREQ, 1, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_COMPOSITE_TAPS, TBM_SETPAGESIZE, 0, 1);

    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_R, TBM_SETRANGEMIN, FALSE, GODRAY_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_R, TBM_SETRANGEMAX, FALSE, GODRAY_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_R, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_R, TBM_SETPAGESIZE, 0, 1);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_G, TBM_SETRANGEMIN, FALSE, GODRAY_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_G, TBM_SETRANGEMAX, FALSE, GODRAY_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_G, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_G, TBM_SETPAGESIZE, 0, 1);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_B, TBM_SETRANGEMIN, FALSE, GODRAY_COLOR_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_B, TBM_SETRANGEMAX, FALSE, GODRAY_COLOR_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_B, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_COLOR_B, TBM_SETPAGESIZE, 0, 1);

    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_INTENSITY, TBM_SETRANGEMIN, FALSE, GODRAY_INTENSITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_INTENSITY, TBM_SETRANGEMAX, FALSE, GODRAY_INTENSITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_INTENSITY, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_INTENSITY, TBM_SETPAGESIZE, 0, 1);

    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_VIRTUAL_PROXIMITY, TBM_SETRANGEMIN, FALSE, GODRAY_VIRTUAL_PROXIMITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_VIRTUAL_PROXIMITY, TBM_SETRANGEMAX, FALSE, GODRAY_VIRTUAL_PROXIMITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_VIRTUAL_PROXIMITY, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_VIRTUAL_PROXIMITY, TBM_SETPAGESIZE, 0, 1);

    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_X, TBM_SETRANGEMIN, FALSE, GODRAY_POS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_X, TBM_SETRANGEMAX, FALSE, GODRAY_POS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_X, TBM_SETTICFREQ, 10, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_X, TBM_SETPAGESIZE, 0, 1);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Y, TBM_SETRANGEMIN, FALSE, GODRAY_POS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Y, TBM_SETRANGEMAX, FALSE, GODRAY_POS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Y, TBM_SETTICFREQ, 10, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Y, TBM_SETPAGESIZE, 0, 1);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Z, TBM_SETRANGEMIN, FALSE, GODRAY_POS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Z, TBM_SETRANGEMAX, FALSE, GODRAY_POS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Z, TBM_SETTICFREQ, 10, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_GODRAY_POS_Z, TBM_SETPAGESIZE, 0, 1);
}

bool HandleOpenMeshCommand(HWND hDlg, const WORD commandId)
{
    if (commandId == IDC_BUTTON_OPEN_MIX_MESH)
    {
        if (ShowOpenFileDialog(hDlg,
                               L"Mix Mesh Files (*.x;*.blend.x)\0*.x;*.blend.x\0All Files (*.*)\0*.*\0",
                               g_selectedMixMeshPath))
        {
            SpawnMeshMixAtLookAt(g_selectedMixMeshPath);
            RefreshSelectedMeshPaths(hDlg);
        }
        return true;
    }

    if (commandId == IDC_BUTTON_OPEN_MESH)
    {
        if (ShowOpenFileDialog(hDlg,
                               L"Mesh Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                               g_selectedMeshPath))
        {
            SpawnMeshAtLookAt(g_selectedMeshPath);
            RefreshSelectedMeshPaths(hDlg);
        }
        return true;
    }

    if (commandId == IDC_BUTTON_OPEN_ANIM_MESH)
    {
        if (ShowOpenFileDialog(hDlg,
                               L"Anim Mesh Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                               g_selectedAnimMeshPath))
        {
            SpawnAnimMeshAtLookAt(g_selectedAnimMeshPath);
            RefreshSelectedMeshPaths(hDlg);
        }
        return true;
    }

    if (commandId == IDC_BUTTON_OPEN_SKIN_ANIM_MESH)
    {
        if (ShowOpenFileDialog(hDlg,
                               L"Skin Anim Mesh Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                               g_selectedSkinAnimMeshPath))
        {
            SpawnSkinAnimMeshAtLookAt(g_selectedSkinAnimMeshPath);
            RefreshSelectedMeshPaths(hDlg);
        }
        return true;
    }

    if (commandId == IDC_BUTTON_OPEN_MIX_SKIN_ANIM_MESH)
    {
        if (ShowOpenFileDialog(hDlg,
                               L"MeshMix Skin Anim Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                               g_selectedMixSkinAnimMeshPath))
        {
            SpawnMeshMixSkinAnimAtLookAt(g_selectedMixSkinAnimMeshPath);
            RefreshSelectedMeshPaths(hDlg);
        }
        return true;
    }

    if (commandId == IDC_BUTTON_OPEN_MASKED_GAUSSIAN_MASK)
    {
        if (ShowOpenFileDialog(hDlg,
                               L"Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.dds;*.tga)\0*.png;*.jpg;*.jpeg;*.bmp;*.dds;*.tga\0All Files (*.*)\0*.*\0",
                               g_selectedMaskedGaussianMaskPath,
                               L"png"))
        {
            ApplyMaskedGaussianMaskPath();
            RefreshSelectedMeshPaths(hDlg);
        }
        return true;
    }

    return false;
}
}

void ShowSettingsDialog(HWND hWnd, const bool activateDialog)
{
    if (activateDialog)
    {
        DisableMouseLook();
    }

    if (g_hSettingsDialog != NULL)
    {
        ShowWindow(g_hSettingsDialog, SW_SHOWNORMAL);
        if (activateDialog)
        {
            SetForegroundWindow(g_hSettingsDialog);
        }
        else
        {
            SetForegroundWindow(hWnd);
        }
        return;
    }

    g_hSettingsDialog = CreateDialog(GetModuleHandle(NULL),
                                     MAKEINTRESOURCE(IDD_SETTINGS_DIALOG),
                                     hWnd,
                                     SettingsDialogProc);
    assert(g_hSettingsDialog != NULL);

    ShowWindow(g_hSettingsDialog, SW_SHOWNORMAL);

    if (activateDialog)
    {
        SetForegroundWindow(g_hSettingsDialog);
    }
    else
    {
        SetWindowPos(g_hSettingsDialog,
                     HWND_TOP,
                     0,
                     0,
                     0,
                     0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
    }
}

void ToggleSettingsDialog(HWND hWnd)
{
    if (g_hSettingsDialog != NULL && IsWindowVisible(g_hSettingsDialog))
    {
        ShowWindow(g_hSettingsDialog, SW_HIDE);
        SetForegroundWindow(hWnd);
        SetFocus(hWnd);
        return;
    }

    ShowSettingsDialog(hWnd, true);
}

void RefreshSettingsDialogState()
{
    if (g_hSettingsDialog != NULL)
    {
        RefreshAllControls(g_hSettingsDialog);
    }
}

INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        MoveDialogToRightOfParent(hDlg);
        InitializeEditableNumericFields(hDlg);
        InitializeTrackbars(hDlg);
        PopulateResolutionCombo(hDlg);
        PopulatePointLightTypeCombo(hDlg);
        PopulateSSAOModeCombo(hDlg);
        InitializeLoadedModelListView(hDlg);
        InitializePointLightListView(hDlg);
        RefreshAllControls(hDlg);
        g_settingsDialogScrollPos = 0;
        UpdateSettingsDialogScrollBar(hDlg);
        return TRUE;
    }
    case WM_MOUSEWHEEL:
    {
        const short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        ScrollSettingsDialogTo(hDlg,
                               g_settingsDialogScrollPos - ((wheelDelta / WHEEL_DELTA) * SETTINGS_DIALOG_WHEEL_STEP_PX));
        return TRUE;
    }
    case WM_VSCROLL:
    {
        SCROLLINFO scrollInfo { };
        scrollInfo.cbSize = sizeof(scrollInfo);
        scrollInfo.fMask = SIF_ALL;
        GetScrollInfo(hDlg, SB_VERT, &scrollInfo);

        int newPos = g_settingsDialogScrollPos;
        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            newPos -= SETTINGS_DIALOG_WHEEL_STEP_PX;
            break;
        case SB_LINEDOWN:
            newPos += SETTINGS_DIALOG_WHEEL_STEP_PX;
            break;
        case SB_PAGEUP:
            newPos -= static_cast<int>(scrollInfo.nPage);
            break;
        case SB_PAGEDOWN:
            newPos += static_cast<int>(scrollInfo.nPage);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newPos = scrollInfo.nTrackPos;
            break;
        case SB_TOP:
            newPos = 0;
            break;
        case SB_BOTTOM:
            newPos = scrollInfo.nMax;
            break;
        default:
            return FALSE;
        }

        ScrollSettingsDialogTo(hDlg, newPos);
        return TRUE;
    }
    case WM_CLOSE:
    {
        DestroyWindow(hDlg);
        return TRUE;
    }
    case WM_DESTROY:
    {
        if (g_hSettingsDialog == hDlg)
        {
            g_hSettingsDialog = NULL;
        }
        return TRUE;
    }
    case WM_HSCROLL:
    {
        const HWND slider = reinterpret_cast<HWND>(lParam);
        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SATURATE_LEVEL))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_saturateLevel = SliderValueToSaturateLevel(sliderValue);
            ApplySaturateLevel();
            RefreshSaturateControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_gaussianSampleSize = SliderValueToGaussianSampleSize(sliderValue);
            ApplyGaussianSampleSize();
            RefreshGaussianControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_FXAA_QUALITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_fxaaQuality = SliderValueToFXAAQuality(sliderValue);
            ApplyFXAAQuality();
            RefreshFXAAControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_motionBlurCameraMaxBlurPixels = SliderValueToMotionBlurCameraMaxBlurPixels(sliderValue);
            ApplyMotionBlurCameraSettings();
            RefreshMotionBlurCameraControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_motionBlurCameraSampleCount = SliderValueToMotionBlurCameraSampleCount(sliderValue);
            ApplyMotionBlurCameraSettings();
            RefreshMotionBlurCameraControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_PCF_TAPS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowPcfTapCount = SliderValueToShadowTapCount(sliderValue);
            ApplyShadowPcfTapCount();
            RefreshShadowPcfTapControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_COMPOSITE_TAPS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowCompositeTapCount = SliderValueToShadowTapCount(sliderValue);
            ApplyShadowCompositeTapCount();
            RefreshShadowCompositeTapControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_COLOR_R))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayLightColor.x = SliderValueToGodRayLightColor(sliderValue);
            ApplyGodRayLightColor();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_COLOR_G))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayLightColor.y = SliderValueToGodRayLightColor(sliderValue);
            ApplyGodRayLightColor();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_COLOR_B))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayLightColor.z = SliderValueToGodRayLightColor(sliderValue);
            ApplyGodRayLightColor();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayIntensity = SliderValueToGodRayIntensity(sliderValue);
            ApplyGodRayIntensity();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_VIRTUAL_PROXIMITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayVirtualProximityStrength = SliderValueToGodRayVirtualProximityStrength(sliderValue);
            ApplyGodRayVirtualProximityStrength();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_POS_X))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayLightPos.x = SliderValueToGodRayLightPos(sliderValue);
            ApplyGodRayLightPos();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_POS_Y))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayLightPos.y = SliderValueToGodRayLightPos(sliderValue);
            ApplyGodRayLightPos();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_GODRAY_POS_Z))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_godRayLightPos.z = SliderValueToGodRayLightPos(sliderValue);
            ApplyGodRayLightPos();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_FOG_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_fogIntensity = SliderValueToFogIntensity(sliderValue);
            ApplyFogIntensity();
            RefreshFogControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_heightFogIntensity = SliderValueToHeightFogIntensity(sliderValue);
            ApplyHeightFogIntensity();
            RefreshHeightFogIntensityControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_START))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_heightFogStart = SliderValueToHeightFogHeight(sliderValue);
            ApplyHeightFogStart();
            RefreshHeightFogStartControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_MAX))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_heightFogMax = SliderValueToHeightFogHeight(sliderValue);
            ApplyHeightFogMax();
            RefreshHeightFogMaxControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_START))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_heightFogDistanceStart = SliderValueToHeightFogDistance(sliderValue);
            ApplyHeightFogDistanceStart();
            RefreshHeightFogDistanceStartControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_MAX))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_heightFogDistanceMax = SliderValueToHeightFogDistance(sliderValue);
            ApplyHeightFogDistanceMax();
            RefreshHeightFogDistanceMaxControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sunLightIntensity = SliderValueToSunLightIntensity(sliderValue);
            ApplySunLightIntensity();
            RefreshSunLightIntensityControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_R))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sunLightColor.r = SliderValueToSunLightColor(sliderValue);
            ApplySunLightColor();
            RefreshSunLightColorControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_G))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sunLightColor.g = SliderValueToSunLightColor(sliderValue);
            ApplySunLightColor();
            RefreshSunLightColorControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SUN_LIGHT_COLOR_B))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sunLightColor.b = SliderValueToSunLightColor(sliderValue);
            ApplySunLightColor();
            RefreshSunLightColorControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_AMBIENT_LIGHT_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ambientLightIntensity = SliderValueToAmbientLightIntensity(sliderValue);
            ApplyAmbientLightIntensity();
            RefreshAmbientLightControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_R))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ambientLightColor.r = SliderValueToSunLightColor(sliderValue);
            ApplyAmbientLightColor();
            RefreshAmbientLightControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_G))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ambientLightColor.g = SliderValueToSunLightColor(sliderValue);
            ApplyAmbientLightColor();
            RefreshAmbientLightControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_AMBIENT_LIGHT_COLOR_B))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ambientLightColor.b = SliderValueToSunLightColor(sliderValue);
            ApplyAmbientLightColor();
            RefreshAmbientLightControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowIntensity = SliderValueToShadowIntensity(sliderValue);
            ApplyShadowIntensity();
            RefreshShadowControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_COVERAGE))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowCoverage = SliderValueToShadowCoverage(sliderValue);
            ApplyShadowCoverage();
            RefreshShadowCoverageControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_SATURATION_BOOST))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowSaturationBoost = SliderValueToShadowSaturationBoost(sliderValue);
            ApplyShadowSaturationBoost();
            RefreshShadowSaturationBoostControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_HALF_LAMBERT_SHADOW_SATURATION))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_halfLambertShadowSaturation = SliderValueToHalfLambertShadowSaturation(sliderValue);
            ApplyHalfLambertShadowSaturation();
            RefreshHalfLambertShadowSaturationControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_DARKNESS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowDarkness = SliderValueToShadowDarkness(sliderValue);
            ApplyShadowDarkness();
            RefreshShadowDarknessControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SPECULAR_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_specularIntensity = SliderValueToSpecularIntensity(sliderValue);
            ApplySpecularIntensity();
            RefreshSpecularIntensityControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SPECULAR_EDGE))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_specularEdge = SliderValueToSpecularEdge(sliderValue);
            ApplySpecularEdge();
            RefreshSpecularEdgeControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSS_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sssIntensity = SliderValueToSSSIntensity(sliderValue);
            ApplySSSIntensity();
            RefreshSSSControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_R))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sssColor.r = SliderValueToSSSColor(sliderValue);
            ApplySSSColor();
            RefreshSSSControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_G))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sssColor.g = SliderValueToSSSColor(sliderValue);
            ApplySSSColor();
            RefreshSSSControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_B))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_sssColor.b = SliderValueToSSSColor(sliderValue);
            ApplySSSColor();
            RefreshSSSControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ssaoBrightness = SliderValueToSSAOBrightness(sliderValue);
            ApplySSAOBrightness();
            RefreshSSAOBrightnessControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSAO_SAMPLE_RADIUS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ssaoSampleRadius = SliderValueToSSAOSampleRadius(sliderValue);
            ApplySSAOSampleRadius();
            RefreshSSAOSampleRadiusControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSAO_SATURATION_BOOST))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ssaoSaturationBoost = SliderValueToSSAOSaturationBoost(sliderValue);
            ApplySSAOSaturationBoost();
            RefreshSSAOSaturationBoostControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_BLOOM_THRESHOLD))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_bloomThreshold = SliderValueToBloomThreshold(sliderValue);
            ApplyBloomThreshold();
            RefreshBloomThresholdControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_DOF_FOCAL_DISTANCE))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_dofFocalDistance = SliderValueToDepthOfFieldFocalDistance(sliderValue);
            ApplyDepthOfFieldFocalDistance();
            RefreshDepthOfFieldControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_DOF_MAX_BLUR_DISTANCE))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_dofMaxBlurDistance = SliderValueToDepthOfFieldMaxBlurDistance(sliderValue);
            ApplyDepthOfFieldMaxBlurDistance();
            RefreshDepthOfFieldMaxBlurControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_DOF_AUTO_ACTIVATION_DISTANCE))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_dofAutoActivationDistance = SliderValueToDepthOfFieldAutoActivationDistance(sliderValue);
            ApplyDepthOfFieldAutoActivationDistance();
            RefreshDepthOfFieldAutoActivationControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_STARBURST_THRESHOLD))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_starBurstThreshold = SliderValueToStarBurstThreshold(sliderValue);
            ApplyStarBurstThreshold();
            RefreshStarBurstThresholdControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_modelLoadScale = SliderValueToModelLoadScale(sliderValue);
            ApplyModelLoadScale();
            RefreshModelLoadScaleControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_R))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_pointLightColor.r = SliderValueToPointLightColor(sliderValue);
            ApplyPointLightColor();
            RefreshPointLightControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_G))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_pointLightColor.g = SliderValueToPointLightColor(sliderValue);
            ApplyPointLightColor();
            RefreshPointLightControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_POINT_LIGHT_COLOR_B))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_pointLightColor.b = SliderValueToPointLightColor(sliderValue);
            ApplyPointLightColor();
            RefreshPointLightControls(hDlg);
            return TRUE;
        }

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_POINT_LIGHT_BRIGHTNESS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_pointLightBrightness = SliderValueToPointLightBrightness(sliderValue);
            ApplyPointLightBrightness();
            RefreshPointLightControls(hDlg);
            return TRUE;
        }

        break;
    }
    case WM_CONTEXTMENU:
    {
        HWND source = reinterpret_cast<HWND>(wParam);
        HWND loadedModelListView = GetDlgItem(hDlg, IDC_LIST_LOADED_MODELS);
        HWND pointLightListView = GetDlgItem(hDlg, IDC_LIST_POINT_LIGHTS);
        if (source == loadedModelListView)
        {
            POINT screenPoint { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (ShowLoadedModelContextMenu(hDlg, loadedModelListView, screenPoint))
            {
                return TRUE;
            }
        }
        else if (source == pointLightListView)
        {
            POINT screenPoint { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (ShowPointLightContextMenu(hDlg, pointLightListView, screenPoint))
            {
                return TRUE;
            }
        }
        break;
    }
    case WM_COMMAND:
    {
        const WORD commandId = LOWORD(wParam);
        const WORD notifyCode = HIWORD(wParam);

        if (notifyCode == EN_KILLFOCUS && HandleNumericEditCommit(hDlg, commandId))
        {
            return TRUE;
        }

        if (commandId == IDC_BUTTON_SATURATE_DOWN)
        {
            g_saturateLevel -= SATURATE_STEP;
            ApplySaturateLevel();
            RefreshSaturateControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_BUTTON_SATURATE_UP)
        {
            g_saturateLevel += SATURATE_STEP;
            ApplySaturateLevel();
            RefreshSaturateControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_BUTTON_SATURATE_RESET)
        {
            g_saturateLevel = 1.0f;
            ApplySaturateLevel();
            RefreshSaturateControls(hDlg);
            return TRUE;
        }

        if (HandleOpenMeshCommand(hDlg, commandId))
        {
            return TRUE;
        }

        if (commandId == IDC_CHECK_ANIMATE_LIGHT)
        {
            g_bAnimateLight = (IsDlgButtonChecked(hDlg, IDC_CHECK_ANIMATE_LIGHT) == BST_CHECKED);
            RefreshAnimateLight(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_REMOTE_DESKTOP)
        {
            g_bRemoteDesktop = (IsDlgButtonChecked(hDlg, IDC_CHECK_REMOTE_DESKTOP) == BST_CHECKED);
            RefreshRemoteDesktop(hDlg);
            return TRUE;
        }

        if (commandId == IDC_BUTTON_ADD_POINT_LIGHT)
        {
            AddPointLightAtLookAt();
            RefreshPointLightControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_COMBO_POINT_LIGHT_TYPE && HIWORD(wParam) == CBN_SELCHANGE)
        {
            HWND combo = reinterpret_cast<HWND>(lParam);
            const int index = static_cast<int>(SendMessage(combo, CB_GETCURSEL, 0, 0));
            g_pointLightShape = ComboIndexToPointLightShape(index);
            ApplyPointLightShape();
            RefreshPointLightControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_COMBO_SSAO_MODE && HIWORD(wParam) == CBN_SELCHANGE)
        {
            HWND combo = reinterpret_cast<HWND>(lParam);
            const int index = static_cast<int>(SendMessage(combo, CB_GETCURSEL, 0, 0));
            g_ssaoMode = (index == 1) ? SampleSSAOMode::SSAO2 : SampleSSAOMode::Legacy;
            ApplySSAOMode();
            RefreshSSAOModeControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_MIX_MESH_NONE)
        {
            g_mixMeshShaderMode = MixMeshShaderMode::None;
            RefreshMixMeshShaderMode(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_MIX_MESH_POM)
        {
            g_mixMeshShaderMode = MixMeshShaderMode::ParallaxOcclusionMapping;
            RefreshMixMeshShaderMode(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_MIX_MESH_NORMAL_MAP)
        {
            g_mixMeshShaderMode = MixMeshShaderMode::NormalMapping;
            RefreshMixMeshShaderMode(hDlg);
            return TRUE;
        }

        if (commandId == IDC_COMBO_RESOLUTION && HIWORD(wParam) == CBN_SELCHANGE)
        {
            HWND combo = reinterpret_cast<HWND>(lParam);
            const int index = static_cast<int>(SendMessage(combo, CB_GETCURSEL, 0, 0));
            if (index != CB_ERR)
            {
                wchar_t buffer[64] { };
                SendMessage(combo, CB_GETLBTEXT, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(buffer));

                int width = 0;
                int height = 0;
                if (swscanf_s(buffer, L"%d x %d", &width, &height) == 2)
                {
                    g_resolutionWidth = width;
                    g_resolutionHeight = height;
                    ApplyResolution();
                    RefreshResolutionControls(hDlg);
                }
            }
            return TRUE;
        }

        if (commandId == IDC_RADIO_WINDOW_MODE_WINDOW)
        {
            g_windowMode = NSRender::eWindowMode::WINDOW;
            ApplyWindowMode();
            RefreshResolutionControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_WINDOW_MODE_BORDERLESS)
        {
            g_windowMode = NSRender::eWindowMode::BORDERLESS;
            ApplyWindowMode();
            RefreshResolutionControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_WINDOW_MODE_FULLSCREEN)
        {
            g_windowMode = NSRender::eWindowMode::FULLSCREEN;
            ApplyWindowMode();
            RefreshResolutionControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_DEPTH_BUFFER_SHADOW)
        {
            g_bDepthBufferShadow = (IsDlgButtonChecked(hDlg, IDC_CHECK_DEPTH_BUFFER_SHADOW) == BST_CHECKED);
            g_Render.SetPostEffectDepthBufferShadow(g_bDepthBufferShadow);
            RefreshDepthBufferShadow(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_SSAO)
        {
            g_bSSAO = (IsDlgButtonChecked(hDlg, IDC_CHECK_SSAO) == BST_CHECKED);
            g_Render.SetPostEffectSSAO(g_bSSAO);
            RefreshSSAO(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_GAUSSIAN_FILTER)
        {
            g_bGaussianFilter = (IsDlgButtonChecked(hDlg, IDC_CHECK_GAUSSIAN_FILTER) == BST_CHECKED);
            g_Render.SetPostEffectGaussianFilter(g_bGaussianFilter);
            RefreshGaussianControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_HEIGHT_FOG)
        {
            g_bHeightFog = (IsDlgButtonChecked(hDlg, IDC_CHECK_HEIGHT_FOG) == BST_CHECKED);
            g_Render.SetPostEffectHeightFog(g_bHeightFog);
            RefreshHeightFogControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_MASKED_GAUSSIAN_FILTER)
        {
            g_bMaskedGaussianFilter = (IsDlgButtonChecked(hDlg, IDC_CHECK_MASKED_GAUSSIAN_FILTER) == BST_CHECKED);
            g_Render.SetPostEffectMaskedGaussianFilter(g_bMaskedGaussianFilter);
            RefreshGaussianControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_FXAA)
        {
            g_bFXAA = (IsDlgButtonChecked(hDlg, IDC_CHECK_FXAA) == BST_CHECKED);
            g_Render.SetPostEffectFXAA(g_bFXAA);
            RefreshFXAAControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_MOTION_BLUR_CAMERA)
        {
            g_bMotionBlurCamera = (IsDlgButtonChecked(hDlg, IDC_CHECK_MOTION_BLUR_CAMERA) == BST_CHECKED);
            g_Render.SetPostEffectMotionBlurCamera(g_bMotionBlurCamera);
            RefreshMotionBlurCameraControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_SSS)
        {
            g_bSSS = (IsDlgButtonChecked(hDlg, IDC_CHECK_SSS) == BST_CHECKED);
            ApplySSS();
            RefreshSSSControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_BLOOM)
        {
            g_bBloom = (IsDlgButtonChecked(hDlg, IDC_CHECK_BLOOM) == BST_CHECKED);
            g_Render.SetPostEffectBloom(g_bBloom);
            RefreshBloom(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_DEPTH_OF_FIELD_OFF)
        {
            g_depthOfFieldMode = NSRender::DepthOfFieldMode::Disabled;
            ApplyDepthOfFieldMode();
            RefreshDepthOfField(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_DEPTH_OF_FIELD_ON)
        {
            g_depthOfFieldMode = NSRender::DepthOfFieldMode::Enabled;
            ApplyDepthOfFieldMode();
            RefreshDepthOfField(hDlg);
            return TRUE;
        }

        if (commandId == IDC_RADIO_DEPTH_OF_FIELD_AUTO)
        {
            g_depthOfFieldMode = NSRender::DepthOfFieldMode::AutoNear;
            ApplyDepthOfFieldMode();
            RefreshDepthOfField(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_STARBURST)
        {
            g_bStarBurst = (IsDlgButtonChecked(hDlg, IDC_CHECK_STARBURST) == BST_CHECKED);
            g_Render.SetPostEffectStarBurst(g_bStarBurst);
            RefreshStarBurst(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_GODRAY)
        {
            g_bGodRay = (IsDlgButtonChecked(hDlg, IDC_CHECK_GODRAY) == BST_CHECKED);
            ApplyGodRay();
            RefreshGodRayControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_SPECULAR_INTENSITY_OVERRIDE)
        {
            g_bUseSpecularIntensityOverride = (IsDlgButtonChecked(hDlg, IDC_CHECK_SPECULAR_INTENSITY_OVERRIDE) == BST_CHECKED);
            ApplySpecularIntensityOverride();
            RefreshSpecularIntensityControls(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_SPECULAR_EDGE_OVERRIDE)
        {
            g_bUseSpecularEdgeOverride = (IsDlgButtonChecked(hDlg, IDC_CHECK_SPECULAR_EDGE_OVERRIDE) == BST_CHECKED);
            ApplySpecularEdgeOverride();
            RefreshSpecularEdgeControls(hDlg);
            return TRUE;
        }

        if (commandId == IDCANCEL)
        {
            return TRUE;
        }

        if (commandId == IDOK)
        {
            DestroyWindow(hDlg);
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}
