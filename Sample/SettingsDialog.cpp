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
constexpr int GAUSSIAN_SLIDER_MIN = 1;
constexpr int GAUSSIAN_SLIDER_MAX = (GAUSSIAN_SAMPLE_MAX + 1) / 2;

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
    RefreshAnimateLight(hDlg);
    RefreshDepthBufferShadow(hDlg);
    RefreshGaussianControls(hDlg);
}

void InitializeTrackbars(HWND hDlg)
{
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETRANGEMIN, FALSE, SATURATE_SLIDER_MIN);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETRANGEMAX, FALSE, SATURATE_SLIDER_MAX);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETTICFREQ, 5, 0);
    SendDlgItemMessage(hDlg, IDC_SLIDER_SATURATE_LEVEL, TBM_SETPAGESIZE, 0, 5);

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

        if (commandId == IDC_CHECK_DEPTH_BUFFER_SHADOW)
        {
            g_bDepthBufferShadow = (IsDlgButtonChecked(hDlg, IDC_CHECK_DEPTH_BUFFER_SHADOW) == BST_CHECKED);
            g_Render.SetPostEffectDepthBufferShadow(g_bDepthBufferShadow);
            RefreshDepthBufferShadow(hDlg);
            return TRUE;
        }

        if (commandId == IDC_CHECK_GAUSSIAN_FILTER)
        {
            g_bGaussianFilter = (IsDlgButtonChecked(hDlg, IDC_CHECK_GAUSSIAN_FILTER) == BST_CHECKED);
            g_Render.SetPostEffectGaussianFilter(g_bGaussianFilter);
            RefreshGaussianControls(hDlg);
            return TRUE;
        }

        if (commandId == IDOK || commandId == IDCANCEL)
        {
            DestroyWindow(hDlg);
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}
