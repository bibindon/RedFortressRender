
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

    LPDIRECT3D9 m_pD3D = NULL;
    LPDIRECT3DDEVICE9 m_pd3dDevice = NULL;
    LPD3DXFONT m_pFont = NULL;
    LPD3DXMESH m_pMesh = NULL;
    std::vector<D3DMATERIAL9> m_pMaterials;
    std::vector<LPDIRECT3DTEXTURE9> m_pTextures;
    DWORD m_dwNumMaterials = 0;
    LPD3DXEFFECT m_pEffect = NULL;

};
}

