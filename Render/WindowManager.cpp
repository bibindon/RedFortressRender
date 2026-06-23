#include "WindowManager.h"

#include <set>
#include <algorithm>

#include "Util.h"

namespace NSRender
{
namespace
{
LONG_PTR AddVisibleStyleIfVisible(const HWND hWnd, const LONG_PTR style)
{
    if (IsWindowVisible(hWnd))
    {
        return style | WS_VISIBLE;
    }

    return style;
}

RECT BuildCenteredWindowRect(const HWND hWnd, const int width, const int height)
{
    RECT rect { };
    SetRect(&rect, 0, 0, width, height);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    rect.right -= rect.left;
    rect.bottom -= rect.top;
    rect.left = 0;
    rect.top = 0;

    HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo { sizeof(monitorInfo) };
    GetMonitorInfo(monitor, &monitorInfo);

    RECT monitorRect = monitorInfo.rcMonitor;
    const int windowWidth = rect.right;
    const int windowHeight = rect.bottom;

    rect.left = monitorRect.left + ((monitorRect.right - monitorRect.left - windowWidth) / 2);
    rect.top = monitorRect.top + ((monitorRect.bottom - monitorRect.top - windowHeight) / 2);
    rect.right = rect.left + windowWidth;
    rect.bottom = rect.top + windowHeight;

    return rect;
}
}

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

    /* FullScreen_RefreshRateInHz must be zero for Windowed mode */
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_INTERVAL_DEFAULT;

    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    LPDIRECT3DDEVICE9 D3DDevice = NULL;

    HRESULT hResult = E_FAIL;

    hResult = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                   D3DDEVTYPE_HAL,
                                   m_hWnd,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                                   &d3dpp,
                                   &D3DDevice);

    if (FAILED(hResult))
    {
        hResult = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL,
                                       m_hWnd,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                                       &d3dpp,
                                       &D3DDevice);

        assert(hResult == S_OK);
    }

    Common::SetD3DDevice(D3DDevice);

}

void WindowManager::Finalize()
{
    m_hWnd = NULL;
    SAFE_RELEASE(m_pD3D);
}

void WindowManager::ChangeResolution(const int W, const int H)
{
    if (W <= 0 || H <= 0)
    {
        return;
    }

    ResetDeviceForMode(m_eWindowModeCurrent, W, H);
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
    std::set<std::pair<int, int>> uniqueResolutions;

    for (const auto& mode : modes)
    {
        const std::pair<int, int> resolution(static_cast<int>(mode.width), static_cast<int>(mode.height));
        if (uniqueResolutions.insert(resolution).second)
        {
            resolutionList.push_back(resolution);
        }
    }

    const std::pair<int, int> targetResolution(1600, 900);
    if (uniqueResolutions.find(targetResolution) == uniqueResolutions.end())
    {
        resolutionList.push_back(targetResolution);
    }

    return resolutionList;
}

eWindowMode WindowManager::GetCurrentWindowMode() const
{
    return m_eWindowModeCurrent;
}

void WindowManager::ChangeWindowMode()
{
    if (m_eWindowModeRequest == eWindowMode::NONE)
    {
        return;
    }

    if (ResetDeviceForMode(m_eWindowModeRequest, Common::ScreenW(), Common::ScreenH()))
    {
        m_eWindowModeRequest = eWindowMode::NONE;
    }
}

void WindowManager::NotifyDeviceLost()
{
    if (!m_bDeviceLost)
    {
        Common::OnDeviceLostAll();
        m_bDeviceLost = true;
    }
}

bool WindowManager::EnsureDeviceReady()
{
    LPDIRECT3DDEVICE9 device = Common::D3DDevice();
    if (device == NULL)
    {
        return false;
    }

    const HRESULT cooperativeResult = device->TestCooperativeLevel();
    if (cooperativeResult == D3D_OK)
    {
        return true;
    }

    if (cooperativeResult == D3DERR_DEVICELOST)
    {
        NotifyDeviceLost();
        return false;
    }

    if (cooperativeResult == D3DERR_DEVICENOTRESET)
    {
        return ResetDeviceForMode(m_eWindowModeCurrent, Common::ScreenW(), Common::ScreenH());
    }

    assert(false);
    return false;
}

std::pair<int, int> WindowManager::ResolveFullscreenResolution(const int requestedWidth,
                                                               const int requestedHeight)
{
    std::vector<DisplayModeInfo> modes = EnumerateFullscreenModes(m_pD3D, D3DADAPTER_DEFAULT);
    for (const auto& mode : modes)
    {
        if (static_cast<int>(mode.width) == requestedWidth &&
            static_cast<int>(mode.height) == requestedHeight)
        {
            return { requestedWidth, requestedHeight };
        }
    }

    if (!modes.empty())
    {
        return { static_cast<int>(modes.front().width), static_cast<int>(modes.front().height) };
    }

    return { requestedWidth, requestedHeight };
}

D3DPRESENT_PARAMETERS WindowManager::CreatePresentParameters(const eWindowMode mode,
                                                             int width,
                                                             int height)
{
    if (mode == eWindowMode::FULLSCREEN)
    {
        const auto resolved = ResolveFullscreenResolution(width, height);
        width = resolved.first;
        height = resolved.second;
    }

    Common::SetScreenW(width);
    Common::SetScreenH(height);

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    d3dpp.Windowed = (mode != eWindowMode::FULLSCREEN);
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    if (mode == eWindowMode::FULLSCREEN)
    {
        d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    }
    d3dpp.BackBufferCount = 1;
    d3dpp.BackBufferWidth = width;
    d3dpp.BackBufferHeight = height;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality = 0;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.hDeviceWindow = m_hWnd;
    d3dpp.Flags = 0;

    if (mode == eWindowMode::FULLSCREEN)
    {
        d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_INTERVAL_IMMEDIATE;
    }
    else
    {
        d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    }

    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    return d3dpp;
}

void WindowManager::UpdateWindowPlacement(const eWindowMode mode, const int width, const int height)
{
    if (mode == eWindowMode::WINDOW)
    {
        const RECT rect = BuildCenteredWindowRect(m_hWnd, width, height);
        const LONG_PTR style = AddVisibleStyleIfVisible(m_hWnd, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME);
        SetWindowLongPtr(m_hWnd,
                         GWL_STYLE,
                         style);
        SetWindowPos(m_hWnd,
                     HWND_TOP,
                     rect.left,
                     rect.top,
                     rect.right - rect.left,
                     rect.bottom - rect.top,
                     SWP_FRAMECHANGED);
        return;
    }

    HMONITOR monitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo { sizeof(monitorInfo) };
    GetMonitorInfo(monitor, &monitorInfo);
    RECT monitorRect = monitorInfo.rcMonitor;

    const LONG_PTR style = AddVisibleStyleIfVisible(m_hWnd, WS_POPUP);
    SetWindowLongPtr(m_hWnd, GWL_STYLE, style);
    SetWindowPos(m_hWnd,
                 HWND_TOP,
                 monitorRect.left,
                 monitorRect.top,
                 monitorRect.right - monitorRect.left,
                 monitorRect.bottom - monitorRect.top,
                 SWP_FRAMECHANGED);
}

bool WindowManager::ResetDeviceForMode(const eWindowMode mode, int width, int height)
{
    NotifyDeviceLost();

    D3DPRESENT_PARAMETERS d3dpp = CreatePresentParameters(mode, width, height);
    UpdateWindowPlacement(mode, static_cast<int>(d3dpp.BackBufferWidth), static_cast<int>(d3dpp.BackBufferHeight));

    const HRESULT hResult = Common::D3DDevice()->Reset(&d3dpp);
    if (hResult == D3DERR_DEVICELOST)
    {
        return false;
    }
    assert(hResult == S_OK);
    if (hResult != S_OK)
    {
        return false;
    }

    Common::OnDeviceResetAll();
    m_eWindowModeCurrent = mode;
    m_bDeviceLost = false;
    return true;
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

