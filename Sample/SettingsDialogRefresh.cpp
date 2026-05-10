#include "SettingsDialog.h"

#include <commctrl.h>
#include <cwchar>

#include "AppState.h"
#include "../Render/Light.h"
#include "resource.h"

void RefreshHeightFogIntensityControls(HWND hDlg);
void RefreshHeightFogStartControls(HWND hDlg);
void RefreshHeightFogMaxControls(HWND hDlg);
void RefreshHeightFogDistanceStartControls(HWND hDlg);
void RefreshHeightFogDistanceMaxControls(HWND hDlg);

namespace
{
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
    const BOOL enabled = (g_ssaoMode != SampleSSAOMode::SSAO2);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO_BRIGHTNESS_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSAO_BRIGHTNESS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS), enabled);
}

void RefreshSSAO2ShadowStrengthControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssao2ShadowStrength);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO2_SHADOW_STRENGTH, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO2_SHADOW_STRENGTH,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAO2ShadowStrengthToSliderValue(g_ssao2ShadowStrength)));
    const BOOL enabled = (g_ssaoMode == SampleSSAOMode::SSAO2);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO2_SHADOW_STRENGTH_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSAO2_SHADOW_STRENGTH), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSAO2_SHADOW_STRENGTH), enabled);
}

void RefreshSSAO2ShadowSaturationControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssao2ShadowSaturationBoost);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO2_SHADOW_SATURATION, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO2_SHADOW_SATURATION,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAO2ShadowSaturationBoostToSliderValue(g_ssao2ShadowSaturationBoost)));
    const BOOL enabled = (g_ssaoMode == SampleSSAOMode::SSAO2);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO2_SHADOW_SATURATION_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSAO2_SHADOW_SATURATION), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSAO2_SHADOW_SATURATION), enabled);
}

void RefreshSSAO2SampleCountControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_ssao2SampleCount);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO2_SAMPLE_COUNT, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO2_SAMPLE_COUNT,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAO2SampleCountToSliderValue(g_ssao2SampleCount)));
    const BOOL enabled = (g_ssaoMode == SampleSSAOMode::SSAO2);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO2_SAMPLE_COUNT_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSAO2_SAMPLE_COUNT), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSAO2_SAMPLE_COUNT), enabled);
}

void RefreshSSAO2DepthScaledSampleDistanceControls(HWND hDlg)
{
    CheckDlgButton(hDlg,
                   IDC_CHECK_SSAO2_DEPTH_SCALED_SAMPLE_DISTANCE,
                   g_bSSAO2DepthScaledSampleDistance ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO2_DEPTH_SCALED_SAMPLE_DISTANCE), g_ssaoMode == SampleSSAOMode::SSAO2);
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

void RefreshCameraClipPlaneControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_cameraNearPlane);
    SetDlgItemText(hDlg, IDC_EDIT_CAMERA_NEAR, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_cameraFarPlane);
    SetDlgItemText(hDlg, IDC_EDIT_CAMERA_FAR, buffer);
}

void RefreshGBufferClipPlaneControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_gbufferNearPlane);
    SetDlgItemText(hDlg, IDC_EDIT_GBUFFER_NEAR, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_gbufferFarPlane);
    SetDlgItemText(hDlg, IDC_EDIT_GBUFFER_FAR, buffer);
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

void RefreshSSAO2BlurControls(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_SSAO2_BLUR, g_bSSAO2Blur ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO2_BLUR), g_ssaoMode == SampleSSAOMode::SSAO2);
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
