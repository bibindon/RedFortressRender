#pragma comment( lib, "d3d9.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif

#include "Render.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>

#include "Common.h"

#include "Mesh.h"
#include "AnimMesh.h"
#include "SkinAnimMesh.h"

#include "Camera.h"
#include "Light.h"

#include "Font.h"

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

void NSRender::Render::Initialize(HWND hWnd)
{
    HRESULT hResult = E_FAIL;

    m_hWnd = hWnd;
    m_eWindowModeCurrent = eWindowMode::WINDOW;

    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    assert(m_pD3D != NULL);

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount = 1;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality = 0;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.hDeviceWindow = m_hWnd;
    d3dpp.Flags = 0;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    LPDIRECT3DDEVICE9 D3DDevice = NULL;

    hResult = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                   D3DDEVTYPE_HAL,
                                   m_hWnd,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                   &d3dpp,
                                   &D3DDevice);

    if (FAILED(hResult))
    {
        hResult = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL,
                                       m_hWnd,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                       &d3dpp,
                                       &D3DDevice);

        assert(hResult == S_OK);
    }

    Common::SetD3DDevice(D3DDevice);

    hResult = D3DXCreateFont(Common::D3DDevice(),
                             20,
                             0,
                             FW_HEAVY,
                             1,
                             FALSE,
                             SHIFTJIS_CHARSET,
                             OUT_TT_ONLY_PRECIS,
                             CLEARTYPE_NATURAL_QUALITY,
                             FF_DONTCARE,
                             _T("ＭＳ ゴシック"),
                             &m_pFont);

    assert(hResult == S_OK);

    // AddMesh(L"cube.x", D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0), 1.f, 1.f);
}

void NSRender::Render::Finalize()
{
    SAFE_RELEASE(m_pFont);

    Common::D3DDevice()->Release();
    Common::SetD3DDevice(NULL);

    SAFE_RELEASE(m_pD3D);
}

void NSRender::Render::Draw()
{
    HRESULT hResult = E_FAIL;

    hResult = Common::D3DDevice()->Clear(0,
                                         NULL,
                                         D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                         D3DCOLOR_XRGB(100, 100, 100),
                                         1.0f,
                                         0);

    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->BeginScene();
    assert(hResult == S_OK);

    for (auto& elem : m_meshList)
    {
        elem.Render();
    }

    for (auto& elem : m_animMeshList)
    {
        elem->Render();
    }

    for (auto& elem : m_skinAnimMeshList)
    {
        elem->Render(Camera::GetViewMatrix(),
                     Camera::GetProjMatrix(),
                     Light::GetLightNormal(),
                     Light::GetBrightness());
    }

    for (auto& elem : m_fontList)
    {
        elem.Draw();
    }

    hResult = Common::D3DDevice()->EndScene();
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->Present(NULL, NULL, NULL, NULL);
    assert(hResult == S_OK);

    if (m_eWindowModeRequest != eWindowMode::NONE)
    {
        ChangeWindowMode();
    }

}

void NSRender::Render::ChangeResolution(const int W, const int H)
{
}

void NSRender::Render::ChangeWindowMode(const eWindowMode eWindowMode_)
{
    if (m_eWindowModeRequest != eWindowMode_)
    {
        m_eWindowModeRequest = eWindowMode_;
    }
}

void NSRender::Render::AddMesh(const std::wstring& filePath,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rot,
                               const float scale,
                               const float radius)
{
    m_meshList.push_back(Mesh(filePath, pos, rot, scale, radius));
    m_meshList.rbegin()->Init();
}

void NSRender::Render::AddAnimMesh(const std::wstring& filePath,
                                   const D3DXVECTOR3& pos,
                                   const D3DXVECTOR3& rot,
                                   const float scale,
                                   const AnimSetMap& animSetMap)
{
    AnimMesh* animMesh = NEW AnimMesh(filePath, pos, rot, scale, animSetMap);
    m_animMeshList.push_back(animMesh);
}

void NSRender::Render::AddSkinAnimMesh(const std::wstring& filePath,
                                       const D3DXVECTOR3& pos,
                                       const D3DXVECTOR3& rot,
                                       const float scale,
                                       const AnimSetMap& animSetMap)
{
    SkinAnimMesh* mesh = NEW SkinAnimMesh(filePath, pos, rot, scale, animSetMap);
    m_skinAnimMeshList.push_back(mesh);
}

void NSRender::Render::SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt)
{
    Camera::SetEyePos(pos);
    Camera::SetLookAtPos(lookAt);
}

void NSRender::Render::MoveCamera(const D3DXVECTOR3& pos)
{
    auto eyePos = Camera::GetEyePos();
    Camera::SetEyePos(eyePos + pos);

    auto lookAtPos = Camera::GetLookAtPos();
    Camera::SetLookAtPos(lookAtPos + pos);
}

D3DXVECTOR3 NSRender::Render::GetLookAtPos()
{
    return Camera::GetLookAtPos();
}

D3DXVECTOR3 NSRender::Render::GetCameraRotate()
{
    auto eyePos = Camera::GetEyePos();
    auto lookAtPos = Camera::GetLookAtPos();
    auto dir(lookAtPos - eyePos);
    D3DXVec3Normalize(&dir, &dir);
    return dir;
}

int NSRender::Render::SetUpFont(const std::wstring& fontName,
                                const int fontSize,
                                const UINT fontColor)
{
    Font font;
    font.Initialize(fontName, fontSize, fontColor);
    m_fontList.push_back(font);

    return (int)(m_fontList.size() - 1);
}

void NSRender::Render::AddTextLeft(const int fontId,
                                   const std::wstring& text,
                                   const int X,
                                   const int Y)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId).AddTextLeft(text, X, Y);
}

void NSRender::Render::AddTextCenter(const int fontId,
                                     const std::wstring& text,
                                     const int X,
                                     const int Y,
                                     const int Width,
                                     const int Height)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId).AddTextCenter(text, X, Y, Width, Height);
}

void NSRender::Render::RotateCamera(const D3DXVECTOR3& rot)
{
    D3DXVECTOR3 lookAt = Camera::GetLookAtPos();
    D3DXVECTOR3 eye = Camera::GetEyePos();

    // 注視点から見た相対位置
    D3DXVECTOR3 rel = eye - lookAt;

    // 現在の距離
    float r = D3DXVec3Length(&rel);

    // 現在の角度を求める（spherical座標）
    float yaw = atan2f(rel.x, rel.z);             // 水平方向
    float pitch = asinf(rel.y / r);                 // 上下方向

    // 回転を加える
    yaw += rot.y;
    pitch += rot.x;

    // --- ピッチ角を制限する ---
    const float limit = D3DXToRadian(89.0f);        // 真上/真下を少し手前で止める
    if (pitch > limit) pitch = limit;
    if (pitch < -limit) pitch = -limit;

    // 極座標 → デカルト座標に戻す
    D3DXVECTOR3 newRel;
    newRel.x = r * cosf(pitch) * sinf(yaw);
    newRel.y = r * sinf(pitch);
    newRel.z = r * cosf(pitch) * cosf(yaw);

    // 新しいeye位置をセット
    D3DXVECTOR3 newEye = lookAt + newRel;

    Camera::SetEyePos(newEye);
    Camera::SetLookAtPos(lookAt);
}

void NSRender::Render::TextDraw(const std::wstring& text, int X, int Y)
{
    RECT rect = { X, Y, 0, 0 };

    // DrawTextの戻り値は文字数である。
    // そのため、hResultの中身が整数でもエラーが起きているわけではない。
    HRESULT hResult = m_pFont->DrawText(NULL,
                                      text.c_str(),
                                      -1,
                                      &rect,
                                      DT_LEFT | DT_NOCLIP,
                                      D3DCOLOR_ARGB(255, 0, 0, 0));

    assert((int)hResult >= 0);
}

// TODO いずれちゃんと書くこと
void NSRender::Render::ChangeWindowMode()
{
    HRESULT hResult = E_FAIL;

    m_pFont->OnLostDevice();

    for (auto& elem : m_meshList)
    {
        elem.OnDeviceLost();
    }

    for (auto& elem : m_animMeshList)
    {
        elem->OnDeviceLost();
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    if (m_eWindowModeRequest == eWindowMode::FULLSCREEN)
    {
        d3dpp.Windowed = FALSE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
        d3dpp.BackBufferCount = 1;
        d3dpp.BackBufferWidth = 1600;
        d3dpp.BackBufferHeight = 900;
        d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;

        // TODO 要確認
        d3dpp.MultiSampleQuality = 0;

        d3dpp.EnableAutoDepthStencil = TRUE;
        d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
        d3dpp.hDeviceWindow = m_hWnd;
        d3dpp.Flags = 0;
        d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    }
    else if (m_eWindowModeRequest == eWindowMode::WINDOW)
    {
        // 目的モニタを決める
        HMONITOR mon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi { sizeof(mi) };
        GetMonitorInfo(mon, &mi);

        // 物理座標（タスクバー含む全面）
        RECT r = mi.rcMonitor;

        const int x_ = (r.right / 2) - (1600 / 2);
        const int y_ = (r.bottom / 2) - (900 / 2);

        SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(m_hWnd,
                     HWND_TOP,
                     x_,
                     y_,
                     1650,
                     910,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        // ウィンドウサイズの変更をさせない。最小化はOK
        SetWindowLongPtr(m_hWnd,
                         GWL_STYLE,
                         WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME) | WS_VISIBLE);

        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
        d3dpp.BackBufferCount = 1;
        d3dpp.BackBufferWidth = 1600;
        d3dpp.BackBufferHeight = 900;
        d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;

        // TODO 要確認
        d3dpp.MultiSampleQuality = 0;

        d3dpp.EnableAutoDepthStencil = TRUE;
        d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
        d3dpp.hDeviceWindow = m_hWnd;
        d3dpp.Flags = 0;
        d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    }
    else if (m_eWindowModeRequest == eWindowMode::BORDERLESS)
    {
        // 目的モニタを決める
        HMONITOR mon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi { sizeof(mi) };
        GetMonitorInfo(mon, &mi);

        // 物理座標（タスクバー含む全面）
        RECT r = mi.rcMonitor;

        SetWindowLongPtr(m_hWnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(m_hWnd,
                     HWND_TOP,
                     r.left,
                     r.top,
                     r.right - r.left,
                     r.bottom - r.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
        d3dpp.BackBufferCount = 1;
        d3dpp.BackBufferWidth = 1600;
        d3dpp.BackBufferHeight = 900;
        d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;

        // TODO 要確認
        d3dpp.MultiSampleQuality = 0;

        d3dpp.EnableAutoDepthStencil = TRUE;
        d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
        d3dpp.hDeviceWindow = m_hWnd;
        d3dpp.Flags = 0;
        d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    }

    hResult = Common::D3DDevice()->Reset(&d3dpp);
    assert(hResult == S_OK);

    for (auto& elem : m_meshList)
    {
        elem.OnDeviceReset();
    }

    for (auto& elem : m_animMeshList)
    {
        elem->OnDeviceReset();
    }

    m_pFont->OnResetDevice();

    m_eWindowModeCurrent = m_eWindowModeRequest;
    m_eWindowModeRequest = eWindowMode::NONE;
}

