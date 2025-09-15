
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>

#include "Mesh.h"
#include "AnimMesh.h"
#include "SkinAnimMesh.h"

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
    void AddMesh(const std::wstring& filePath,
                 const D3DXVECTOR3& pos,
                 const D3DXVECTOR3& rot,
                 const float scale,
                 const float radius = -1.f);

    void AddAnimMesh(const std::wstring& filePath,
                     const D3DXVECTOR3& pos,
                     const D3DXVECTOR3& rot,
                     const float scale,
                     const AnimSetMap& animSetMap);

    void AddSkinAnimMesh(const std::wstring& filePath,
                         const D3DXVECTOR3& pos,
                         const D3DXVECTOR3& rot,
                         const float scale,
                         const AnimSetMap& animSetMap);

    void SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt);
    void MoveCamera(const D3DXVECTOR3& pos);
    void RotateCamera(const D3DXVECTOR3& rot);

    D3DXVECTOR3 GetLookAtPos();
    D3DXVECTOR3 GetCameraRotate();

private:

    HWND m_hWnd = NULL;

    void TextDraw(const std::wstring& text, int X, int Y);

    void ChangeWindowMode();

    eWindowMode m_eWindowModeCurrent = eWindowMode::NONE;
    eWindowMode m_eWindowModeRequest = eWindowMode::NONE;

    LPDIRECT3D9 m_pD3D = NULL;
    LPD3DXFONT m_pFont = NULL;

    std::vector<Mesh> m_meshList;
    std::vector<AnimMesh*> m_animMeshList;
    std::vector<SkinAnimMesh*> m_skinAnimMeshList;

    int m_windowSizeWidth = 1600;
    int m_windowSizeHeight = 900;
};
}

