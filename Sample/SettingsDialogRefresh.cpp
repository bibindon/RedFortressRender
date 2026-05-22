#include "SettingsDialog.h"

#include <commctrl.h>
#include <cwchar>

#include "AppState.h"
#include "../Render/Light.h"
#include "resource.h"

// このファイルは設定ダイアログの表示更新だけを担当する。
// 値の確定処理や WM_COMMAND の分岐は SettingsDialog.cpp 側にまとめ、
// ここでは AppState に保持している現在値を UI へ反映する。
void RefreshHeightFogIntensityControls(HWND hDlg);
void RefreshHeightFogStartControls(HWND hDlg);
void RefreshHeightFogMaxControls(HWND hDlg);
void RefreshHeightFogDistanceStartControls(HWND hDlg);
void RefreshHeightFogDistanceMaxControls(HWND hDlg);

void RefreshFog(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bFog)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_FOG, checkState);
}

void RefreshHeightFog(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bHeightFog)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_HEIGHT_FOG, checkState);
}

void RefreshGBuffer(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bGBuffer)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_GBUFFER, checkState);
}

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

    // 色と強度は常に最新の実行時状態を表示する。
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

    // ライト形状ごとに必要な入力項目だけを表示する。
    const int lineControlIds[] =
    {
        IDC_STATIC_POINT_LIGHT_LINE_LENGTH_LABEL,
        IDC_EDIT_POINT_LIGHT_LINE_LENGTH,
    };

    for (const int controlId : lineControlIds)
    {
        HWND control = GetDlgItem(hDlg, controlId);
        int showMode = SW_HIDE;
        BOOL enabled = FALSE;
        if (isLineLight)
        {
            showMode = SW_SHOW;
            enabled = TRUE;
        }
        ShowWindow(control, showMode);
        EnableWindow(control, enabled);
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
        int showMode = SW_HIDE;
        BOOL enabled = FALSE;
        if (usesRotation)
        {
            showMode = SW_SHOW;
            enabled = TRUE;
        }
        ShowWindow(control, showMode);
        EnableWindow(control, enabled);
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
        int showMode = SW_HIDE;
        BOOL enabled = FALSE;
        if (isSquareLight)
        {
            showMode = SW_SHOW;
            enabled = TRUE;
        }
        ShowWindow(control, showMode);
        EnableWindow(control, enabled);
    }
}

void RefreshParticlePlacementControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_PARTICLE_EFFECT);
    if (combo == NULL)
    {
        return;
    }

    int comboIndex = 0;
    switch (g_particleEffectPreset)
    {
    case NSRender::ParticleEffectPreset::Fire:
        comboIndex = 1;
        break;
    case NSRender::ParticleEffectPreset::Dust:
        comboIndex = 2;
        break;
    case NSRender::ParticleEffectPreset::Fog:
        comboIndex = 3;
        break;
    case NSRender::ParticleEffectPreset::Rain:
        comboIndex = 4;
        break;
    case NSRender::ParticleEffectPreset::Smoke:
    default:
        comboIndex = 0;
        break;
    }

    SendMessage(combo, CB_SETCURSEL, comboIndex, 0);
}

void RefreshFogControls(HWND hDlg)
{
    const BOOL fogColorEnabled = (g_bFog || g_bHeightFog) ? TRUE : FALSE;
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_fogIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_FOG_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FOG_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FogIntensityToSliderValue(g_fogIntensity)));
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FOG_INTENSITY), g_bFog);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_FOG_INTENSITY), g_bFog);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FOG_INTENSITY_LABEL), g_bFog);

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_fogColor.r);
    SetDlgItemText(hDlg, IDC_EDIT_FOG_COLOR_R, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FOG_COLOR_R,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FogColorToSliderValue(g_fogColor.r)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_fogColor.g);
    SetDlgItemText(hDlg, IDC_EDIT_FOG_COLOR_G, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FOG_COLOR_G,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FogColorToSliderValue(g_fogColor.g)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_fogColor.b);
    SetDlgItemText(hDlg, IDC_EDIT_FOG_COLOR_B, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FOG_COLOR_B,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FogColorToSliderValue(g_fogColor.b)));

    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FOG_COLOR_R_LABEL), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FOG_COLOR_R), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_FOG_COLOR_R), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FOG_COLOR_G_LABEL), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FOG_COLOR_G), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_FOG_COLOR_G), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FOG_COLOR_B_LABEL), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FOG_COLOR_B), fogColorEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_FOG_COLOR_B), fogColorEnabled);
}

void RefreshHeightFogControls(HWND hDlg)
{
    RefreshHeightFogIntensityControls(hDlg);
    RefreshHeightFogStartControls(hDlg);
    RefreshHeightFogMaxControls(hDlg);
    RefreshHeightFogDistanceStartControls(hDlg);
    RefreshHeightFogDistanceMaxControls(hDlg);

    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_HEIGHT_FOG_INTENSITY_LABEL), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HEIGHT_FOG_INTENSITY), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_INTENSITY), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_HEIGHT_FOG_START_LABEL), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HEIGHT_FOG_START), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_START), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_HEIGHT_FOG_MAX_LABEL), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HEIGHT_FOG_MAX), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_MAX), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_HEIGHT_FOG_DISTANCE_START_LABEL), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HEIGHT_FOG_DISTANCE_START), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_START), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_HEIGHT_FOG_DISTANCE_MAX_LABEL), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_HEIGHT_FOG_DISTANCE_MAX), g_bHeightFog);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_HEIGHT_FOG_DISTANCE_MAX), g_bHeightFog);
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

    UINT checkState = BST_UNCHECKED;
    if (g_bUseSpecularIntensityOverride)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SPECULAR_INTENSITY_OVERRIDE, checkState);

    // Override が無効な間は値を編集できないようにする。
    BOOL enabled = FALSE;
    if (g_bUseSpecularIntensityOverride)
    {
        enabled = TRUE;
    }
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

    UINT checkState = BST_UNCHECKED;
    if (g_bUseSpecularEdgeOverride)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SPECULAR_EDGE_OVERRIDE, checkState);

    BOOL enabled = FALSE;
    if (g_bUseSpecularEdgeOverride)
    {
        enabled = TRUE;
    }
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SPECULAR_EDGE), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SPECULAR_EDGE), enabled);
}

void RefreshFresnelIntensityControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_fresnelIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_FRESNEL_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FRESNEL_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FresnelIntensityToSliderValue(g_fresnelIntensity)));
}

void RefreshPhongTreatTextureAsWhiteControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bPhongTreatTextureAsWhite)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_PHONG_TREAT_TEXTURE_AS_WHITE, checkState);
}

void RefreshEnvMapBlendControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_envMapBlend);
    SetDlgItemText(hDlg, IDC_EDIT_ENVMAP_BLEND, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_ENVMAP_BLEND,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(EnvMapBlendToSliderValue(g_envMapBlend)));
}

void RefreshPBRRoughnessControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_pbrRoughness);
    SetDlgItemText(hDlg, IDC_EDIT_PBR_ROUGHNESS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_PBR_ROUGHNESS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PBRRoughnessToSliderValue(g_pbrRoughness)));
}

void RefreshPBRMetallicControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_pbrMetallic);
    SetDlgItemText(hDlg, IDC_EDIT_PBR_METALLIC, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_PBR_METALLIC,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PBRMetallicToSliderValue(g_pbrMetallic)));
}

void RefreshPBREnvReflectionIntensityControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_pbrEnvReflectionIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_PBR_ENV_REFLECTION_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_PBR_ENV_REFLECTION_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PBREnvReflectionIntensityToSliderValue(g_pbrEnvReflectionIntensity)));
}

void RefreshPBREnvMaxMipLevelControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_pbrEnvMaxMipLevel);
    SetDlgItemText(hDlg, IDC_EDIT_PBR_ENV_MAX_MIP_LEVEL, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_PBR_ENV_MAX_MIP_LEVEL,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PBREnvMaxMipLevelToSliderValue(g_pbrEnvMaxMipLevel)));
}

void RefreshPBREnvDiffuseIntensityControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_pbrEnvDiffuseIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_PBR_ENV_DIFFUSE_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_PBR_ENV_DIFFUSE_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PBREnvDiffuseIntensityToSliderValue(g_pbrEnvDiffuseIntensity)));
}

void RefreshPBREnvDiffuseMipLevelControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_pbrEnvDiffuseMipLevel);
    SetDlgItemText(hDlg, IDC_EDIT_PBR_ENV_DIFFUSE_MIP_LEVEL, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_PBR_ENV_DIFFUSE_MIP_LEVEL,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(PBREnvDiffuseMipLevelToSliderValue(g_pbrEnvDiffuseMipLevel)));
}

void RefreshSSSControls(HWND hDlg)
{
    wchar_t buffer[32];
    UINT checkState = BST_UNCHECKED;
    if (g_bSSS)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SSS, checkState);

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

    BOOL enabled = FALSE;
    if (g_bSSS)
    {
        enabled = TRUE;
    }
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_INTENSITY), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_INTENSITY), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_COLOR_R), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_R), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_COLOR_G), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_G), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSS_COLOR_B), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSS_COLOR_B), enabled);
}

void RefreshSSAOShadowStrengthControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssaoShadowStrength);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO_SHADOW_STRENGTH, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO_SHADOW_STRENGTH,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAOShadowStrengthToSliderValue(g_ssaoShadowStrength)));
    const BOOL enabled = g_bSSAO;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO_SHADOW_STRENGTH_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSAO_SHADOW_STRENGTH), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSAO_SHADOW_STRENGTH), enabled);
}

void RefreshSSAOShadowSaturationControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssaoShadowSaturationBoost);
    SetDlgItemText(hDlg, IDC_EDIT_SSAO_SHADOW_SATURATION, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSAO_SHADOW_SATURATION,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSAOShadowSaturationBoostToSliderValue(g_ssaoShadowSaturationBoost)));
    const BOOL enabled = g_bSSAO;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO_SHADOW_SATURATION_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSAO_SHADOW_SATURATION), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSAO_SHADOW_SATURATION), enabled);
}

void RefreshSSAOSampleCountControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SSAO_SAMPLE_COUNT);
    if (combo != NULL)
    {
        SendMessage(combo,
                    CB_SETCURSEL,
                    static_cast<WPARAM>(SSAOSampleCountToComboIndex(g_ssaoSampleCount)),
                    0);
    }

    const BOOL enabled = g_bSSAO;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO_SAMPLE_COUNT_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SSAO_SAMPLE_COUNT), enabled);
}

void RefreshSSAORandomSamplingDirectionControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSAORandomSamplingDirection)
    {
        checkState = BST_CHECKED;
    }

    CheckDlgButton(hDlg, IDC_CHECK_SSAO_RANDOM_SAMPLING_DIRECTION, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO_RANDOM_SAMPLING_DIRECTION), g_bSSAO);
}

void RefreshSSAODepthScaledSampleDistanceControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSAODepthScaledSampleDistance)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SSAO_DEPTH_SCALED_SAMPLE_DISTANCE, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO_DEPTH_SCALED_SAMPLE_DISTANCE), g_bSSAO);
}

void RefreshSSAOMaxDarknessClampControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSAOMaxDarknessClamp)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SSAO_MAX_DARKNESS_CLAMP, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO_MAX_DARKNESS_CLAMP), g_bSSAO);
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
    const BOOL enabled = g_bSSAO;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO_SAMPLE_RADIUS_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSAO_SAMPLE_RADIUS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSAO_SAMPLE_RADIUS), enabled);
}

void RefreshSSGISampleRadiusControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssgiSampleRadius);
    SetDlgItemText(hDlg, IDC_EDIT_SSGI_SAMPLE_RADIUS, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_SSGI_SAMPLE_RADIUS,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(SSGISampleRadiusToSliderValue(g_ssgiSampleRadius)));
    const BOOL enabled = g_bSSGI;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSGI_SAMPLE_RADIUS_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSGI_SAMPLE_RADIUS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_SSGI_SAMPLE_RADIUS), enabled);
}

void RefreshSSAOBlurKernelSizeControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SSAO_BLUR_KERNEL_SIZE);
    if (combo != NULL)
    {
        SendMessage(combo,
                    CB_SETCURSEL,
                    static_cast<WPARAM>(SSAOBlurKernelSizeToComboIndex(g_ssaoBlurKernelSize)),
                    0);
    }

    const BOOL enabled = g_bSSAO && g_bSSAOBlur;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO_BLUR_KERNEL_SIZE_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SSAO_BLUR_KERNEL_SIZE), enabled);
}

void RefreshSSAOSeparableBlurControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSAOSeparableBlur)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SSAO_SEPARABLE_BLUR, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO_SEPARABLE_BLUR), g_bSSAO && g_bSSAOBlur);
}

void RefreshSSAOTexSizeControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SSAO_TEX_SIZE);
    if (combo == NULL)
    {
        return;
    }

    int comboIndex = 0;
    if (g_ssaoTexSizeDivisor == 2)
    {
        comboIndex = 1;
    }
    else if (g_ssaoTexSizeDivisor == 4)
    {
        comboIndex = 2;
    }

    SendMessage(combo, CB_SETCURSEL, static_cast<WPARAM>(comboIndex), 0);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSAO_TEX_SIZE_LABEL), g_bSSAO);
    EnableWindow(combo, g_bSSAO);
}

void RefreshSSAOCompositeGaussian3x3Controls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSAOCompositeGaussian3x3)
    {
        checkState = BST_CHECKED;
    }

    CheckDlgButton(hDlg, IDC_CHECK_SSAO_COMPOSITE_GAUSSIAN_3X3, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO_COMPOSITE_GAUSSIAN_3X3), g_bSSAO);
}

void RefreshCameraClipPlaneControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_cameraNearPlane);
    SetDlgItemText(hDlg, IDC_EDIT_CAMERA_NEAR, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_cameraFarPlane);
    SetDlgItemText(hDlg, IDC_EDIT_CAMERA_FAR, buffer);
}

void RefreshCameraShakeControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_cameraShakeDurationSeconds);
    SetDlgItemText(hDlg, IDC_EDIT_CAMERA_SHAKE_DURATION, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_CAMERA_SHAKE_DURATION,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(CameraShakeDurationToSliderValue(g_cameraShakeDurationSeconds)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_cameraShakeIntensity);
    SetDlgItemText(hDlg, IDC_EDIT_CAMERA_SHAKE_INTENSITY, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_CAMERA_SHAKE_INTENSITY,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(CameraShakeIntensityToSliderValue(g_cameraShakeIntensity)));
}

void RefreshGBufferClipPlaneControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.3f", g_gbufferNearPlane);
    SetDlgItemText(hDlg, IDC_EDIT_GBUFFER_NEAR, buffer);
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_gbufferFarPlane);
    SetDlgItemText(hDlg, IDC_EDIT_GBUFFER_FAR, buffer);

    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_GBUFFER_NEAR_LABEL), g_bGBuffer);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_GBUFFER_NEAR), g_bGBuffer);
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_GBUFFER_FAR_LABEL), g_bGBuffer);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_GBUFFER_FAR), g_bGBuffer);
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

void RefreshBloomWeightSumControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.0f", g_bloomWeightSum);
    SetDlgItemText(hDlg, IDC_EDIT_BLOOM_WEIGHT_SUM, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_BLOOM_WEIGHT_SUM,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(BloomWeightSumToSliderValue(g_bloomWeightSum)));
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

void RefreshStarBurstDistanceFadeControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_starBurstDistanceFade);
    SetDlgItemText(hDlg, IDC_EDIT_STARBURST_DISTANCE_FADE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_STARBURST_DISTANCE_FADE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(StarBurstDistanceFadeToSliderValue(g_starBurstDistanceFade)));
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
    UINT checkState = BST_UNCHECKED;
    if (g_bAnimateLight)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_ANIMATE_LIGHT, checkState);
}

void RefreshMoveSpeedBoost100x(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bMoveSpeedBoost100x)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_MOVE_SPEED_BOOST_100X, checkState);
}

void RefreshRemoteDesktop(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bRemoteDesktop)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_REMOTE_DESKTOP, checkState);
}

void RefreshZShadowTexSizeControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_ZSHADOW_TEX_SIZE);
    if (combo == NULL)
    {
        return;
    }

    int comboIndex = 0;
    if (g_zShadowTexSizeDivisor == 2)
    {
        comboIndex = 1;
    }
    else if (g_zShadowTexSizeDivisor == 4)
    {
        comboIndex = 2;
    }
    else if (g_zShadowTexSizeDivisor == 8)
    {
        comboIndex = 3;
    }
    else if (g_zShadowTexSizeDivisor == 16)
    {
        comboIndex = 4;
    }

    SendMessage(combo, CB_SETCURSEL, static_cast<WPARAM>(comboIndex), 0);
}

void RefreshDepthBufferShadow(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bDepthBufferShadow)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_DEPTH_BUFFER_SHADOW, checkState);
    RefreshZShadowTexSizeControls(hDlg);
}

void RefreshSSAO(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSAO)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SSAO, checkState);
}

void RefreshSSGI(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSGI)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SSGI, checkState);
}

void RefreshSSGIIndirectLightControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssgiIndirectLightStrength);
    SetDlgItemText(hDlg, IDC_EDIT_SSGI_INDIRECT_LIGHT, buffer);

    const BOOL enabled = g_bSSGI;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSGI_INDIRECT_LIGHT_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSGI_INDIRECT_LIGHT), enabled);
}

void RefreshSSGISampleCountControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SSGI_SAMPLE_COUNT);
    if (combo != NULL)
    {
        SendMessage(combo,
                    CB_SETCURSEL,
                    static_cast<WPARAM>(SSGISampleCountToComboIndex(g_ssgiSampleCount)),
                    0);
    }

    const BOOL enabled = g_bSSGI;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSGI_SAMPLE_COUNT_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SSGI_SAMPLE_COUNT), enabled);
}

void RefreshSSGIBlurKernelSizeControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SSGI_BLUR_KERNEL_SIZE);
    if (combo != NULL)
    {
        SendMessage(combo,
                    CB_SETCURSEL,
                    static_cast<WPARAM>(SSGIBlurKernelSizeToComboIndex(g_ssgiBlurKernelSize)),
                    0);
    }

    const BOOL enabled = g_bSSGI && g_bSSGIBlur;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSGI_BLUR_KERNEL_SIZE_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SSGI_BLUR_KERNEL_SIZE), enabled);
}

void RefreshSSGISeparableBlurControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSGISeparableBlur)
    {
        checkState = BST_CHECKED;
    }

    CheckDlgButton(hDlg, IDC_CHECK_SSGI_SEPARABLE_BLUR, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSGI_SEPARABLE_BLUR), g_bSSGI && g_bSSGIBlur);
}

void RefreshSSGIBlurControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSGIBlur)
    {
        checkState = BST_CHECKED;
    }

    CheckDlgButton(hDlg, IDC_CHECK_SSGI_BLUR, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSGI_BLUR), g_bSSGI);
    RefreshSSGIBlurKernelSizeControls(hDlg);
    RefreshSSGISeparableBlurControls(hDlg);
}

void RefreshSSGIIndirectLightMaxControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_ssgiIndirectLightMaxContribution);
    SetDlgItemText(hDlg, IDC_EDIT_SSGI_INDIRECT_LIGHT_MAX, buffer);

    const BOOL enabled = g_bSSGI;
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_SSGI_INDIRECT_LIGHT_MAX_LABEL), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SSGI_INDIRECT_LIGHT_MAX), enabled);
}

void RefreshSSAOBlurControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bSSAOBlur)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_SSAO_BLUR, checkState);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SSAO_BLUR), g_bSSAO);
    RefreshSSAOBlurKernelSizeControls(hDlg);
    RefreshSSAOSeparableBlurControls(hDlg);
}

void RefreshBloom(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bBloom)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_BLOOM, checkState);
}

void RefreshHalo(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bHalo)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_HALO, checkState);
}

void RefreshPostEffectAA(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bPostEffectAA)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_POST_EFFECT_AA, checkState);
}

void RefreshTAA(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bTAA)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_TAA, checkState);
}

void RefreshDepthOfField(HWND hDlg)
{
    // DOF は OFF / ON / AutoNear の 3 状態をラジオボタンで同期する。
    UINT dofOffState = BST_UNCHECKED;
    if (g_depthOfFieldMode == NSRender::DepthOfFieldMode::Disabled)
    {
        dofOffState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_RADIO_DEPTH_OF_FIELD_OFF, dofOffState);

    UINT dofOnState = BST_UNCHECKED;
    if (g_depthOfFieldMode == NSRender::DepthOfFieldMode::Enabled)
    {
        dofOnState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_RADIO_DEPTH_OF_FIELD_ON, dofOnState);

    UINT dofAutoState = BST_UNCHECKED;
    if (g_depthOfFieldMode == NSRender::DepthOfFieldMode::AutoNear)
    {
        dofAutoState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_RADIO_DEPTH_OF_FIELD_AUTO, dofAutoState);
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

void RefreshDepthOfFieldStartNearControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_dofStartNear);
    SetDlgItemText(hDlg, IDC_EDIT_DOF_START_NEAR, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_DOF_START_NEAR,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(DepthOfFieldStartNearToSliderValue(g_dofStartNear)));
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
    UINT checkState = BST_UNCHECKED;
    if (g_bStarBurst)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_STARBURST, checkState);
}

void RefreshGaussianControls(HWND hDlg)
{
    UINT gaussianState = BST_UNCHECKED;
    if (g_bGaussianFilter)
    {
        gaussianState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_GAUSSIAN_FILTER, gaussianState);

    UINT maskedGaussianState = BST_UNCHECKED;
    if (g_bMaskedGaussianFilter)
    {
        maskedGaussianState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_MASKED_GAUSSIAN_FILTER, maskedGaussianState);

    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_gaussianSampleSize);
    SetDlgItemText(hDlg, IDC_EDIT_GAUSSIAN_SAMPLE_SIZE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(GaussianSampleSizeToSliderValue(g_gaussianSampleSize)));

    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", g_gaussianStrength);
    SetDlgItemText(hDlg, IDC_EDIT_GAUSSIAN_STRENGTH, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_GAUSSIAN_STRENGTH,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(GaussianStrengthToSliderValue(g_gaussianStrength)));
}

void RefreshFontExGaussianControls(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_fontExGaussianSampleSize);
    SetDlgItemText(hDlg, IDC_EDIT_FONTEX_GAUSSIAN_SAMPLE_SIZE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FONTEX_GAUSSIAN_SAMPLE_SIZE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(FontExGaussianSampleSizeToSliderValue(g_fontExGaussianSampleSize)));
}

void RefreshFXAAControls(HWND hDlg)
{
    UINT checkState = BST_UNCHECKED;
    if (g_bFXAA)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_FXAA, checkState);

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
    UINT checkState = BST_UNCHECKED;
    if (g_bMotionBlurCamera)
    {
        checkState = BST_CHECKED;
    }
    CheckDlgButton(hDlg, IDC_CHECK_MOTION_BLUR_CAMERA, checkState);

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

    // Motion Blur が OFF の間は関連入力をまとめて無効化する。
    BOOL enabled = FALSE;
    if (g_bMotionBlurCamera)
    {
        enabled = TRUE;
    }
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MOTION_BLUR_CAMERA_SAMPLE_COUNT), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_MOTION_BLUR_CAMERA_SAMPLE_COUNT), enabled);
}

void RefreshShadowPcfTapControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SHADOW_PCF_TAPS);
    if (combo != NULL)
    {
        SendMessage(combo,
                    CB_SETCURSEL,
                    static_cast<WPARAM>(ShadowTapCountToComboIndex(g_shadowPcfTapCount)),
                    0);
    }
}

void RefreshShadowCompositeTapControls(HWND hDlg)
{
    HWND combo = GetDlgItem(hDlg, IDC_COMBO_SHADOW_COMPOSITE_TAPS);
    if (combo != NULL)
    {
        SendMessage(combo,
                    CB_SETCURSEL,
                    static_cast<WPARAM>(ShadowTapCountToComboIndex(g_shadowCompositeTapCount)),
                    0);
    }
}

void RefreshGodRayControls(HWND hDlg)
{

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

