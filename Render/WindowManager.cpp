#include "WindowManager.h"
#include <set>
#include <algorithm>

namespace NSRender
{

void WindowManager::Initialize(const HWND hWnd)
{
    m_hWnd = hWnd;

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

    HRESULT hResult = E_FAIL;

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

}

void WindowManager::Finalize()
{
    m_hWnd = NULL;
    m_pD3D = NULL;
}

void WindowManager::ChangeResolution(const int W, const int H)
{

}

void WindowManager::RequestWindowMode(const eWindowMode eWindowMode_)
{
    if (m_eWindowModeRequest != eWindowMode_)
    {
        m_eWindowModeRequest = eWindowMode_;
    }
}

std::vector<std::pair<int, int>> WindowManager::GetResolutionList()
{
    std::vector<DisplayModeInfo> modes = EnumerateFullscreenModes(m_pD3D, D3DADAPTER_DEFAULT);

    std::vector<std::pair<int, int>> resolutionList;

    for (auto& mode : modes)
    {
        resolutionList.push_back({ mode.width, mode.height });
    }

    return resolutionList;
}

void WindowManager::ChangeWindowMode()
{
    if (m_eWindowModeRequest == eWindowMode::NONE)
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    Common::OnDeviceLostAll();

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

    Common::OnDeviceResetAll();

    m_eWindowModeCurrent = m_eWindowModeRequest;
    m_eWindowModeRequest = eWindowMode::NONE;
}

std::vector<D3DFORMAT> WindowManager::GetCandidateFormats()
{
    std::vector<D3DFORMAT> formats;

    formats.push_back(D3DFMT_A8R8G8B8);
    formats.push_back(D3DFMT_X8R8G8B8);
    formats.push_back(D3DFMT_R5G6B5);

    return formats;
}

std::vector<DisplayModeInfo> WindowManager::EnumerateFullscreenModes(LPDIRECT3D9 d3d,
                                                                     UINT adapterIndex)
{
    std::vector<DisplayModeInfo> result;
    std::set<std::tuple<UINT, UINT, UINT, int> > seen;

    if (d3d == NULL)
    {
        return result;
    }

    std::vector<D3DFORMAT> candidateFormats = GetCandidateFormats();

    for (size_t formatIndex = 0; formatIndex < candidateFormats.size(); ++formatIndex)
    {
        D3DFORMAT format = candidateFormats[formatIndex];

        UINT modeCount = d3d->GetAdapterModeCount(adapterIndex, format);

        for (UINT i = 0; i < modeCount; ++i)
        {
            D3DDISPLAYMODE mode {};
            HRESULT hr = d3d->EnumAdapterModes(adapterIndex, format, i, &mode);

            if (FAILED(hr))
            {
                continue;
            }

            std::tuple<UINT, UINT, UINT, int> key =
                std::make_tuple(mode.Width, mode.Height, mode.RefreshRate, static_cast<int>(mode.Format));

            if (seen.find(key) == seen.end())
            {
                seen.insert(key);

                DisplayModeInfo info {};
                info.width = mode.Width;
                info.height = mode.Height;
                info.refreshRate = mode.RefreshRate;
                info.format = mode.Format;

                result.push_back(info);
            }
        }
    }

    std::sort(result.begin(), result.end(),
              [](const DisplayModeInfo& a, const DisplayModeInfo& b)
              {
                  if (a.width != b.width)
                  {
                      return a.width < b.width;
                  }

                  if (a.height != b.height)
                  {
                      return a.height < b.height;
                  }

                  if (a.refreshRate != b.refreshRate)
                  {
                      return a.refreshRate < b.refreshRate;
                  }

                  return static_cast<int>(a.format) < static_cast<int>(b.format);
              });

    return result;
}

}

