#include "../Render/Render.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

const int WINDOW_SIZE_W = 1600;
const int WINDOW_SIZE_H = 900;

bool g_bClose = false;
NSRender::Render g_Render;

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern int WINAPI _tWinMain(_In_ HINSTANCE hInstance,
                            _In_opt_ HINSTANCE hPrevInstance,
                            _In_ LPTSTR lpCmdLine,
                            _In_ int nCmdShow);

int WINAPI _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    WNDCLASSEX wc { };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = MsgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("Window1");
    wc.hIconSm = NULL;

    ATOM atom = RegisterClassEx(&wc);
    assert(atom != 0);

    RECT rect;
    SetRect(&rect, 0, 0, WINDOW_SIZE_W, WINDOW_SIZE_H);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.top = 0;
    rect.left = 0;

    HWND hWnd = CreateWindow(_T("Window1"),
                             _T("Hello DirectX9 World !!"),
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             rect.right,
                             rect.bottom,
                             NULL,
                             NULL,
                             wc.hInstance,
                             NULL);

    g_Render.Initialize(hWnd);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    MSG msg;

    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&msg);
        }
        else
        {
            Sleep(16);
            g_Render.Draw();
        }

        if (g_bClose)
        {
            break;
        }
    }

    g_Render.Finalize();

    UnregisterClass(_T("Window1"), wc.hInstance);
    return 0;
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        g_bClose = true;
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (wParam == '8')
        {
            g_Render.ChangeWindowMode(NSRender::eWindowMode::WINDOW);
        }
        else if (wParam == '9')
        {
            g_Render.ChangeWindowMode(NSRender::eWindowMode::BORDERLESS);
        }
        else if (wParam == '0')
        {
            g_Render.ChangeWindowMode(NSRender::eWindowMode::FULLSCREEN);
        }

        if (wParam == 'M')
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            g_Render.AddMesh(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
        }

        {
            // 現在向いている前方向ベクトル
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward); // 念のため正規化

            // 右方向は 前方向×(世界上方向)
            D3DXVECTOR3 worldUp(0, 1, 0);
            D3DXVECTOR3 right;
            D3DXVec3Cross(&right, &worldUp, &forward);
            D3DXVec3Normalize(&right, &right);

            const float speed = 0.2f;

            if (wParam == 'W')
            {
                g_Render.MoveCamera(forward * speed);
            }
            else if (wParam == 'S')
            {
                g_Render.MoveCamera(forward * -speed);
            }
            else if (wParam == 'D')
            {
                g_Render.MoveCamera(right * speed);
            }
            else if (wParam == 'A')
            {
                g_Render.MoveCamera(right * -speed);
            }
        }

        if (wParam == VK_UP)
        {
            g_Render.RotateCamera(D3DXVECTOR3(0.2f, 0, 0));
        }
        else if (wParam == VK_DOWN)
        {
            g_Render.RotateCamera(D3DXVECTOR3(-0.2f, 0, 0));
        }
        else if (wParam == VK_LEFT)
        {
            g_Render.RotateCamera(D3DXVECTOR3(0, 0.2f, 0));
        }
        else if (wParam == VK_RIGHT)
        {
            g_Render.RotateCamera(D3DXVECTOR3(0, -0.2f, 0));
        }
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

