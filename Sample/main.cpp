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

int g_sunId = 0;

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

    // 光源の方角がわかりやすくなるように、光源の方角に球を表示
    g_sunId = g_Render.AddMeshMix(L"cubeMixSun.blend.x", D3DXVECTOR3(10, 0, 0), D3DXVECTOR3(0, 0, 0), 1.f, 1.f);

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
            // Sleep入れなくてよい。
            // ここでSleepをいれているのは速すぎて疲れるから。
            //Sleep(16);

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
                text += L"Control + p : ポイントライトを追加\n";
                text += L"\n";
                text += L"m : メッシュ追加\n";
                text += L"Shift + m : スムーズなメッシュ追加\n";
                text += L"Ctrl + m : SSSメッシュ追加\n";
                text += L"\n";
                text += L"n : アニメーションメッシュ追加\n";
                text += L"k : スキンアニメーションメッシュ追加\n";
                text += L"\n";
                text += L"i : インスタンシングメッシュ追加\n";
                text += L"\n";
                text += L"o : ポイントライトが反映されるメッシュ追加\n";
                text += L"\n";
                text += L"Shift + n : 法線マッピング対応のメッシュ追加\n";
                text += L"\n";
                text += L"Shift + s : 彩度を上げる\n";
                text += L"Control + s : 彩度を下げる\n";
                text += L"\n";
                text += L"g : ガウスフィルターON/OFF\n";
                text += L"\n";
                text += L"b : ブルームON/OFF\n";
                text += L"Shift + b : スターバーストON/OFF\n";
                text += L"\n";
                text += L"Shift + f : FPS表示ON/OFF\n";
                g_Render.DrawText_(g_fontId, text, 10, 40);
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

            // 平行光源の方角を変える
            {
                static float work_f = 0.0f;
                work_f += 0.02f;

                D3DXVECTOR3 lightDir(0.0f, 0.0f, 0.0f);

                lightDir.x = sinf(work_f);
                lightDir.z = cosf(work_f);
                lightDir.y = sinf(work_f * 2);

                g_Render.SetLightDir(lightDir);

                // 動作確認のため、光源の方角に球を表示する
                lightDir *= 4;
                g_Render.SetMeshMixPos(g_sunId, lightDir);
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

            //g_Render.AddMeshMix(L"cubeMix.blend.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
            g_Render.AddMeshMix(L"monkeySSAO.blend.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
        }

        if (wParam == 'M' && shift && control)
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

//            g_Render.AddMeshSmooth(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
            g_Render.AddMeshPOM(L"cubePOM.blend.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
        }

        if (wParam == 'M' && !shift && control)
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            // AddMeshの第3引数が「回転角 (ラジアン)」だと仮定
            g_Render.AddMeshSSS(L"monkey.blend.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.f, 1.f);
        }

        if (wParam == 'N' && shift && !control)
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);

            // Yaw, Pitch を計算
            float yaw = atan2f(forward.x, forward.z);

            g_Render.AddMeshNormalMapping(L"cubeNormalMap.x",
                                          L"normalMap.png",
                                          pos,
                                          D3DXVECTOR3(0, yaw, 0.0f),
                                          1.f);
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

        if (wParam == 'O' && !shift && control)
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

        if (wParam == 'N' && !shift && !control)
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
            if (control)
            {
                auto pos = g_Render.GetLookAtPos();
                g_Render.AddPointLight(pos, 1.f, D3DXCOLOR(1.0f, 0.85f, 0.4f, 1.0f));
            }
            else
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

        // ブルーム
        {
            static bool bBloom = true;
            if (wParam == 'B' && !shift)
            {
                bBloom = !bBloom;
                g_Render.SetPostEffectBloom(bBloom);
            }
        }

        // スターバースト
        {
            static bool bStarBurst = false;
            if (wParam == 'B' && shift)
            {
                bStarBurst = !bStarBurst;
                g_Render.SetPostEffectStarBurst(bStarBurst);
            }
        }

        // FPS表示
        {
            static bool bShowFPS = true;
            if (wParam == 'F')
            {
                bShowFPS = !bShowFPS;
                g_Render.SetShowFPS(bShowFPS);
            }
        }

        // 解像度を取得
        {
            if (wParam == 'R')
            {
                auto resoList = g_Render.GetResolutionList();
                std::wstring work;
                for (auto& reso : resoList)
                {
                    work += L" Resolution : ";
                    work += std::to_wstring(reso.first);
                    work += L" x ";
                    work += std::to_wstring(reso.second);
                    work += L"\n";
                    OutputDebugString(work.c_str());
                }
            }
        }

    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

