#pragma once

#include <windows.h>

INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void ShowSettingsDialog(HWND hWnd, bool activateDialog = true);
void ToggleSettingsDialog(HWND hWnd);
void RefreshSettingsDialogState();
