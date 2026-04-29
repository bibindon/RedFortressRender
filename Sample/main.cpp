#include "../Render/Render.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>
#include <commdlg.h>
#include <windowsx.h>

#include "resource.h"

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

const int WINDOW_SIZE_W = 1600;
const int WINDOW_SIZE_H = 900;
const float MOUSE_CAMERA_SENSITIVITY = 0.005f;
const float MODEL_SPAWN_FORWARD_OFFSET = 6.0f;
const float MOUSE_WHEEL_CAMERA_DISTANCE = 1.0f;

bool g_bClose = false;
NSRender::Render g_Render;
int g_fontId = 0;
bool g_bRecenteringMouse = false;
bool g_bMouseLookEnabled = false;
bool g_bMoveForward = false;
bool g_bMoveBackward = false;
bool g_bMoveLeft = false;
bool g_bMoveRight = false;
bool g_bMoveUp = false;
bool g_bMoveDown = false;
float g_saturateLevel = 1.0f;
HWND g_hSettingsDialog = NULL;
std::wstring g_selectedMixMeshPath;
std::wstring g_selectedMeshPath;
std::wstring g_selectedAnimMeshPath;
std::wstring g_selectedSkinAnimMeshPath;
bool g_bAnimateLight = false;

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
INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void ApplySaturateLevel();
void RefreshSettingsDialog(HWND hDlg);
void RefreshSelectedMeshPaths(HWND hDlg);
void RefreshAnimateLight(HWND hDlg);
void SpawnMeshAtCameraFront(const std::wstring& filePath);
void SpawnMeshMixAtCameraFront(const std::wstring& filePath);
void SpawnAnimMeshAtCameraFront(const std::wstring& filePath);
void SpawnSkinAnimMeshAtCameraFront(const std::wstring& filePath);
NSRender::AnimSetMap CreateDefaultAnimSetMap();
bool ShowOpenFileDialog(HWND hWnd, const wchar_t* filter, std::wstring& selectedPath);

void UpdateCameraMoveByKeyboard()
{
    if (!g_bMoveForward && !g_bMoveBackward && !g_bMoveLeft && !g_bMoveRight && !g_bMoveUp && !g_bMoveDown)
    {
        return;
    }

    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    D3DXVECTOR3 worldUp(0, 1, 0);
    D3DXVECTOR3 right;
    D3DXVec3Cross(&right, &worldUp, &forward);
    D3DXVec3Normalize(&right, &right);

    D3DXVECTOR3 move(0.0f, 0.0f, 0.0f);

    if (g_bMoveForward)
    {
        move += forward;
    }

    if (g_bMoveBackward)
    {
        move -= forward;
    }

    if (g_bMoveRight)
    {
        move += right;
    }

    if (g_bMoveLeft)
    {
        move -= right;
    }

    if (g_bMoveUp)
    {
        move += worldUp;
    }

    if (g_bMoveDown)
    {
        move -= worldUp;
    }

    if (D3DXVec3LengthSq(&move) <= 0.0f)
    {
        return;
    }

    D3DXVec3Normalize(&move, &move);

    const float speed = 0.2f;
    g_Render.MoveCamera(move * speed);
}

void MoveCameraAwayFromLookAtByWheel(const short wheelDelta)
{
    if (wheelDelta == 0)
    {
        return;
    }

    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float notchCount = static_cast<float>(-wheelDelta) / WHEEL_DELTA;
    const D3DXVECTOR3 lookAt = g_Render.GetLookAtPos();
    const D3DXVECTOR3 eye = g_Render.GetCameraPos();
    const D3DXVECTOR3 newEye = eye - forward * MOUSE_WHEEL_CAMERA_DISTANCE * notchCount;

    g_Render.SetCamera(newEye, lookAt);
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

POINT GetClientCenter(HWND hWnd)
{
    RECT clientRect { };
    GetClientRect(hWnd, &clientRect);

    POINT center { };
    center.x = (clientRect.right - clientRect.left) / 2;
    center.y = (clientRect.bottom - clientRect.top) / 2;
    return center;
}

void RecenterMouseCursor(HWND hWnd)
{
    POINT center = GetClientCenter(hWnd);
    ClientToScreen(hWnd, &center);

    if (SetCursorPos(center.x, center.y))
    {
        g_bRecenteringMouse = true;
    }
}

void HideMouseCursor()
{
    while (ShowCursor(FALSE) >= 0)
    {
    }
}

void ShowMouseCursor()
{
    while (ShowCursor(TRUE) < 0)
    {
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void EnableMouseLook(HWND hWnd)
{
    if (g_bMouseLookEnabled)
    {
        return;
    }

    g_bMouseLookEnabled = true;
    HideMouseCursor();
    SetCursor(NULL);
    RecenterMouseCursor(hWnd);
}

void DisableMouseLook()
{
    if (!g_bMouseLookEnabled)
    {
        return;
    }

    g_bMouseLookEnabled = false;
    g_bRecenteringMouse = false;
    ShowMouseCursor();
}

void ShowSettingsDialog(HWND hWnd, const bool activateDialog = true)
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

void ApplySaturateLevel()
{
    if (g_saturateLevel < 0.0f)
    {
        g_saturateLevel = 0.0f;
    }

    g_Render.SetPostEffectSaturate(g_saturateLevel);
}

void RefreshSettingsDialog(HWND hDlg)
{
    wchar_t buffer[32];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.1f", g_saturateLevel);
    SetDlgItemText(hDlg, IDC_EDIT_SATURATE_LEVEL, buffer);
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

void SpawnMeshAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, 1.0f);
}

void SpawnMeshMixAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddMeshMix(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, 1.0f);
}

NSRender::AnimSetMap CreateDefaultAnimSetMap()
{
    NSRender::AnimSetMap animMap;
    NSRender::AnimSetting animSetting;
    animSetting.m_startPos = 0.f;
    animSetting.m_duration = 1.f;
    animSetting.m_loop = true;
    animSetting.m_stopEnd = false;
    animMap[L"0_Idle"] = animSetting;
    return animMap;
}

void SpawnAnimMeshAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddAnimMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, CreateDefaultAnimSetMap());
}

void SpawnSkinAnimMeshAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddSkinAnimMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, CreateDefaultAnimSetMap());
}

bool ShowOpenFileDialog(HWND hWnd, const wchar_t* filter, std::wstring& selectedPath)
{
    wchar_t filePath[MAX_PATH] { };

    OPENFILENAMEW ofn { };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = static_cast<DWORD>(_countof(filePath));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"x";

    if (!GetOpenFileNameW(&ofn))
    {
        return false;
    }

    selectedPath = filePath;
    return true;
}

INT_PTR CALLBACK SettingsDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        MoveDialogToRightOfParent(hDlg);
        RefreshSettingsDialog(hDlg);
        RefreshSelectedMeshPaths(hDlg);
        RefreshAnimateLight(hDlg);
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
    case WM_COMMAND:
    {
        const WORD commandId = LOWORD(wParam);

        if (commandId == IDC_BUTTON_SATURATE_DOWN)
        {
            g_saturateLevel -= 0.1f;
            ApplySaturateLevel();
            RefreshSettingsDialog(hDlg);
            return TRUE;
        }

        if (commandId == IDC_BUTTON_SATURATE_UP)
        {
            g_saturateLevel += 0.1f;
            ApplySaturateLevel();
            RefreshSettingsDialog(hDlg);
            return TRUE;
        }

        if (commandId == IDC_BUTTON_SATURATE_RESET)
        {
            g_saturateLevel = 1.0f;
            ApplySaturateLevel();
            RefreshSettingsDialog(hDlg);
            return TRUE;
        }

        if (commandId == IDC_BUTTON_OPEN_MIX_MESH)
        {
            if (ShowOpenFileDialog(hDlg,
                                   L"Mix Mesh Files (*.x;*.blend.x)\0*.x;*.blend.x\0All Files (*.*)\0*.*\0",
                                   g_selectedMixMeshPath))
            {
                SpawnMeshMixAtCameraFront(g_selectedMixMeshPath);
                RefreshSelectedMeshPaths(hDlg);
            }
            return TRUE;
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
            return TRUE;
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
            return TRUE;
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
            return TRUE;
        }

        if (commandId == IDC_CHECK_ANIMATE_LIGHT)
        {
            g_bAnimateLight = (IsDlgButtonChecked(hDlg, IDC_CHECK_ANIMATE_LIGHT) == BST_CHECKED);
            RefreshAnimateLight(hDlg);
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
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
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

    g_Render.Initialize(hWnd, L"RenderSettings.csv");
    g_Render.SetCamera(D3DXVECTOR3(0.0f, 2.0f, -6.0f), D3DXVECTOR3(0.0f, 1.5f, 0.0f));
    g_fontId = g_Render.SetUpFont(L"BIZ UDゴシック", 20, D3DCOLOR_RGBA(255, 255, 255, 255));
    g_Render.AddMeshNoLighting(L"cubeNormalInverse.x",
                               D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                               D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                               1.0f,
                               100.0f);
    g_Render.AddMesh(L"plateField.x",
                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                     1.0f,
                     100.0f);

    // 光源の方角がわかりやすくなるように、光源の方角に球を表示
//    g_sunId = g_Render.AddMeshMix(L"cubeMixSun.blend.x", D3DXVECTOR3(10, 0, 0), D3DXVECTOR3(0, 0, 0), 1.f, 1.f);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);
    ShowSettingsDialog(hWnd, false);
    ShowMouseCursor();

    MSG msg;

    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (g_hSettingsDialog != NULL && IsDialogMessage(g_hSettingsDialog, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
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
                text += L"Q/E : カメラ上下移動\n";
                text += L"矢印キー : カメラ回転\n";
                text += L"Esc : マウスカメラ操作開始\n";
                text += L"Ctrl : マウスカメラ操作終了\n";
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
                D3DXVECTOR3 lightDir(0.0f, 0.0f, 0.0f);

                if (g_bAnimateLight)
                {
                    work_f += 0.02f;
                    lightDir.x = sinf(work_f);
                    lightDir.z = cosf(work_f);
                    lightDir.y = sinf(work_f * 2);
                }
                else
                {
                    lightDir = D3DXVECTOR3(1.0f, 1.0f, -1.0f);
                }

                g_Render.SetLightDir(lightDir);

                // 動作確認のため、光源の方角に球を表示する
                lightDir *= 4;
//                g_Render.SetMeshMixPos(g_sunId, lightDir);
            }

            UpdateCameraMoveByKeyboard();

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

        if (g_bRecenteringMouse)
        {
            g_bRecenteringMouse = false;
            return 0;
        }

        POINT currentMousePos { };
        currentMousePos.x = GET_X_LPARAM(lParam);
        currentMousePos.y = GET_Y_LPARAM(lParam);

        POINT centerMousePos = GetClientCenter(hWnd);

        const int mouseMoveX = currentMousePos.x - centerMousePos.x;
        const int mouseMoveY = currentMousePos.y - centerMousePos.y;

        if (mouseMoveX != 0 || mouseMoveY != 0)
        {
            g_Render.RotateCamera(D3DXVECTOR3(mouseMoveY * MOUSE_CAMERA_SENSITIVITY,
                                              mouseMoveX * MOUSE_CAMERA_SENSITIVITY,
                                              0.0f));
            RecenterMouseCursor(hWnd);
        }

        return 0;
    }
    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(g_bMouseLookEnabled ? NULL : LoadCursor(NULL, IDC_ARROW));
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
        if (wParam == 'W')
        {
            g_bMoveForward = false;
        }
        else if (wParam == 'S')
        {
            g_bMoveBackward = false;
        }
        else if (wParam == 'A')
        {
            g_bMoveLeft = false;
        }
        else if (wParam == 'D')
        {
            g_bMoveRight = false;
        }
        else if (wParam == 'E')
        {
            g_bMoveUp = false;
        }
        else if (wParam == 'Q')
        {
            g_bMoveDown = false;
        }

        return 0;
    }
    case WM_KEYDOWN:
    {
        bool shift = false;
        bool control = false;

        if (wParam == VK_F1)
        {
            ShowSettingsDialog(hWnd);
            return 0;
        }

        if (wParam == VK_ESCAPE)
        {
            EnableMouseLook(hWnd);
            return 0;
        }

        if (wParam == VK_CONTROL)
        {
            DisableMouseLook();
            return 0;
        }

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
            SpawnMeshMixAtCameraFront(L"monkeySSAO.blend.x");
        }

        if (wParam == 'M' && shift && control)
        {
            auto pos = g_Render.GetLookAtPos();
            D3DXVECTOR3 forward = g_Render.GetCameraRotate();
            D3DXVec3Normalize(&forward, &forward);
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

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
            if (wParam == 'W')
            {
                g_bMoveForward = true;
            }
            else if (wParam == 'S')
            {
                g_bMoveBackward = true;
            }
            else if (wParam == 'D')
            {
                g_bMoveRight = true;
            }
            else if (wParam == 'A')
            {
                g_bMoveLeft = true;
            }
            else if (wParam == 'E')
            {
                g_bMoveUp = true;
            }
            else if (wParam == 'Q')
            {
                g_bMoveDown = true;
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
                g_Render.AddPointLight(pos, 1.f, D3DXCOLOR(1.0f, 0.35f, 0.1f, 1.0f));
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
            if (wParam == 'S' && shift)
            {
                g_saturateLevel += 0.1f;
                ApplySaturateLevel();
            }

            if (wParam == 'S' && control)
            {
                g_saturateLevel -= 0.1f;
                ApplySaturateLevel();
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

