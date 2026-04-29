#pragma once

#include "Common.h"

namespace NSRender
{

enum class eWindowMode
{
    WINDOW,
    BORDERLESS,
    FULLSCREEN,
    NONE,
};

struct DisplayModeInfo
{
    UINT width;
    UINT height;
    UINT refreshRate;
    D3DFORMAT format;
};

class WindowManager
{

public:

    void Initialize(const HWND hWnd);

    void Finalize();

    void ChangeResolution(const int W, const int H);

    void RequestWindowMode(const eWindowMode eWindowMode_);

    void ChangeWindowMode();

    std::vector<std::pair<int, int>> GetResolutionList();

private:

    std::vector<D3DFORMAT> GetCandidateFormats();

    std::vector<DisplayModeInfo> EnumerateFullscreenModes(LPDIRECT3D9 d3d, UINT adapterIndex);
    std::pair<int, int> ResolveFullscreenResolution(int requestedWidth, int requestedHeight);
    D3DPRESENT_PARAMETERS CreatePresentParameters(eWindowMode mode, int width, int height);
    void UpdateWindowPlacement(eWindowMode mode, int width, int height);
    void ResetDeviceForMode(eWindowMode mode, int width, int height);

    eWindowMode m_eWindowModeCurrent = eWindowMode::WINDOW;
    eWindowMode m_eWindowModeRequest = eWindowMode::NONE;

    HWND m_hWnd = NULL;
    LPDIRECT3D9 m_pD3D = NULL;

};

}

