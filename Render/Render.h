// ひとまずComPtrは無しでやってみる。
// 必要になったら考える


#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>

#include "Mesh.h"
#include "MeshSmooth.h"
#include "MeshSSSLike.h"
#include "MeshPointLight.h"

#include "AnimMesh.h"
#include "SkinAnimMesh.h"

#include "MeshInstancing.h"

#include "Font.h"
#include "Sprite.h"

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

    void AddMeshSmooth(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius = -1.f);

    void AddMeshSSSLike(const std::wstring& filePath,
                        const D3DXVECTOR3& pos,
                        const D3DXVECTOR3& rot,
                        const float scale,
                        const float radius = -1.f);

    void AddMeshPointLight(const std::wstring& filePath,
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

    // インスタンシング可能なメッシュ
    void AddMeshInstansing(const std::wstring& filePath,
                           const D3DXVECTOR3& pos,
                           const D3DXVECTOR3& rot,
                           const float scale);

    void SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt);
    void MoveCamera(const D3DXVECTOR3& pos);
    void RotateCamera(const D3DXVECTOR3& rot);

    D3DXVECTOR3 GetLookAtPos();
    D3DXVECTOR3 GetCameraRotate();

    // フォント作成
    // IDが返ってくるので、そのIDを文字描画するときに指定する
    int SetUpFont(const std::wstring& fontName, const int fontSize, const UINT fontColor);

    // フォント作成時に取得したIDを指定して文字を描画する
    // 文字が表示され続けるためにはこの関数を毎フレーム実行する必要がある。
    void DrawText_(const int fontId,
                   const std::wstring& text,
                   const int X,
                   const int Y);

    void DrawText_(const int fontId,
                   const std::wstring& text,
                   const int X,
                   const int Y,
                   const UINT color);

    void DrawTextCenter(const int fontId,
                        const std::wstring& text,
                        const int X,
                        const int Y,
                        const int Width,
                        const int Height);

    void DrawTextCenter(const int fontId,
                        const std::wstring& text,
                        const int X,
                        const int Y,
                        const int Width,
                        const int Height,
                        const UINT color);

    void DrawImage(const std::wstring& text,
                   const int X,
                   const int Y,
                   const int transparency = 255);

    // 彩度をどれくらい上げるか（下げるか）を設定
    void SetFilterSaturate(const float level);

private:

    HWND m_hWnd = NULL;

    void ChangeWindowMode();

    eWindowMode m_eWindowModeCurrent = eWindowMode::NONE;
    eWindowMode m_eWindowModeRequest = eWindowMode::NONE;

    LPDIRECT3D9 m_pD3D = NULL;

    std::vector<Mesh> m_meshList;
    std::vector<AnimMesh*> m_animMeshList;
    std::vector<SkinAnimMesh*> m_skinAnimMeshList;
    std::vector<MeshSmooth> m_meshSmoothList;
    std::vector<MeshSSSLike> m_meshSSSLikeList;
    std::vector<MeshPointLight> m_meshPointLightList;

    std::unordered_map<std::wstring, MeshInstancing*> m_meshInstancingMap;

    int m_windowSizeWidth = 1600;
    int m_windowSizeHeight = 900;

    std::vector<Font> m_fontList;
    Sprite m_sprite;

    //---------------------------------------------------------------
    // マルチパスレンダリング関連
    //---------------------------------------------------------------

    void DrawPass1();

    // 彩度フィルター
    // TODO ポストエフェクト用のクラスを作る
    void DrawPass2();

    // ガウスフィルター
    // TODO ポストエフェクト用のクラスを作る
    void DrawPass3();

    void Draw2D();

    LPD3DXEFFECT g_pEffect2 = NULL;

    LPDIRECT3DTEXTURE9 g_pRenderTarget = NULL;
    LPDIRECT3DTEXTURE9 g_pRenderTarget2 = NULL;

    // フルスクリーンクアッド用
    LPDIRECT3DVERTEXDECLARATION9 g_pQuadDecl = NULL;

    // 追加: スプライト
    LPD3DXSPRITE g_pSprite = NULL;

    struct QuadVertex
    {
        float x, y, z, w; // クリップ空間（-1..1, w=1）
        float u, v;       // テクスチャ座標
    };

    void DrawFullscreenQuad();

    // 彩度フィルター
    float m_saturateLevel = 1.0f;

    //---------------------------------------------------------------
    // ガウスフィルター
    // TODO 別のクラスにする
    //---------------------------------------------------------------

    LPD3DXEFFECT g_pEffect3 = NULL;

    LPDIRECT3DTEXTURE9 g_pSceneTex = NULL;
    LPDIRECT3DTEXTURE9 g_pTempTex = NULL;


    // フルスクリーンクアッド用
//    LPDIRECT3DVERTEXDECLARATION9 g_pQuadDecl = NULL;

    void DrawFullscreenQuad(LPDIRECT3DTEXTURE9 tex, const char* tech);

    struct ScreenVertex
    {
        float x, y, z, rhw;
        float u, v;
    };


};
}

