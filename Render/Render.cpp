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

namespace NSRender
{

void Render::Initialize(HWND hWnd)
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

    m_sprite.Initialize();

    // マルチパスレンダリング関連
    {
        hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                           L"res\\shader\\PostEffectSaturate.fx",
                                           NULL,
                                           NULL,
                                           D3DXSHADER_DEBUG,
                                           NULL,
                                           &g_pEffect2,
                                           NULL);
        assert(hResult == S_OK);

        // === 変更: RT を 2 枚作成（両方 A8R8G8B8） ===
        hResult = D3DXCreateTexture(Common::D3DDevice(),
                                    1600,
                                    900,
                                    1,
                                    D3DUSAGE_RENDERTARGET,
                                    D3DFMT_A8R8G8B8,
                                    D3DPOOL_DEFAULT,
                                    &g_pRenderTarget);
        assert(hResult == S_OK);

        hResult = D3DXCreateTexture(Common::D3DDevice(),
                                    1600,
                                    900,
                                    1,
                                    D3DUSAGE_RENDERTARGET,
                                    D3DFMT_A8R8G8B8,
                                    D3DPOOL_DEFAULT,
                                    &g_pRenderTarget2);
        assert(hResult == S_OK);

        // オフスクリーン用テクスチャ
        D3DXCreateTexture(Common::D3DDevice(),
                          m_windowSizeWidth,
                          m_windowSizeHeight,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pSceneTex);

        // フルスクリーンクアッドの頂宣言
        D3DVERTEXELEMENT9 elems[] =
        {
            { 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };
        hResult = Common::D3DDevice()->CreateVertexDeclaration(elems, &g_pQuadDecl);
        assert(hResult == S_OK);

        // スプライト
        hResult = D3DXCreateSprite(Common::D3DDevice(), &g_pSprite);
        assert(hResult == S_OK);

    }

    m_postEffectSaturate.Initialize();

    // ガウスフィルター
    m_postEffectGauss.Initialize();

    // ブルーム
    {
        // エフェクト読み込み
        hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                           _T("res\\shader\\PostEffectBloom.fx"),
                                           NULL,
                                           NULL,
                                           D3DXSHADER_DEBUG,
                                           NULL,
                                           &g_pBloomEffect,
                                           NULL);
        assert(SUCCEEDED(hResult));

        // 各テクスチャ作成（サーフェイスは保持しない）
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pSceneTex2);

        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pBrightTex);

        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pBlurTexH);

        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pBlurTexV);

    }

    // スターバースト
    {

        hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                           L"res\\shader\\PostEffectStarBurst.fx",
                                           NULL,
                                           NULL,
                                           D3DXSHADER_DEBUG,
                                           NULL,
                                           &g_pStarBusrtEffect,
                                           NULL);
        assert(hResult == S_OK);

        // ★ 各テクスチャ作成（サーフェイスは保持しない）
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &m_texPostEffectBack1);

        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pBrightTex2);

        // 0°（水平）
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pBlurTexH2);

        // 60°
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pBlurTexV2);

        // 120°（★追加）
        D3DXCreateTexture(Common::D3DDevice(),
                          1600,
                          900,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pBlurTexD);

    }

    {
        HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                                   L"res\\shader\\PostEffectEnd.fx",
                                                   NULL, NULL,
                                                   D3DXSHADER_DEBUG,
                                                   NULL,
                                                   &g_pEffectEnd,
                                                   NULL);
        assert(SUCCEEDED(hResult));
    }

//    AddMesh(L"cube.x", D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0), 1.f, 1.f);
}

void Render::Finalize()
{
    Common::D3DDevice()->Release();
    Common::SetD3DDevice(NULL);

    SAFE_RELEASE(m_pD3D);
}

void Render::Draw()
{
    HRESULT hResult = E_FAIL;

    DrawPass1();

    // 彩度変更
//    DrawPass2();
    g_pSceneTex = m_postEffectSaturate.Draw(g_pRenderTarget);

    // ブルーム
    DrawPass4();

    // スターバースト
    DrawPass5();

    // ガウス
    //DrawPassGaussian();
    m_texPostEffectBack1 = m_postEffectGauss.Draw(m_texPostEffectBack1);

    DrawPassEnd();

    Draw2D();

    hResult = Common::D3DDevice()->Present(NULL, NULL, NULL, NULL);
    assert(hResult == S_OK);

    if (m_eWindowModeRequest != eWindowMode::NONE)
    {
        ChangeWindowMode();
    }

}

void Render::ChangeResolution(const int W, const int H)
{
}

void Render::ChangeWindowMode(const eWindowMode eWindowMode_)
{
    if (m_eWindowModeRequest != eWindowMode_)
    {
        m_eWindowModeRequest = eWindowMode_;
    }
}

void Render::AddMesh(const std::wstring& filePath,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rot,
                               const float scale,
                               const float radius)
{
    m_meshList.push_back(Mesh(filePath, pos, rot, scale, radius));
    m_meshList.rbegin()->Initialize();
}

void Render::AddMeshSmooth(const std::wstring& filePath,
                                     const D3DXVECTOR3& pos,
                                     const D3DXVECTOR3& rot,
                                     const float scale,
                                     const float radius)
{
    MeshSmooth mesh;
    m_meshSmoothList.push_back(mesh);
    m_meshSmoothList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
}

void Render::AddMeshSSSLike(const std::wstring& filePath,
                                      const D3DXVECTOR3& pos,
                                      const D3DXVECTOR3& rot,
                                      const float scale,
                                      const float radius)
{
    MeshSSSLike mesh;
    m_meshSSSLikeList.push_back(mesh);
    m_meshSSSLikeList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
}

void Render::AddMeshPointLight(const std::wstring& filePath,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rot,
                               const float scale,
                               const float radius)
{
    MeshPointLight mesh;
    m_meshPointLightList.push_back(mesh);
    m_meshPointLightList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
}

void Render::AddMeshNormalMapping(const std::wstring& filePath,
                                  const std::wstring& normalMap,
                                  const D3DXVECTOR3& pos,
                                  const D3DXVECTOR3& rot,
                                  const float scale,
                                  const float radius)
{
    MeshNormalMapping mesh;
    m_meshNormalMapList.push_back(mesh);
    m_meshNormalMapList.rbegin()->Initialize(filePath, normalMap, pos, rot, scale, radius);
}

void Render::AddAnimMesh(const std::wstring& filePath,
                                   const D3DXVECTOR3& pos,
                                   const D3DXVECTOR3& rot,
                                   const float scale,
                                   const AnimSetMap& animSetMap)
{
    AnimMesh* animMesh = NEW AnimMesh(filePath, pos, rot, scale, animSetMap);
    m_animMeshList.push_back(animMesh);
}

void Render::AddSkinAnimMesh(const std::wstring& filePath,
                                       const D3DXVECTOR3& pos,
                                       const D3DXVECTOR3& rot,
                                       const float scale,
                                       const AnimSetMap& animSetMap)
{
    SkinAnimMesh* mesh = NEW SkinAnimMesh(filePath, pos, rot, scale, animSetMap);
    m_skinAnimMeshList.push_back(mesh);
}

void Render::AddMeshInstansing(const std::wstring& filePath,
                                         const D3DXVECTOR3& pos,
                                         const D3DXVECTOR3& rot,
                                         const float scale)
{

    if (m_meshInstancingMap.find(filePath) == m_meshInstancingMap.end())
    {
        MeshInstancing* mesh = NEW MeshInstancing();
        mesh->Initialize();

        m_meshInstancingMap[filePath] = mesh;
    }

    m_meshInstancingMap[filePath]->AddInstance(pos);
}

void Render::SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt)
{
    Camera::SetEyePos(pos);
    Camera::SetLookAtPos(lookAt);
}

void Render::MoveCamera(const D3DXVECTOR3& pos)
{
    auto eyePos = Camera::GetEyePos();
    Camera::SetEyePos(eyePos + pos);

    auto lookAtPos = Camera::GetLookAtPos();
    Camera::SetLookAtPos(lookAtPos + pos);
}

D3DXVECTOR3 Render::GetLookAtPos()
{
    return Camera::GetLookAtPos();
}

D3DXVECTOR3 Render::GetCameraRotate()
{
    auto eyePos = Camera::GetEyePos();
    auto lookAtPos = Camera::GetLookAtPos();
    auto dir(lookAtPos - eyePos);
    D3DXVec3Normalize(&dir, &dir);
    return dir;
}

int Render::SetUpFont(const std::wstring& fontName,
                                const int fontSize,
                                const UINT fontColor)
{
    Font font;
    font.Initialize(fontName, fontSize, fontColor);
    m_fontList.push_back(font);

    return (int)(m_fontList.size() - 1);
}

void Render::DrawText_(const int fontId,
                                 const std::wstring& text,
                                 const int X,
                                 const int Y)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId).AddText(text, X, Y);
}

void Render::DrawText_(const int fontId,
                                 const std::wstring& text,
                                 const int X,
                                 const int Y,
                                 const UINT color)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId).AddText(text, X, Y, color);
}

void Render::DrawTextCenter(const int fontId,
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

void Render::DrawTextCenter(const int fontId,
                                      const std::wstring& text,
                                      const int X,
                                      const int Y,
                                      const int Width,
                                      const int Height,
                                      const UINT color)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId).AddTextCenter(text, X, Y, Width, Height, color);
}

void Render::DrawImage(const std::wstring& text,
                                 const int X,
                                 const int Y,
                                 const int transparency)
{
    m_sprite.LoadImage_(text);
    m_sprite.PlaceImage(text, X, Y, transparency);
}

void Render::SetPostEffectSaturate(const float level)
{
    m_postEffectSaturate.SetPostEffectSaturate(level);
}

void Render::SetPostEffectGaussianFilter(const bool arg)
{
    m_postEffectGauss.SetEnable(arg);
}

void Render::SetPostEffectStarBurst(const bool arg)
{
    m_bStarBurstON = arg;
}

void Render::RotateCamera(const D3DXVECTOR3& rot)
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

// TODO いずれちゃんと書くこと
void Render::ChangeWindowMode()
{
    HRESULT hResult = E_FAIL;

    for (auto& elem : m_meshList)
    {
        elem.OnDeviceLost();
    }

    for (auto& elem : m_meshSmoothList)
    {
        elem.OnDeviceLost();
    }

    for (auto& elem : m_meshSSSLikeList)
    {
        elem.OnDeviceLost();
    }

    for (auto& elem : m_meshPointLightList)
    {
        elem.OnDeviceLost();
    }

    for (auto& elem : m_meshNormalMapList)
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

    for (auto& elem : m_meshSmoothList)
    {
        elem.OnDeviceReset();
    }

    for (auto& elem : m_meshSSSLikeList)
    {
        elem.OnDeviceReset();
    }

    for (auto& elem : m_meshPointLightList)
    {
        elem.OnDeviceReset();
    }

    for (auto& elem : m_meshNormalMapList)
    {
        elem.OnDeviceReset();
    }

    for (auto& elem : m_animMeshList)
    {
        elem->OnDeviceReset();
    }

    m_eWindowModeCurrent = m_eWindowModeRequest;
    m_eWindowModeRequest = eWindowMode::NONE;
}

void Render::DrawPass1()
{
    HRESULT hResult = E_FAIL;

    // 既存の RT0 を保存
    LPDIRECT3DSURFACE9 pOldRT0 = NULL;
    hResult = Common::D3DDevice()->GetRenderTarget(0, &pOldRT0);
    assert(hResult == S_OK);

    // 2 枚の RT サーフェスを取得
    LPDIRECT3DSURFACE9 pRT0 = NULL;
    LPDIRECT3DSURFACE9 pRT1 = NULL;

    hResult = g_pRenderTarget->GetSurfaceLevel(0, &pRT0);
    assert(hResult == S_OK);

    hResult = g_pRenderTarget2->GetSurfaceLevel(0, &pRT1);
    assert(hResult == S_OK);

    // MRT セット（スロット 0 と 1）
    hResult = Common::D3DDevice()->SetRenderTarget(0, pRT0);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderTarget(1, pRT1);
    assert(hResult == S_OK);




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

    for (auto& elem : m_meshSmoothList)
    {
        elem.Draw();
    }

    for (auto& elem : m_meshSSSLikeList)
    {
        elem.Draw();
    }

    for (auto& elem : m_meshPointLightList)
    {
        elem.Draw();
    }

    for (auto& elem : m_meshNormalMapList)
    {
        elem.Draw();
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

    for (auto& elem : m_meshInstancingMap)
    {
        elem.second->Draw();
    }

    hResult = Common::D3DDevice()->EndScene();
    assert(hResult == S_OK);

    // MRT を解除してバックバッファへ戻す
    hResult = Common::D3DDevice()->SetRenderTarget(1, NULL);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderTarget(0, pOldRT0);
    assert(hResult == S_OK);

    SAFE_RELEASE(pRT0);
    SAFE_RELEASE(pRT1);
    SAFE_RELEASE(pOldRT0);


}


// TODO ポストエフェクト用のクラスを作成する
void Render::DrawPass2()
{
    HRESULT hResult = E_FAIL;

    // 既存RT0を退避
    LPDIRECT3DSURFACE9 pOldRT0 = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &pOldRT0);

    // ★ g_pSceneTex のサーフェスを取得して RT0 にセット
    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    g_pSceneTex->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    // Zは使わない
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);

    Common::D3DDevice()->Clear(0, NULL,
                               D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    Common::D3DDevice()->BeginScene();

    // フルスクリーン: RT0(=g_pSceneTex) へ彩度フィルタ適用
    g_pEffect2->SetTechnique("Technique1");
    UINT numPass = 0;
    g_pEffect2->Begin(&numPass, 0);
    g_pEffect2->BeginPass(0);

    g_pEffect2->SetFloat("g_level", m_saturateLevel);
    g_pEffect2->SetTexture("texture1", g_pRenderTarget); // 入力はPass1の結果
    g_pEffect2->CommitChanges();

    DrawFullscreenQuad(); // 現在のテクニックで全画面描画

    g_pEffect2->EndPass();
    g_pEffect2->End();

    // === 追加: 左上に RT1 を 1/2 スケールで表示（D3DXSPRITE） ===
    if (false)
    {
        if (g_pSprite)
        {
            hResult = g_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
            assert(hResult == S_OK);

            D3DXMATRIX mat;
            D3DXVECTOR2 scaling(0.5f, 0.5f);     // 半分
            D3DXVECTOR2 trans(0.0f, 0.0f);       // 左上
            D3DXMatrixTransformation2D(&mat, NULL, 0.0f, &scaling, NULL, 0.0f, &trans);
            g_pSprite->SetTransform(&mat);

            // そのまま (0,0) へ描画

            // マルチターゲットレンダリングが未実装なので何も映らない。
            // どう実装すればいいのか謎
            //hResult = g_pSprite->Draw(g_pRenderTarget2, NULL, NULL, NULL, 0xFFFFFFFF);

            //hResult = g_pSprite->Draw(g_pRenderTarget, NULL, NULL, NULL, 0xFFFFFFFF);
            hResult = g_pSprite->Draw(g_pSceneTex, NULL, NULL, NULL, 0xFFFFFFFF);
            assert(hResult == S_OK);

            hResult = g_pSprite->End();
            assert(hResult == S_OK);
        }
    }

    hResult = Common::D3DDevice()->EndScene();
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
    assert(hResult == S_OK);

    Common::D3DDevice()->SetRenderTarget(0, pOldRT0);
    SAFE_RELEASE(pOldRT0);
}

void Render::SetRTFromTex(LPDIRECT3DTEXTURE9 tex)
{
    LPDIRECT3DSURFACE9 rt = NULL;
    tex->GetSurfaceLevel(0, &rt);                 // AddRef 済みで返る
    Common::D3DDevice()->SetRenderTarget(0, rt);         // Device 側が参照を保持
    SAFE_RELEASE(rt);                             // 即ReleaseでOK
}

void Render::SetRTBackBuffer()
{
    LPDIRECT3DSURFACE9 bb = NULL;
    Common::D3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    Common::D3DDevice()->SetRenderTarget(0, bb);
    SAFE_RELEASE(bb);
}


void Render::DrawPass4()
{
    // Bloom 用シェーダが未ロードなら何もしない
    if (g_pBloomEffect == NULL) return;

    // テクセルサイズ（ブラーで使用）
    float texelSize[2] = { 1.0f / m_windowSizeWidth, 1.0f / m_windowSizeHeight };
    g_pBloomEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    // フルスクリーン板ポリ
    ScreenVertex quad[4] =
    {
        {                     -0.5f,                      -0.5f, 0, 1, 0, 0 },
        {  m_windowSizeWidth - 0.5f,                      -0.5f, 0, 1, 1, 0 },
        {                     -0.5f,   m_windowSizeHeight - 0.5f, 0, 1, 0, 1 },
        {  m_windowSizeWidth - 0.5f,   m_windowSizeHeight - 0.5f, 0, 1, 1, 1 },
    };

    // ------------------------------------------------------------
    // (1) BrightPass : 入力 = g_pSceneTex, 出力 = g_pBrightTex
    // ------------------------------------------------------------
    {
        LPDIRECT3DSURFACE9 pRT = NULL;
        g_pBrightTex->GetSurfaceLevel(0, &pRT);
        Common::D3DDevice()->SetRenderTarget(0, pRT);
        SAFE_RELEASE(pRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();

        g_pBloomEffect->SetTechnique("BrightPass");
        g_pBloomEffect->SetTexture("g_SrcTex", g_pSceneTex);

        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
        Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
        g_pBloomEffect->Begin(NULL, 0);
        g_pBloomEffect->BeginPass(0);
        Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
        g_pBloomEffect->EndPass();
        g_pBloomEffect->End();
        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);

        Common::D3DDevice()->EndScene();
    }

    // ------------------------------------------------------------
    // (2) Horizontal Blur : 入力 = g_pBrightTex, 出力 = g_pBlurTexH
    // ------------------------------------------------------------
    {
        LPDIRECT3DSURFACE9 pRT = NULL;
        g_pBlurTexH->GetSurfaceLevel(0, &pRT);
        Common::D3DDevice()->SetRenderTarget(0, pRT);
        SAFE_RELEASE(pRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();

        g_pBloomEffect->SetTechnique("Blur");
        g_pBloomEffect->SetTexture("g_SrcTex", g_pBrightTex);
        {
            // 横方向
            float dir[4] = { 1, 0, 0, 0 };
            g_pBloomEffect->SetFloatArray("g_Direction", dir, 4);
        }

        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
        Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
        g_pBloomEffect->Begin(NULL, 0);
        g_pBloomEffect->BeginPass(0);
        Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
        g_pBloomEffect->EndPass();
        g_pBloomEffect->End();
        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);

        Common::D3DDevice()->EndScene();
    }

    // ------------------------------------------------------------
    // (3) Vertical Blur : 入力 = g_pBlurTexH, 出力 = g_pBlurTexV
    // ------------------------------------------------------------
    {
        LPDIRECT3DSURFACE9 pRT = NULL;
        g_pBlurTexV->GetSurfaceLevel(0, &pRT);
        Common::D3DDevice()->SetRenderTarget(0, pRT);
        SAFE_RELEASE(pRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();

        g_pBloomEffect->SetTechnique("Blur");
        g_pBloomEffect->SetTexture("g_SrcTex", g_pBlurTexH);
        {
            // 縦方向
            float dir[4] = { 0, 1, 0, 0 };
            g_pBloomEffect->SetFloatArray("g_Direction", dir, 4);
        }

        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
        Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
        g_pBloomEffect->Begin(NULL, 0);
        g_pBloomEffect->BeginPass(0);
        Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
        g_pBloomEffect->EndPass();
        g_pBloomEffect->End();
        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);

        Common::D3DDevice()->EndScene();
    }

    // ------------------------------------------------------------
    // (4) Combine : (SceneTex + BlurTexV) → g_pSceneTex2
    // ------------------------------------------------------------
    {
        LPDIRECT3DSURFACE9 pRT = NULL;
        g_pSceneTex2->GetSurfaceLevel(0, &pRT);
        Common::D3DDevice()->SetRenderTarget(0, pRT);
        SAFE_RELEASE(pRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();

        g_pBloomEffect->SetTechnique("Combine");
        g_pBloomEffect->SetTexture("g_SceneTex", g_pSceneTex);
        g_pBloomEffect->SetTexture("g_BlurTex", g_pBlurTexV);

        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
        Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
        g_pBloomEffect->Begin(NULL, 0);
        g_pBloomEffect->BeginPass(0);
        Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
        g_pBloomEffect->EndPass();
        g_pBloomEffect->End();
        Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);

        Common::D3DDevice()->EndScene();
    }
}

void Render::DrawPass5()
{
    // 最終表示は DrawPassEnd で g_pSceneTex3 を画面に出す想定
    // （Render::DrawPassEnd で g_SrcTex ← m_texPostEffectBack1 をコピー） 
    // ※ g_pEffectEnd の Copy を使って素通しにも対応。 :contentReference[oaicite:6]{index=6}

    // シェーダ未ロード or 機能OFFなら g_pSceneTex2 → g_pSceneTex3 をコピーして終了
    if (g_pStarBusrtEffect == NULL || !m_bStarBurstON)
    {
        LPDIRECT3DSURFACE9 pRT = NULL;
        m_texPostEffectBack1->GetSurfaceLevel(0, &pRT);
        Common::D3DDevice()->SetRenderTarget(0, pRT);
        SAFE_RELEASE(pRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();
        // 汎用コピー（エフェクトEndの"Copy"）
        DrawFullScreenQuad(g_pSceneTex2, g_pEffectEnd, "Copy");
        Common::D3DDevice()->EndScene();
        return;
    }

    // テクセルサイズ（ブラーで使用）
    float texelSize[2] = { 1.0f / m_windowSizeWidth, 1.0f / m_windowSizeHeight };
    g_pStarBusrtEffect->SetFloatArray("g_TexelSize", texelSize, 2); // :contentReference[oaicite:7]{index=7}

    // (2) BrightPass : 入力=g_pSceneTex2, 出力=g_pBrightTex2
    SetRTFromTex(g_pBrightTex2);
    Common::D3DDevice()->BeginScene();
    DrawFullScreenQuad(g_pSceneTex2, g_pStarBusrtEffect, "BrightPass"); // :contentReference[oaicite:8]{index=8}
    Common::D3DDevice()->EndScene();

    // (3a) 0° ブラー : 入力=g_pBrightTex2, 出力=g_pBlurTexH2
    SetRTFromTex(g_pBlurTexH2);
    Common::D3DDevice()->BeginScene();
    { float dir[4] = { 1.0f, 0.0f, 0, 0 }; g_pStarBusrtEffect->SetFloatArray("g_Direction", dir, 4); } // :contentReference[oaicite:9]{index=9}
    DrawFullScreenQuad(g_pBrightTex2, g_pStarBusrtEffect, "Blur"); // :contentReference[oaicite:10]{index=10}
    Common::D3DDevice()->EndScene();

    // (3b) 60° ブラー : 入力=g_pBrightTex2, 出力=g_pBlurTexV2
    SetRTFromTex(g_pBlurTexV2);
    Common::D3DDevice()->BeginScene();
    { float dir[4] = { 0.5f, 0.8660254f, 0, 0 }; g_pStarBusrtEffect->SetFloatArray("g_Direction", dir, 4); }
    DrawFullScreenQuad(g_pBrightTex2, g_pStarBusrtEffect, "Blur");
    Common::D3DDevice()->EndScene();

    // (3c) 120° ブラー : 入力=g_pBrightTex2, 出力=g_pBlurTexD
    SetRTFromTex(g_pBlurTexD);
    Common::D3DDevice()->BeginScene();
    { float dir[4] = { -0.5f, 0.8660254f, 0, 0 }; g_pStarBusrtEffect->SetFloatArray("g_Direction", dir, 4); }
    DrawFullScreenQuad(g_pBrightTex2, g_pStarBusrtEffect, "Blur");
    Common::D3DDevice()->EndScene();

    // (4) 合成 : (SceneTex2 + 0° + 60° + 120°) → g_pSceneTex3
    SetRTFromTex(m_texPostEffectBack1);
    Common::D3DDevice()->BeginScene();
    g_pStarBusrtEffect->SetTexture("g_SceneTex", g_pSceneTex2);
    g_pStarBusrtEffect->SetTexture("g_BlurTexH", g_pBlurTexH2);
    g_pStarBusrtEffect->SetTexture("g_BlurTexV", g_pBlurTexV2);
    g_pStarBusrtEffect->SetTexture("g_BlurTex60", g_pBlurTexD); // Combine は3軸を加算 :contentReference[oaicite:11]{index=11}
    DrawFullScreenQuad(NULL, g_pStarBusrtEffect, "Combine");
    Common::D3DDevice()->EndScene();
}


void Render::DrawPassEnd()
{
    // 最終表示（固定機能ではなくシェーダでコピー）
    if (g_pEffectEnd == NULL) return;

    // バックバッファを RT にセット（都度取得→即 Release）
    LPDIRECT3DSURFACE9 pBackBuffer = NULL;
    Common::D3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    Common::D3DDevice()->SetRenderTarget(0, pBackBuffer);
    SAFE_RELEASE(pBackBuffer);

    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    Common::D3DDevice()->BeginScene();

    // フルスクリーン板ポリ
    ScreenVertex quad[4] =
    {
        {                     -0.5f,                      -0.5f, 0, 1, 0, 0 },
        {  m_windowSizeWidth - 0.5f,                      -0.5f, 0, 1, 1, 0 },
        {                     -0.5f,   m_windowSizeHeight - 0.5f, 0, 1, 0, 1 },
        {  m_windowSizeWidth - 0.5f,   m_windowSizeHeight - 0.5f, 0, 1, 1, 1 },
    };

    // ここでは DrawPass4 の合成結果（g_pSceneTex2）を画面にコピー
    g_pEffectEnd->SetTechnique("Copy");
    g_pEffectEnd->SetTexture("g_SrcTex", m_texPostEffectBack1);

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    g_pEffectEnd->Begin(NULL, 0);
    g_pEffectEnd->BeginPass(0);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
    g_pEffectEnd->EndPass();
    g_pEffectEnd->End();
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);

    Common::D3DDevice()->EndScene();
}


void Render::Draw2D()
{
    // 文字と画像は彩度フィルタの影響を受けないようにする
    for (auto& elem : m_fontList)
    {
        elem.Draw();
    }

    m_sprite.Draw();

}

void Render::DrawFullscreenQuad()
{
    QuadVertex v[4] { };

    float du = 0.5f / 1600.f;
    float dv = 0.5f / 900.f;

    v[0].x = -1.0f;
    v[0].y = -1.0f;
    v[0].z = 0.0f;
    v[0].w = 1.0f;
    v[0].u = 0.0f + du;
    v[0].v = 1.0f - dv;

    v[1].x = -1.0f;
    v[1].y = 1.0f;
    v[1].z = 0.0f;
    v[1].w = 1.0f;
    v[1].u = 0.0f + du;
    v[1].v = 0.0f + dv;

    v[2].x = 1.0f;
    v[2].y = -1.0f;
    v[2].z = 0.0f;
    v[2].w = 1.0f;
    v[2].u = 1.0f - du;
    v[2].v = 1.0f - dv;

    v[3].x = 1.0f;
    v[3].y = 1.0f;
    v[3].z = 0.0f;
    v[3].w = 1.0f;
    v[3].u = 1.0f - du;
    v[3].v = 0.0f + dv;

    Common::D3DDevice()->SetVertexDeclaration(g_pQuadDecl);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(QuadVertex));
}

void Render::DrawFullScreenQuad(LPDIRECT3DTEXTURE9 tex, LPD3DXEFFECT effect, const char* technique)
{
    // 固定機能（FVF）で描くため、前のパスの VS/頂点宣言を解除
    Common::D3DDevice()->SetVertexShader(NULL);
    Common::D3DDevice()->SetVertexDeclaration(NULL);

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);

    SCREENVERTEX vertices[4] =
    {
        { -0.5f,  -0.5f,   0, 1, 0, 0 },
        { 1599.5f, -0.5f,   0, 1, 1, 0 },
        { -0.5f,  899.5f,  0, 1, 0, 1 },
        { 1599.5f, 899.5f,  0, 1, 1, 1 },
    };

    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    effect->SetTechnique(technique);
    if (tex) effect->SetTexture("g_SrcTex", tex);

    UINT nPass = 0;
    effect->Begin(&nPass, 0);
    effect->BeginPass(0);
    // （BeginPass後にVSがバインドされるテクでも、上でNULLにしているので固定機能で通る）
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(SCREENVERTEX));
    effect->EndPass();
    effect->End();

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}


}
