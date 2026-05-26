#pragma once

#include <windows.h>

namespace NSRender
{

class Render;

class RenderSettingsDialog
{
public:
    void Show(HWND parent, Render* render, bool activateDialog);
    void Toggle(HWND parent, Render* render);
    void Finalize();

private:
    HWND m_hWnd = NULL;
};

}
