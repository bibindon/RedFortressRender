#include <cassert>
#include <crtdbg.h>
#include <vector>
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <windowsx.h>

#include "AppState.h"
#include "InputHandlers.h"
#include "SettingsDialog.h"
#include "resource.h"

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern int WINAPI _tWinMain(_In_ HINSTANCE hInstance,
                            _In_opt_ HINSTANCE hPrevInstance,
                            _In_ LPTSTR lpCmdLine,
                            _In_ int nCmdShow);

namespace
{
bool RegisterRawMouseInput(const HWND hWnd)
{
    RAWINPUTDEVICE rawInputDevice { };
    rawInputDevice.usUsagePage = 0x01;
    rawInputDevice.usUsage = 0x02;
    rawInputDevice.dwFlags = 0;
    rawInputDevice.hwndTarget = hWnd;
    return RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice)) == TRUE;
}

bool TryGetRawMouseDelta(const LPARAM lParam, LONG& deltaX, LONG& deltaY)
{
    deltaX = 0;
    deltaY = 0;

    UINT rawInputSize = 0;
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
                        RID_INPUT,
                        NULL,
                        &rawInputSize,
                        sizeof(RAWINPUTHEADER)) != 0 ||
        rawInputSize == 0)
    {
        return false;
    }

    std::vector<BYTE> rawInputBuffer(rawInputSize);
    const UINT bytesRead = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
                                           RID_INPUT,
                                           rawInputBuffer.data(),
                                           &rawInputSize,
                                           sizeof(RAWINPUTHEADER));
    if (bytesRead != rawInputSize)
    {
        return false;
    }

    const RAWINPUT* rawInput = reinterpret_cast<const RAWINPUT*>(rawInputBuffer.data());
    if (rawInput->header.dwType != RIM_TYPEMOUSE)
    {
        return false;
    }

    deltaX = rawInput->data.mouse.lLastX;
    deltaY = rawInput->data.mouse.lLastY;
    return true;
}

void InitializeCommonControlsForSample()
{
    INITCOMMONCONTROLSEX icc { };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);
}

HWND CreateSampleWindow(const HINSTANCE hInstance)
{
    WNDCLASSEX wc { };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = MsgProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = _T("Window1");

    const ATOM atom = RegisterClassEx(&wc);
    assert(atom != 0);

    RECT rect { };
    SetRect(&rect, 0, 0, WINDOW_SIZE_W, WINDOW_SIZE_H);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.top = 0;
    rect.left = 0;

    return CreateWindow(_T("Window1"),
                        _T("Hello DirectX9 World !!"),
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        rect.right,
                        rect.bottom,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);
}

void InitializeSampleScene(HWND hWnd)
{
    const bool rawMouseRegistered = RegisterRawMouseInput(hWnd);
    assert(rawMouseRegistered);

    InitializeRemoteDesktopDefault();
    LoadSampleSettingsFromCsv(L"RenderSettings.csv");
    g_Render.Initialize(hWnd, L"RenderSettings.csv");
    g_Render.SetCamera(D3DXVECTOR3(0.0f, 1.7f, -2.0f), D3DXVECTOR3(0.0f, 1.5f, 0.0f));
    g_fontId = g_Render.SetUpFont(L"BIZ UDゴシック", 20, D3DCOLOR_RGBA(255, 255, 255, 255));
    g_fontExId = g_Render.SetUpFontEx(L"BIZ UDゴシック", 20, D3DCOLOR_RGBA(255, 255, 255, 255));
    const int cubeNormalInverseId = g_Render.AddMeshMix(L"..\\..\\Sample\\res\\model2\\cubeNormalInverse.x",
                                                        D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                        D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                        1.0f,
                                                        100.0f,
                                                        false,
                                                        false);
    RegisterLoadedModel(L"MeshMixManager", L"..\\..\\Sample\\res\\model2\\cubeNormalInverse.x", D3DXVECTOR3(0.0f, 0.0f, 0.0f), 1.0f, cubeNormalInverseId);
    const int plateFieldId = g_Render.AddMeshMix(L"..\\..\\Sample\\res\\model2\\plateField.x",
                                                 D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                 D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                 1.0f,
                                                 100.0f,
                                                 false,
                                                 false);
    RegisterLoadedModel(L"MeshMixManager", L"..\\..\\Sample\\res\\model2\\plateField.x", D3DXVECTOR3(0.0f, 0.0f, 0.0f), 1.0f, plateFieldId);
    ApplyAllSampleSettings();
}

void TickAndRenderFrame()
{
    DrawSampleOverlay();
    UpdateDirectionalLight();
    UpdateCameraMoveByKeyboard();
    if (g_bGodRay)
    {
        ApplyGodRayLightPos();
    }
    g_Render.Draw();
}
}

int WINAPI _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE,
                     _In_ LPTSTR,
                     _In_ int)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    InitializeCommonControlsForSample();

    HWND hWnd = CreateSampleWindow(GetModuleHandle(NULL));
    InitializeSampleScene(hWnd);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);
    ShowSettingsDialog(hWnd, false);
    ShowMouseCursor();

    MSG msg { };

    while (!g_bClose)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (g_hSettingsDialog != NULL &&
                IsWindowVisible(g_hSettingsDialog) &&
                IsDialogMessage(g_hSettingsDialog, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!g_bClose)
        {
            TickAndRenderFrame();
        }
    }

    g_Render.Finalize();
    UnregisterClass(_T("Window1"), hInstance);
    return 0;
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        ShowMouseCursor();
        PostQuitMessage(0);
        g_bClose = true;
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (!g_bMouseLookEnabled)
        {
            return 0;
        }
        if (!g_bRemoteDesktop)
        {
            return 0;
        }

        if (g_bRecenteringMouse)
        {
            g_bRecenteringMouse = false;
            return 0;
        }

        POINT currentMousePos { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        int mouseMoveX = 0;
        int mouseMoveY = 0;

        if (g_bRemoteDesktop)
        {
            if (!g_bPrevMouseClientPosValid)
            {
                g_prevMouseClientPos = currentMousePos;
                g_bPrevMouseClientPosValid = true;
                return 0;
            }

            mouseMoveX = currentMousePos.x - g_prevMouseClientPos.x;
            mouseMoveY = currentMousePos.y - g_prevMouseClientPos.y;
            g_prevMouseClientPos = currentMousePos;
        }
        else
        {
            POINT centerMousePos = GetClientCenter(hWnd);
            mouseMoveX = currentMousePos.x - centerMousePos.x;
            mouseMoveY = currentMousePos.y - centerMousePos.y;
        }

        if (mouseMoveX != 0 || mouseMoveY != 0)
        {
            g_Render.RotateCamera(D3DXVECTOR3(mouseMoveY * MOUSE_CAMERA_SENSITIVITY,
                                              mouseMoveX * MOUSE_CAMERA_SENSITIVITY,
                                              0.0f));

            if (!g_bRemoteDesktop)
            {
                RecenterMouseCursor(hWnd);
            }
        }

        return 0;
    }
    case WM_INPUT:
    {
        if (!g_bMouseLookEnabled || g_bRemoteDesktop)
        {
            return 0;
        }

        LONG mouseMoveX = 0;
        LONG mouseMoveY = 0;
        if (!TryGetRawMouseDelta(lParam, mouseMoveX, mouseMoveY))
        {
            return 0;
        }

        if (mouseMoveX != 0 || mouseMoveY != 0)
        {
            g_Render.RotateCamera(D3DXVECTOR3(static_cast<float>(mouseMoveY) * MOUSE_CAMERA_SENSITIVITY,
                                              static_cast<float>(mouseMoveX) * MOUSE_CAMERA_SENSITIVITY,
                                              0.0f));
        }

        return 0;
    }
    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
            if (g_bMouseLookEnabled)
            {
                cursor = NULL;
            }
            SetCursor(cursor);
            return TRUE;
        }

        break;
    }
    case WM_MOUSEWHEEL:
    {
        MoveCameraAwayFromLookAtByWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    case WM_KEYUP:
    {
        if (HandleSampleKeyUp(wParam))
        {
            return 0;
        }
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (HandleSampleKeyDown(hWnd, wParam))
        {
            return 0;
        }
        return 0;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
