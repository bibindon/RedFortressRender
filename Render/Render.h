
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>

namespace NSRender
{

enum class eWindowMode
{
    WINDOW,
    BORDERLESS,
    FULLSCREEN,
    NONE,
};

class Render
{
public:

    void Initialize(HWND hWnd);
    void Finalize();
    void Draw();

    void ChangeResolution(const int W, const int H);

    void ChangeWindowMode(const eWindowMode eWindowMode_);

private:

    HWND m_hWnd = NULL;

    void TextDraw(LPD3DXFONT pFont, TCHAR* text, int X, int Y);

    void ChangeWindowMode();

    eWindowMode m_eWindowModeCurrent = eWindowMode::NONE;
    eWindowMode m_eWindowModeRequest = eWindowMode::NONE;

};
}

