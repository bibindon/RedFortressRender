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
constexpr int SUN_LIGHT_INTENSITY_SLIDER_MIN = 0;
constexpr int SUN_LIGHT_INTENSITY_SLIDER_MAX = static_cast<int>(SUN_LIGHT_INTENSITY_MAX / SUN_LIGHT_INTENSITY_STEP);
constexpr int SHADOW_SLIDER_MIN = 0;
constexpr int SHADOW_SLIDER_MAX = static_cast<int>(SHADOW_INTENSITY_MAX / SHADOW_INTENSITY_STEP);
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
constexpr int SSAO_BRIGHTNESS_SLIDER_MIN = 0;
constexpr int SSAO_BRIGHTNESS_SLIDER_MAX = static_cast<int>((SSAO_BRIGHTNESS_MAX - SSAO_BRIGHTNESS_MIN) / SSAO_BRIGHTNESS_STEP);
constexpr int SSAO_SATURATION_BOOST_SLIDER_MIN = 0;
constexpr int SSAO_SATURATION_BOOST_SLIDER_MAX = static_cast<int>(SSAO_SATURATION_BOOST_MAX / SSAO_SATURATION_BOOST_STEP);
constexpr int BLOOM_THRESHOLD_SLIDER_MIN = 0;
constexpr int BLOOM_THRESHOLD_SLIDER_MAX = static_cast<int>(BLOOM_THRESHOLD_MAX / BLOOM_THRESHOLD_STEP);
constexpr int DOF_FOCAL_DISTANCE_SLIDER_MIN = 0;
constexpr int DOF_FOCAL_DISTANCE_SLIDER_MAX = static_cast<int>((DOF_FOCAL_DISTANCE_MAX - DOF_FOCAL_DISTANCE_MIN) / DOF_FOCAL_DISTANCE_STEP);
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
constexpr int SETTINGS_DIALOG_CONTENT_HEIGHT_DLU = 866;
constexpr int SETTINGS_DIALOG_WHEEL_STEP_PX = 36;
constexpr UINT ID_POPUP_EXPORT_BINARY = 60001;
constexpr UINT ID_POPUP_REMOVE_MODEL = 60002;
constexpr UINT ID_POPUP_REMOVE_POINT_LIGHT = 60003;

int g_settingsDialogScrollPos = 0;

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

    column.cx = 96;
    column.pszText = const_cast<LPWSTR>(L"Pos");
    ListView_InsertColumn(listView, 0, &column);

    column.cx = 96;
    column.pszText = const_cast<LPWSTR>(L"Color");
    ListView_InsertColumn(listView, 1, &column);

    column.cx = 64;
    column.pszText = const_cast<LPWSTR>(L"Brightness");
    ListView_InsertColumn(listView, 2, &column);
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

        std::wstring colorText = FormatPointLightColor(pointLight.m_color);
        ListView_SetItemText(listView, i, 1, const_cast<LPWSTR>(colorText.c_str()));

        wchar_t brightnessBuffer[32];
        std::swprintf(brightnessBuffer,
                      sizeof(brightnessBuffer) / sizeof(brightnessBuffer[0]),
                      L"%.2f",
                      pointLight.m_brightness);
        ListView_SetItemText(listView, i, 2, brightnessBuffer);
    }
}

void RefreshPointLightControls(HWND hDlg)
{
    wchar_t buffer[32];

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

void RefreshBloom(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_BLOOM, g_bBloom ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshDepthOfField(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_DEPTH_OF_FIELD, g_bDepthOfField ? BST_CHECKED : BST_UNCHECKED);
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

void RefreshStarBurst(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_STARBURST, g_bStarBurst ? BST_CHECKED : BST_UNCHECKED);
}

void RefreshGaussianControls(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_GAUSSIAN_FILTER, g_bGaussianFilter ? BST_CHECKED : BST_UNCHECKED);

    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", g_gaussianSampleSize);
    SetDlgItemText(hDlg, IDC_EDIT_GAUSSIAN_SAMPLE_SIZE, buffer);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_GAUSSIAN_SAMPLE_SIZE,
                       TBM_SETPOS,
                       TRUE,
                       static_cast<LPARAM>(GaussianSampleSizeToSliderValue(g_gaussianSampleSize)));
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
    RefreshBloom(hDlg);
    RefreshDepthOfField(hDlg);
    RefreshStarBurst(hDlg);
    RefreshFogControls(hDlg);
    RefreshSunLightIntensityControls(hDlg);
    RefreshShadowControls(hDlg);
    RefreshShadowSaturationBoostControls(hDlg);
    RefreshHalfLambertShadowSaturationControls(hDlg);
    RefreshShadowDarknessControls(hDlg);
    RefreshSpecularIntensityControls(hDlg);
    RefreshSpecularEdgeControls(hDlg);
    RefreshSSAOBrightnessControls(hDlg);
    RefreshSSAOSaturationBoostControls(hDlg);
    RefreshBloomThresholdControls(hDlg);
    RefreshDepthOfFieldControls(hDlg);
    RefreshStarBurstThresholdControls(hDlg);
    RefreshModelLoadScaleControls(hDlg);
    RefreshGaussianControls(hDlg);
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

    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETRANGEMIN, FALSE, SUN_LIGHT_INTENSITY_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETRANGEMAX, FALSE, SUN_LIGHT_INTENSITY_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SUN_LIGHT_INTENSITY, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETRANGEMIN, FALSE, SHADOW_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETRANGEMAX, FALSE, SHADOW_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETPAGESIZE, 0, 2);

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

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETRANGEMIN, FALSE, SSAO_BRIGHTNESS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETRANGEMAX, FALSE, SSAO_BRIGHTNESS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETPAGESIZE, 0, 5);

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
}

bool HandleOpenMeshCommand(HWND hDlg, const WORD commandId)
{
    if (commandId == IDC_BUTTON_OPEN_MIX_MESH)
    {
        if (ShowOpenFileDialog(hDlg,
                               L"Mix Mesh Files (*.x;*.blend.x)\0*.x;*.blend.x\0All Files (*.*)\0*.*\0",
                               g_selectedMixMeshPath))
        {
            SpawnMeshMixAtCameraFront(g_selectedMixMeshPath);
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
            SpawnMeshAtCameraFront(g_selectedMeshPath);
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
            SpawnAnimMeshAtCameraFront(g_selectedAnimMeshPath);
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
            SpawnSkinAnimMeshAtCameraFront(g_selectedSkinAnimMeshPath);
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
        InitializeTrackbars(hDlg);
        PopulateResolutionCombo(hDlg);
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

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_FOG_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_fogIntensity = SliderValueToFogIntensity(sliderValue);
            ApplyFogIntensity();
            RefreshFogControls(hDlg);
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

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowIntensity = SliderValueToShadowIntensity(sliderValue);
            ApplyShadowIntensity();
            RefreshShadowControls(hDlg);
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

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_ssaoBrightness = SliderValueToSSAOBrightness(sliderValue);
            ApplySSAOBrightness();
            RefreshSSAOBrightnessControls(hDlg);
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

        if (commandId == IDC_CHECK_BLOOM)
        {
            g_bBloom = (IsDlgButtonChecked(hDlg, IDC_CHECK_BLOOM) == BST_CHECKED);
            g_Render.SetPostEffectBloom(g_bBloom);
            RefreshBloom(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_DEPTH_OF_FIELD)
        {
            g_bDepthOfField = (IsDlgButtonChecked(hDlg, IDC_CHECK_DEPTH_OF_FIELD) == BST_CHECKED);
            g_Render.SetPostEffectDepthOfField(g_bDepthOfField);
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
