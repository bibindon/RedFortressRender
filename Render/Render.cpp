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

    // ガウスフィルター
    {
        // エフェクト読み込み
        hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                           _T("res\\shader\\PostEffectGaussian.fx"),
                                           NULL,
                                           NULL,
                                           D3DXSHADER_DEBUG,
                                           NULL,
                                           &g_pEffect3,
                                           NULL);
        assert(SUCCEEDED(hResult));

        // オフスクリーン用テクスチャ
        D3DXCreateTexture(Common::D3DDevice(),
                          m_windowSizeWidth,
                          m_windowSizeHeight,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pSceneTex);

        // ブラー用一時テクスチャ
        D3DXCreateTexture(Common::D3DDevice(),
                          m_windowSizeWidth,
                          m_windowSizeHeight,
                          1,
                          D3DUSAGE_RENDERTARGET,
                          D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          &g_pTempTex);

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

    DrawPass2();

    DrawPass3();

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
    m_saturateLevel = level;
}

void Render::SetPostEffectGaussianFilter(const bool arg)
{
    m_bGaussianON = arg;
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
    UINT numPass = 0; g_pEffect2->Begin(&numPass, 0);
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

void Render::DrawPass3()
{
    g_pEffect3->SetBool("g_bFilterON", m_bGaussianON);

    // 2) 横ブラー: 入力=g_pSceneTex, 出力=g_pTempTex（現状のままでOK）
    {
        IDirect3DSurface9* pTempRT = NULL;
        g_pTempTex->GetSurfaceLevel(0, &pTempRT);
        Common::D3DDevice()->SetRenderTarget(0, pTempRT);
        SAFE_RELEASE(pTempRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();
        DrawFullscreenQuad(g_pSceneTex, "GaussianH");
        Common::D3DDevice()->EndScene();
    }

    // 3) 縦ブラー: 入力=g_pTempTex, 出力=★g_pSceneTex（←ここを画面ではなくRTへ）
    {
        IDirect3DSurface9* pSceneRT = NULL;
        g_pSceneTex->GetSurfaceLevel(0, &pSceneRT);
        Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
        SAFE_RELEASE(pSceneRT);

        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
        Common::D3DDevice()->BeginScene();
        DrawFullscreenQuad(g_pTempTex, "GaussianV");
        Common::D3DDevice()->EndScene();
    }
}

void Render::DrawPassEnd()
{
    // 1) バックバッファを RT に
    IDirect3DSurface9* pBackBuffer = NULL;
    Common::D3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    Common::D3DDevice()->SetRenderTarget(0, pBackBuffer);
    SAFE_RELEASE(pBackBuffer);

    // 2) クリア
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

    // 3) シーン開始
    Common::D3DDevice()->BeginScene();

    // === ここがポイント: エフェクト End(Copy) + 頂点宣言で描く ===
    LPDIRECT3DTEXTURE9 srcTex = g_pSceneTex;
    g_pEffectEnd->SetTechnique("Copy");
    g_pEffectEnd->SetTexture("g_SrcTex", srcTex);

    // DrawFullscreenQuad() の頂点をそのまま利用
    QuadVertex v[4] {};
    const float du = 0.5f / 1600.f;
    const float dv = 0.5f / 900.f;
    v[0] = { -1.0f, -1.0f, 0, 1, 0.0f + du, 1.0f - dv };
    v[1] = { -1.0f,  1.0f, 0, 1, 0.0f + du, 0.0f + dv };
    v[2] = { 1.0f, -1.0f, 0, 1, 1.0f - du, 1.0f - dv };
    v[3] = { 1.0f,  1.0f, 0, 1, 1.0f - du, 0.0f + dv };

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetVertexDeclaration(g_pQuadDecl);

    UINT passes = 0;
    g_pEffectEnd->Begin(&passes, 0);
    g_pEffectEnd->BeginPass(0);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(QuadVertex));
    g_pEffectEnd->EndPass();
    g_pEffectEnd->End();

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);

    // 4) シーン終了
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

void Render::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 tex, const char* tech)
{
    Common::D3DDevice()->SetVertexShader(NULL);

    g_pEffect3->SetTechnique(tech);
    g_pEffect3->SetTexture("g_SrcTex", tex);

    float texelSize[2] = { 1.0f / m_windowSizeWidth, 1.0f / m_windowSizeHeight };
    g_pEffect3->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] = {
        {                    -0.5f,                     -0.5f, 0, 1, 0, 0 },
        { m_windowSizeWidth - 0.5f,                     -0.5f, 0, 1, 1, 0 },
        {                    -0.5f, m_windowSizeHeight - 0.5f, 0, 1, 0, 1 },
        { m_windowSizeWidth - 0.5f, m_windowSizeHeight - 0.5f, 0, 1, 1, 1 }
    };

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    g_pEffect3->Begin(NULL, 0);
    g_pEffect3->BeginPass(0);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
    g_pEffect3->EndPass();
    g_pEffect3->End();
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}


}
