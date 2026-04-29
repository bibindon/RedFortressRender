#include "SettingsDialog.h"

#include <cassert>
#include <commctrl.h>
#include <string>
#include <cwchar>

#include "AppState.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
constexpr int SATURATE_SLIDER_MIN = 0;
constexpr int SATURATE_SLIDER_MAX = static_cast<int>(SATURATE_MAX / SATURATE_STEP);
constexpr int FOG_SLIDER_MIN = 0;
constexpr int FOG_SLIDER_MAX = static_cast<int>(FOG_INTENSITY_MAX / FOG_INTENSITY_STEP);
constexpr int SHADOW_SLIDER_MIN = 0;
constexpr int SHADOW_SLIDER_MAX = static_cast<int>(SHADOW_INTENSITY_MAX / SHADOW_INTENSITY_STEP);
constexpr int SSAO_BRIGHTNESS_SLIDER_MIN = 0;
constexpr int SSAO_BRIGHTNESS_SLIDER_MAX = static_cast<int>((SSAO_BRIGHTNESS_MAX - SSAO_BRIGHTNESS_MIN) / SSAO_BRIGHTNESS_STEP);
constexpr int BLOOM_THRESHOLD_SLIDER_MIN = 0;
constexpr int BLOOM_THRESHOLD_SLIDER_MAX = static_cast<int>(BLOOM_THRESHOLD_MAX / BLOOM_THRESHOLD_STEP);
constexpr int STARBURST_THRESHOLD_SLIDER_MIN = 0;
constexpr int STARBURST_THRESHOLD_SLIDER_MAX = static_cast<int>(STARBURST_THRESHOLD_MAX / STARBURST_THRESHOLD_STEP);
constexpr int MODEL_LOAD_SCALE_SLIDER_MIN = 0;
constexpr int MODEL_LOAD_SCALE_SLIDER_MAX = static_cast<int>((MODEL_LOAD_SCALE_MAX - MODEL_LOAD_SCALE_MIN) / MODEL_LOAD_SCALE_STEP);
constexpr int GAUSSIAN_SLIDER_MIN = 1;
constexpr int GAUSSIAN_SLIDER_MAX = (GAUSSIAN_SAMPLE_MAX + 1) / 2;

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
    RefreshLoadedModelListView(hDlg);
    RefreshAnimateLight(hDlg);
    RefreshDepthBufferShadow(hDlg);
    RefreshSSAO(hDlg);
    RefreshBloom(hDlg);
    RefreshStarBurst(hDlg);
    RefreshFogControls(hDlg);
    RefreshShadowControls(hDlg);
    RefreshSSAOBrightnessControls(hDlg);
    RefreshBloomThresholdControls(hDlg);
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

    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETRANGEMIN, FALSE, SHADOW_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETRANGEMAX, FALSE, SHADOW_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETTICFREQ, 2, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SHADOW_INTENSITY, TBM_SETPAGESIZE, 0, 2);

    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETRANGEMIN, FALSE, SSAO_BRIGHTNESS_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETRANGEMAX, FALSE, SSAO_BRIGHTNESS_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SSAO_BRIGHTNESS, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETRANGEMIN, FALSE, BLOOM_THRESHOLD_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETRANGEMAX, FALSE, BLOOM_THRESHOLD_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_BLOOM_THRESHOLD, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETRANGEMIN, FALSE, STARBURST_THRESHOLD_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETRANGEMAX, FALSE, STARBURST_THRESHOLD_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_STARBURST_THRESHOLD, TBM_SETPAGESIZE, 0, 5);

    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETRANGEMIN, FALSE, MODEL_LOAD_SCALE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETRANGEMAX, FALSE, MODEL_LOAD_SCALE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETTICFREQ, 50, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_MODEL_LOAD_SCALE, TBM_SETPAGESIZE, 0, 50);

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
        InitializeLoadedModelListView(hDlg);
        RefreshAllControls(hDlg);
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

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_SHADOW_INTENSITY))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_shadowIntensity = SliderValueToShadowIntensity(sliderValue);
            ApplyShadowIntensity();
            RefreshShadowControls(hDlg);
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

        if (slider == GetDlgItem(hDlg, IDC_SLIDER_BLOOM_THRESHOLD))
        {
            const int sliderValue = static_cast<int>(SendMessage(slider, TBM_GETPOS, 0, 0));
            g_bloomThreshold = SliderValueToBloomThreshold(sliderValue);
            ApplyBloomThreshold();
            RefreshBloomThresholdControls(hDlg);
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
