#include "SettingsDialog.h"

#include "AppState.h"
#include "resource.h"

namespace
{
void MoveSettingsDialogNearParent(HWND hDlg, HWND parent)
{
    if (parent == NULL)
    {
        return;
    }

    RECT parentRect { };
    RECT dialogRect { };
    GetWindowRect(parent, &parentRect);
    GetWindowRect(hDlg, &dialogRect);

    const int dialogWidth = dialogRect.right - dialogRect.left;
    const int dialogHeight = dialogRect.bottom - dialogRect.top;
    SetWindowPos(hDlg,
                 NULL,
                 parentRect.right + 8,
                 parentRect.top,
                 dialogWidth,
                 dialogHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void RefreshDialogChecks(HWND hDlg)
{
    CheckDlgButton(hDlg, IDC_CHECK_REMOTE_DESKTOP, g_bRemoteDesktop ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_MOVE_SPEED_BOOST_100X, g_bMoveSpeedBoost100x ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_ANIMATE_LIGHT, g_bAnimateLight ? BST_CHECKED : BST_UNCHECKED);
}
}

void ShowSettingsDialog(HWND hWnd, const bool activateDialog)
{
    if (g_hSettingsDialog != NULL && IsWindow(g_hSettingsDialog))
    {
        ShowWindow(g_hSettingsDialog, SW_SHOWNORMAL);
        RefreshDialogChecks(g_hSettingsDialog);
        if (activateDialog)
        {
            SetForegroundWindow(g_hSettingsDialog);
        }
        return;
    }

    g_hSettingsDialog = CreateDialog(GetModuleHandle(NULL),
                                     MAKEINTRESOURCE(IDD_SETTINGS_DIALOG),
                                     hWnd,
                                     SettingsDialogProc);
    if (g_hSettingsDialog == NULL)
    {
        return;
    }

    MoveSettingsDialogNearParent(g_hSettingsDialog, hWnd);
    ShowWindow(g_hSettingsDialog, SW_SHOWNORMAL);
    RefreshDialogChecks(g_hSettingsDialog);
    if (activateDialog)
    {
        SetForegroundWindow(g_hSettingsDialog);
    }
}

void ToggleSettingsDialog(HWND hWnd)
{
    if (g_hSettingsDialog != NULL && IsWindowVisible(g_hSettingsDialog))
    {
        ShowWindow(g_hSettingsDialog, SW_HIDE);
        return;
    }

    ShowSettingsDialog(hWnd, true);
}

void RefreshSettingsDialogState()
{
    if (g_hSettingsDialog != NULL && IsWindow(g_hSettingsDialog))
    {
        RefreshDialogChecks(g_hSettingsDialog);
    }
}

INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        RefreshDialogChecks(hDlg);
        return TRUE;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;
    case WM_DESTROY:
        if (g_hSettingsDialog == hDlg)
        {
            g_hSettingsDialog = NULL;
        }
        return TRUE;
    case WM_COMMAND:
    {
        const int commandId = LOWORD(wParam);
        const int notifyCode = HIWORD(wParam);
        if (commandId == IDCANCEL || commandId == IDOK)
        {
            ShowWindow(hDlg, SW_HIDE);
            return TRUE;
        }
        if (notifyCode != BN_CLICKED)
        {
            return FALSE;
        }
        if (commandId == IDC_CHECK_REMOTE_DESKTOP)
        {
            g_bRemoteDesktop = (IsDlgButtonChecked(hDlg, IDC_CHECK_REMOTE_DESKTOP) == BST_CHECKED);
            return TRUE;
        }
        if (commandId == IDC_CHECK_MOVE_SPEED_BOOST_100X)
        {
            g_bMoveSpeedBoost100x = (IsDlgButtonChecked(hDlg, IDC_CHECK_MOVE_SPEED_BOOST_100X) == BST_CHECKED);
            return TRUE;
        }
        if (commandId == IDC_CHECK_ANIMATE_LIGHT)
        {
            g_bAnimateLight = (IsDlgButtonChecked(hDlg, IDC_CHECK_ANIMATE_LIGHT) == BST_CHECKED);
            return TRUE;
        }
        return FALSE;
    }
    default:
        return FALSE;
    }
}
