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
int g_fontId = 0;

struct ImageInfo
{
    std::wstring m_imageName;
    RECT m_rect { };
};

std::vector<ImageInfo> g_imageInfoList;

struct TextInfo
{
    std::wstring m_text;
    RECT m_rect { };
};
std::vector<TextInfo> g_textInfoList;

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
    g_fontId = g_Render.SetUpFont(L"BIZ UDゴシック", 20, D3DCOLOR_RGBA(255, 255, 255, 255));

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

            {
                std::wstring text;
                text += L"WASD : カメラ移動\n";
                text += L"矢印キー : カメラ回転\n";
                text += L"\n";
                text += L"8 : ウィンドウモード\n";
                text += L"9 : ボーダーレスウィンドウモード\n";
                text += L"0 : フルスクリーンモード\n";
                text += L"\n";
                text += L"c : 文字を追加\n";
                text += L"Shift + c : 文字を削除\n";
                text += L"p : 画像を追加\n";
                text += L"Shift + p : 画像を削除\n";
                text += L"\n";
                text += L"m : メッシュ追加\n";
                text += L"Shift + m : スムーズなメッシュ追加\n";
                text += L"Ctrl + m : SSS風メッシュ追加\n";
                text += L"\n";
                text += L"n : アニメーションメッシュ追加\n";
                text += L"k : スキンアニメーションメッシュ追加\n";
                text += L"\n";
                text += L"i : インスタンシングメッシュ追加\n";
                text += L"\n";
                text += L"o : ポイントライトが反映されるメッシュ追加\n";
                text += L"\n";
                text += L"Shift + s : 彩度を上げる\n";
                text += L"Control + s : 彩度を下げる\n";
                text += L"\n";
                text += L"g : ガウスフィルターON/OFF\n";
                g_Render.DrawText_(g_fontId, text, 10, 10);
            }

            for (auto& elem : g_textInfoList)
            {
                g_Render.DrawText_(g_fontId,
                                   elem.m_text,
                                   elem.m_rect.left,
                                   elem.m_rect.top,
                                   0xFFAA88FF);
            }

            for (auto& elem : g_imageInfoList)
            {
                g_Render.DrawImage(elem.m_imageName, elem.m_rect.left, elem.m_rect.top);
            }

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
        bool shift = false;
        bool control = false;

        if ((GetKeyState(VK_SHIFT) & 0x8000) != 0)
        {
            shift = true;
        }

        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0)
        {
            control = true;
        }

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

        if (wParam == 'M' && !shift && !control)
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            g_Render.AddMesh(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
        }

        if (wParam == 'M' && shift && !control)
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            g_Render.AddMeshSmooth(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
        }

        if (wParam == 'M' && !shift && control)
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            g_Render.AddMeshSSSLike(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
        }

        if (wParam == 'N')
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            NSRender::AnimSetMap animMap;
            NSRender::AnimSetting animSetting;
            animSetting.m_startPos = 0.f;
            animSetting.m_duration = 1.f;
            animSetting.m_loop = true;
            animSetting.m_stopEnd = false;

            animMap[L"0_Idle"] = animSetting;

            g_Render.AddAnimMesh(L"enemyOrangeCube.x",
                                 pos,
                                 D3DXVECTOR3(0, yaw, 0.0f),
                                 1.f,
                                 animMap);
        }

        if (wParam == 'K')
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            NSRender::AnimSetMap animMap;
            NSRender::AnimSetting animSetting;
            animSetting.m_startPos = 0.f;
            animSetting.m_duration = 1.f;
            animSetting.m_loop = true;
            animSetting.m_stopEnd = false;

            animMap[L"0_Idle"] = animSetting;

            g_Render.AddSkinAnimMesh(L"res\\model\\wolf.x",
                                     pos,
                                     D3DXVECTOR3(0, yaw, 0.0f),
                                     3.f,
                                     animMap);
        }

        if (wParam == 'I')
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            //g_Render.AddMesh(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
            g_Render.AddMeshInstansing(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f);
        }

        if (wParam == 'O')
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            //g_Render.AddMesh(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
            g_Render.AddMeshPointLight(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f);
        }

        if (!shift && !control)
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
            g_Render.RotateCamera(D3DXVECTOR3(-0.2f, 0, 0));
        }
        else if (wParam == VK_DOWN)
        {
            g_Render.RotateCamera(D3DXVECTOR3(0.2f, 0, 0));
        }
        else if (wParam == VK_LEFT)
        {
            g_Render.RotateCamera(D3DXVECTOR3(0, -0.2f, 0));
        }
        else if (wParam == VK_RIGHT)
        {
            g_Render.RotateCamera(D3DXVECTOR3(0, 0.2f, 0));
        }

        if (wParam == 'P')
        {
            if (shift)
            {
                g_imageInfoList.clear();
            }
            else
            {
                ImageInfo imageInfo;
                imageInfo.m_imageName = L"cursor.png";

                int randX = std::abs(rand());
                randX %= 1300;
                int randY = std::abs(rand());
                randY %= 700;

                imageInfo.m_rect.left = randX;
                imageInfo.m_rect.top = randY;

                g_imageInfoList.push_back(imageInfo);
            }
        }

        if (wParam == 'C')
        {
            if (shift)
            {
                g_textInfoList.clear();
            }
            else
            {
                TextInfo textInfo;
                textInfo.m_text = L"サンプルテキスト";

                int randX = std::abs(rand());
                randX %= 1300;
                int randY = std::abs(rand());
                randY %= 700;

                textInfo.m_rect.left = randX;
                textInfo.m_rect.top = randY;

                g_textInfoList.push_back(textInfo);
            }
        }

        // 彩度
        {
            static float saturateLevel = 1.0f;

            if (wParam == 'S' && shift)
            {
                saturateLevel += 0.1f;
                g_Render.SetPostEffectSaturate(saturateLevel);
            }

            if (wParam == 'S' && control)
            {
                saturateLevel -= 0.1f;

                if (saturateLevel < 0.0f)
                {
                    saturateLevel = 0.0f;
                }

                g_Render.SetPostEffectSaturate(saturateLevel);
            }
        }

        // ガウスフィルター
        {
            static bool bGauss = false;
            if (wParam == 'G')
            {
                bGauss = !bGauss;
                g_Render.SetPostEffectGaussianFilter(bGauss);
            }
        }
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

