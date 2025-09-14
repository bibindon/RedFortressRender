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
#include "Camera.h"

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

const int WINDOW_SIZE_W = 1600;
const int WINDOW_SIZE_H = 900;

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

    hResult = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                   D3DDEVTYPE_HAL,
                                   m_hWnd,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                   &d3dpp,
                                   &m_pd3dDevice);

    if (FAILED(hResult))
    {
        hResult = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL,
                                       m_hWnd,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                       &d3dpp,
                                       &m_pd3dDevice);

        assert(hResult == S_OK);
    }

    Common::SetD3DDevice(m_pd3dDevice);

    hResult = D3DXCreateFont(m_pd3dDevice,
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

    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;

    hResult = D3DXLoadMeshFromX(_T("cube.x"),
                                D3DXMESH_SYSTEMMEM,
                                m_pd3dDevice,
                                NULL,
                                &pD3DXMtrlBuffer,
                                NULL,
                                &m_dwNumMaterials,
                                &m_pMesh);

    assert(hResult == S_OK);

    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    m_pMaterials.resize(m_dwNumMaterials);
    m_pTextures.resize(m_dwNumMaterials);

    for (DWORD i = 0; i < m_dwNumMaterials; i++)
    {
        m_pMaterials[i] = d3dxMaterials[i].MatD3D;
        m_pMaterials[i].Ambient = m_pMaterials[i].Diffuse;
        m_pTextures[i] = NULL;
        
        //--------------------------------------------------------------
        // Unicode文字セットでもマルチバイト文字セットでも
        // "d3dxMaterials[i].pTextureFilename"はマルチバイト文字セットになる。
        // 
        // 一方で、D3DXCreateTextureFromFileはプロジェクト設定で
        // Unicode文字セットかマルチバイト文字セットか変わる。
        //--------------------------------------------------------------

        std::string pTexPath(d3dxMaterials[i].pTextureFilename);

        if (!pTexPath.empty())
        {
            bool bUnicode = false;

#ifdef UNICODE
            bUnicode = true;
#endif

            if (!bUnicode)
            {
                hResult = D3DXCreateTextureFromFileA(m_pd3dDevice, pTexPath.c_str(), &m_pTextures[i]);
                assert(hResult == S_OK);
            }
            else
            {
                int len = MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, nullptr, 0);
                std::wstring pTexPathW(len, 0);
                MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, &pTexPathW[0], len);

                hResult = D3DXCreateTextureFromFileW(m_pd3dDevice, pTexPathW.c_str(), &m_pTextures[i]);
                assert(hResult == S_OK);
            }
        }
    }

    hResult = pD3DXMtrlBuffer->Release();
    assert(hResult == S_OK);

    hResult = D3DXCreateEffectFromFile(m_pd3dDevice,
                                       _T("simple.fx"),
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_pEffect,
                                       NULL);

    assert(hResult == S_OK);
}

void NSRender::Render::Finalize()
{
    for (auto& texture : m_pTextures)
    {
        SAFE_RELEASE(texture);
    }

    SAFE_RELEASE(m_pMesh);
    SAFE_RELEASE(m_pEffect);
    SAFE_RELEASE(m_pFont);
    SAFE_RELEASE(m_pd3dDevice);
    SAFE_RELEASE(m_pD3D);
}

void NSRender::Render::Draw()
{
    HRESULT hResult = E_FAIL;

    static float f = 0.0f;
    f += 0.025f;

    D3DXMATRIX mat;
    D3DXMATRIX View, Proj;

    D3DXMatrixPerspectiveFovLH(&Proj,
                               D3DXToRadian(45),
                               (float)WINDOW_SIZE_W / WINDOW_SIZE_H,
                               1.0f,
                               10000.0f);

    D3DXVECTOR3 vec1(10 * sinf(f), 10, -10 * cosf(f));
    Camera::SetEyePos(vec1);
    D3DXVECTOR3 vec2(0, 0, 0);
    D3DXVECTOR3 vec3(0, 1, 0);
    D3DXMatrixLookAtLH(&View, &vec1, &vec2, &vec3);
    D3DXMatrixIdentity(&mat);
    mat = mat * View * Proj;

    hResult = m_pEffect->SetMatrix("g_matWorldViewProj", &mat);
    assert(hResult == S_OK);

    hResult = m_pd3dDevice->Clear(0,
                                  NULL,
                                  D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                  D3DCOLOR_XRGB(100, 100, 100),
                                  1.0f,
                                  0);

    assert(hResult == S_OK);

    hResult = m_pd3dDevice->BeginScene();
    assert(hResult == S_OK);

    TCHAR msg[100];
    _tcscpy_s(msg, 100, _T("Xファイルの読み込みと表示"));
    TextDraw(m_pFont, msg, 0, 0);

    hResult = m_pEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    UINT numPass;
    hResult = m_pEffect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = m_pEffect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < m_dwNumMaterials; i++)
    {
        hResult = m_pEffect->SetTexture("texture1", m_pTextures[i]);
        assert(hResult == S_OK);

        hResult = m_pEffect->CommitChanges();
        assert(hResult == S_OK);

        hResult = m_pMesh->DrawSubset(i);
        assert(hResult == S_OK);
    }

    if (m_pMesh2 != nullptr)
    {
        m_pMesh2->Render();
    }

    hResult = m_pEffect->EndPass();
    assert(hResult == S_OK);

    hResult = m_pEffect->End();
    assert(hResult == S_OK);

    hResult = m_pd3dDevice->EndScene();
    assert(hResult == S_OK);

    hResult = m_pd3dDevice->Present(NULL, NULL, NULL, NULL);
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
    m_pMesh2 = new Mesh(filePath, pos, rot, scale, radius);
    m_pMesh2->Init();
}

void NSRender::Render::TextDraw(LPD3DXFONT pFont, TCHAR* text, int X, int Y)
{
    RECT rect = { X, Y, 0, 0 };

    // DrawTextの戻り値は文字数である。
    // そのため、hResultの中身が整数でもエラーが起きているわけではない。
    HRESULT hResult = pFont->DrawText(NULL,
                                      text,
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
    m_pEffect->OnLostDevice();

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

    hResult = m_pd3dDevice->Reset(&d3dpp);
    assert(hResult == S_OK);

    m_pEffect->OnResetDevice();
    m_pFont->OnResetDevice();

    m_eWindowModeCurrent = m_eWindowModeRequest;
    m_eWindowModeRequest = eWindowMode::NONE;
}

