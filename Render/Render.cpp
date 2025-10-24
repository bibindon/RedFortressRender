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
#include <chrono>
#include <set>
#include <algorithm>

namespace NSRender
{

void Render::Initialize(HWND hWnd)
{
    HRESULT hResult = E_FAIL;

    m_hWnd = hWnd;

    m_windowManager.Initialize(hWnd);

    m_sprite.Initialize();

    CreateTexture();

    m_GBuffer.Initialize();

    // 深度バッファシャドウ
    m_postEffectZShadow.Initialize();

    // SSAO
    m_postEffectSSAO.Initialize();

    // 彩度フィルター
    m_postEffectSaturate.Initialize();

    // ガウスフィルター
    m_postEffectGauss.Initialize();

    // ブルーム
    m_PostEffectBloom.Initialize();

    // スターバースト
    m_postEffectStarBurst.Initialize();

    // 最終処理用ポストエフェクト
    m_postEffectEnd.Initialize();

    Common::AddDeviceLostResource(this);
}

void Render::Finalize()
{
    Common::D3DDevice()->Release();
    Common::SetD3DDevice(NULL);
}

void Render::Draw()
{
    HRESULT hResult = E_FAIL;

    if (m_bShowFPS)
    {
        float fps = CalcFPS();
        ShowFPS(fps);
    }

    DrawPass1();

    //---------------------------------------------------------------
    // ポストエフェクトのために深度画像とワールド座標画像を作成
    //---------------------------------------------------------------
    LPDIRECT3DTEXTURE9 pTexTempZ = NULL;
    LPDIRECT3DTEXTURE9 pTexTempPos = NULL;
    m_GBuffer.Draw(m_meshMixList, &pTexTempZ, &pTexTempPos);

    //---------------------------------------------------------------
    // ポストエフェクト
    // m_pRenderTarget1やm_pRenderTarget2に代入しないこと
    //---------------------------------------------------------------

    LPDIRECT3DTEXTURE9 pTempTexture = NULL;

    pTempTexture = m_pRenderTarget1;

    // 深度バッファシャドウ
    pTempTexture = m_postEffectZShadow.Draw(pTempTexture, m_meshMixList);

    // SSAO
    pTempTexture = m_postEffectSSAO.Draw(pTempTexture, pTexTempZ, pTexTempPos);

    // 彩度変更
    pTempTexture = m_postEffectSaturate.Draw(pTempTexture);

    // TODO 被写界深度

    // ブルーム
    pTempTexture = m_PostEffectBloom.Draw(pTempTexture);

    // スターバースト
    pTempTexture = m_postEffectStarBurst.Draw(pTempTexture);

    // ガウス
    pTempTexture = m_postEffectGauss.Draw(pTempTexture);

    // g_pRenderTargetの内容を画面に転送
    m_postEffectEnd.Draw(pTempTexture);

    // 文字と画像は彩度フィルタの影響を受けないようにする
    Draw2D();

    hResult = Common::D3DDevice()->Present(NULL, NULL, NULL, NULL);
    assert(hResult == S_OK);

    m_windowManager.ChangeWindowMode();

}

void Render::ChangeResolution(const int W, const int H)
{
    m_windowManager.ChangeResolution(W, H);
}

void Render::ChangeWindowMode(const eWindowMode eWindowMode_)
{
    m_windowManager.RequestWindowMode(eWindowMode_);
}

int Render::AddMesh(const std::wstring& filePath,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rot,
                    const float scale,
                    const float radius)
{
    m_meshList.push_back(Mesh(filePath, pos, rot, scale, radius));
    m_meshList.rbegin()->Initialize();

    return (int)m_meshList.size() - 1;
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

void Render::AddMeshSSS(const std::wstring& filePath,
                        const D3DXVECTOR3& pos,
                        const D3DXVECTOR3& rot,
                        const float scale,
                        const float radius)
{
    auto mesh = MeshSSS(filePath, pos, rot, scale, radius);
    m_meshSSSList.push_back(mesh);
    m_meshSSSList.rbegin()->Initialize();
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

void Render::AddMeshPOM(const std::wstring& filePath,
                        const D3DXVECTOR3& pos,
                        const D3DXVECTOR3& rot,
                        const float scale,
                        const float radius)
{
    MeshPOM mesh;
    m_meshPOMList.push_back(mesh);
    m_meshPOMList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
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

int Render::AddMeshMix(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius)
{
    auto param = GetMeshParamPreset(eMeshParamPreset::GRASS);
    param.smooth = false;
    auto mesh = MeshMix(filePath, pos, rot, scale, param);
    m_meshMixList.push_back(mesh);
    m_meshMixList.rbegin()->Initialize();

    return (int)m_meshMixList.size() - 1;
}

void Render::SetMeshMixPos(const int id, const D3DXVECTOR3& pos)
{
    m_meshMixList.at(id).SetPos(pos);
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
    Font* font = NEW Font();
    font->Initialize(fontName, fontSize, fontColor);
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

    m_fontList.at(fontId)->AddText(text, X, Y);
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

    m_fontList.at(fontId)->AddText(text, X, Y, color);
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

    m_fontList.at(fontId)->AddTextCenter(text, X, Y, Width, Height);
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

    m_fontList.at(fontId)->AddTextCenter(text, X, Y, Width, Height, color);
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

void Render::SetPostEffectBloom(const bool arg)
{
    m_PostEffectBloom.SetEnable(arg);
}

void Render::SetPostEffectStarBurst(const bool arg)
{
    m_postEffectStarBurst.SetEnable(arg);
}

void Render::SetShowFPS(const bool arg)
{
    m_bShowFPS = arg;
}

std::vector<std::pair<int, int>> Render::GetResolutionList()
{
    return m_windowManager.GetResolutionList();
}

void Render::SetLightDir(const D3DXVECTOR3& dir)
{
    D3DXVECTOR4 normal(dir, 0.f);
    D3DXVec4Normalize(&normal, &normal);
    Light::SetLightDir(normal);
}

void Render::AddPointLight(const D3DXVECTOR3& pos, const float brightness, const D3DXCOLOR color)
{
    Light::AddPointLight(pos, color, brightness);
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

    // 水平方向
    float yaw = atan2f(rel.x, rel.z);

    // 上下方向
    float pitch = asinf(rel.y / r);

    // 回転を加える
    yaw += rot.y;
    pitch += rot.x;

    //---------------------------------------------------------
    // ピッチ角を制限する
    //---------------------------------------------------------

    // 真上/真下を少し手前で止める
    const float limit = D3DXToRadian(89.0f);
    if (pitch > limit)
    {
        pitch = limit;
    }

    if (pitch < -limit)
    {
        pitch = -limit;
    }

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

void Render::DrawPass1()
{
    HRESULT hResult = E_FAIL;

    LPDIRECT3DSURFACE9 surfaceOld = NULL;
    hResult = Common::D3DDevice()->GetRenderTarget(0, &surfaceOld);
    assert(hResult == S_OK);

    LPDIRECT3DSURFACE9 surfaceRenderTarget0 = NULL;
    LPDIRECT3DSURFACE9 surfaceRenderTarget1 = NULL;

    hResult = m_pRenderTarget1->GetSurfaceLevel(0, &surfaceRenderTarget0);
    assert(hResult == S_OK);

    hResult = m_pRenderTarget2->GetSurfaceLevel(0, &surfaceRenderTarget1);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderTarget(0, surfaceRenderTarget0);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderTarget(1, surfaceRenderTarget1);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->Clear(0,
                                         NULL,
                                         D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                         D3DCOLOR_RGBA(200, 200, 200, 255),
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

    for (auto& elem : m_meshSSSList)
    {
        elem.Render();
    }

    for (auto& elem : m_meshPointLightList)
    {
        elem.Draw();
    }

    for (auto& elem : m_meshNormalMapList)
    {
        elem.Draw();
    }

    for (auto& elem : m_meshPOMList)
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
                     Light::GetLightDir(),
                     Light::GetBrightness());
    }

    for (auto& elem : m_meshInstancingMap)
    {
        elem.second->Draw();
    }

    for (auto& elem : m_meshMixList)
    {
        elem.Render();
    }

    hResult = Common::D3DDevice()->EndScene();
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderTarget(1, NULL);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderTarget(0, surfaceOld);
    assert(hResult == S_OK);

    SAFE_RELEASE(surfaceRenderTarget0);
    SAFE_RELEASE(surfaceRenderTarget1);
    SAFE_RELEASE(surfaceOld);
}

float Render::CalcFPS()
{
    //--------------------------------------------------------
    // 毎フレーム、現在時刻を記録し、リストの末尾に追加する。
    // 先頭ほど古い時刻が記録される。
    // リストの先頭から、記録された時刻と現在時刻の差を比較していくと、
    // 最初は1秒以上の差があるが、やがて1秒以下の要素が見つかる。（Aとする）
    // そのときの、A以降の要素の総数がFPSである
    //--------------------------------------------------------

    using ClockType = std::chrono::steady_clock;

    const int timeRecordCapacity = 300;

    ClockType::time_point nowTime = ClockType::now();

    m_vecTime.push_back(nowTime);

    const int overflowCount = (int)m_vecTime.size() - timeRecordCapacity;
    if (overflowCount > 0)
    {
        m_vecTime.erase(m_vecTime.begin(), m_vecTime.begin() + overflowCount);
    }

    ClockType::time_point oneSecondAgo = nowTime - std::chrono::seconds(1);

    auto windowBegin = std::lower_bound(m_vecTime.begin(), m_vecTime.end(), oneSecondAgo);
    int framesInWindow = (int)std::distance(windowBegin, m_vecTime.end());

    if (framesInWindow <= 1)
    {
        return 0.0f;
    }

    double secondsSpan = std::chrono::duration<double>(m_vecTime.back() - *windowBegin).count();
    if (secondsSpan <= 0.0)
    {
        return 0.0f;
    }

    float fpsFloat = (float)((framesInWindow - 1) / secondsSpan);
    return fpsFloat;
}

void Render::ShowFPS(const float arg)
{
    if (m_fontID == -1)
    {
        m_fontID = SetUpFont(L"BIZ UDゴシック",
                             20,
                             D3DCOLOR_RGBA(0, 255, 0, 255));
    }

    wchar_t buffer[64];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f", arg);

    std::wstring fps(buffer);

    DrawText_(m_fontID, fps, 10, 10);
}

void Render::Draw2D()
{
    for (auto& elem : m_fontList)
    {
        elem->Draw();
    }

    m_sprite.Draw();

}

void Render::OnDeviceLost()
{
    SAFE_RELEASE(m_pRenderTarget1);
    SAFE_RELEASE(m_pRenderTarget2);
    m_sprite.OnDeviceLost();
}

void Render::OnDeviceReset()
{
    CreateTexture();
    m_sprite.OnDeviceReset();
}

void Render::CreateTexture()
{
    HRESULT hr = E_FAIL;

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A8R8G8B8,
                           D3DPOOL_DEFAULT,
                           &m_pRenderTarget1);
    assert(hr == S_OK);

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A8R8G8B8,
                           D3DPOOL_DEFAULT,
                           &m_pRenderTarget2);
    assert(hr == S_OK);

}

}
