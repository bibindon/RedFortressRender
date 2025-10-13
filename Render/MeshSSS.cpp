#include "MeshSSS.h"
#include "MeshSSS.h"

#include <cassert>
#include <cmath>

#include "Common.h"
#include "Util.h"
#include "Camera.h"
#include "Light.h"

namespace NSRender
{

MeshSSS::MeshSSS(const std::wstring& xFilename,
           const D3DXVECTOR3& position,
           const D3DXVECTOR3& rotation,
           const float scale,
           const float radius)
    : m_meshName(xFilename)
    , m_pos(position)
    , m_rotate(rotation)
    , m_scale(scale)
    , m_radius(radius)
{
}

// シェーダーファイルを指定できるコンストラクタ
MeshSSS::MeshSSS(const std::wstring& shaderName,
           const std::wstring& xFilename,
           const D3DXVECTOR3& position,
           const D3DXVECTOR3& rotation,
           const float scale,
           const float radius)
    : SHADER_FILENAME(shaderName)
    , m_meshName(xFilename)
    , m_pos(position)
    , m_rotate(rotation)
    , m_scale(scale)
    , m_radius(radius)
{
}

MeshSSS::~MeshSSS()
{
}

static LPDIRECT3DSURFACE9& OffscreenDepthStorage()
{
    static LPDIRECT3DSURFACE9 s_depth = NULL;
    return s_depth;
}

LPDIRECT3DSURFACE9 MeshSSS::AcquireOffscreenDepth(int width, int height)
{
    LPDIRECT3DSURFACE9& depthSurf = OffscreenDepthStorage();

    if (depthSurf != NULL)
    {
        D3DSURFACE_DESC desc;
        depthSurf->GetDesc(&desc);

        if (static_cast<int>(desc.Width) != width || static_cast<int>(desc.Height) != height)
        {
            SAFE_RELEASE(depthSurf);
        }
    }

    if (depthSurf == NULL)
    {
        HRESULT hr = Common::D3DDevice()->CreateDepthStencilSurface(width,
                                                                    height,
                                                                    D3DFMT_D24S8,
                                                                    D3DMULTISAMPLE_NONE,
                                                                    0,
                                                                    TRUE,
                                                                    &depthSurf,
                                                                    NULL);
        assert(SUCCEEDED(hr));
    }

    return depthSurf;
}

void MeshSSS::ReleaseOffscreenDepth()
{
    LPDIRECT3DSURFACE9& depthSurf = OffscreenDepthStorage();
    SAFE_RELEASE(depthSurf);
}

void MeshSSS::Initialize()
{
    HRESULT hResult = E_FAIL;

    //--------------------------------------------------------
    // エフェクトの作成
    //--------------------------------------------------------
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       SHADER_FILENAME.c_str(),
                                       nullptr,
                                       nullptr,
                                       D3DXSHADER_OPTIMIZATION_LEVEL3,
                                       nullptr,
                                       &m_D3DEffect,
                                       nullptr);

    assert(hResult == S_OK);

    //--------------------------------------------------------
    // Xファイルの読み込み
    //--------------------------------------------------------
    LPD3DXBUFFER adjacencyBuffer = nullptr;
    LPD3DXBUFFER materialBuffer = nullptr;

    hResult = D3DXLoadMeshFromX(m_meshName.c_str(),
                                D3DXMESH_SYSTEMMEM,
                                Common::D3DDevice(),
                                &adjacencyBuffer,
                                &materialBuffer,
                                nullptr,
                                &m_materialCount,
                                &m_D3DMesh);

    assert(hResult == S_OK);

    //--------------------------------------------------------
    // 法線情報をもつメッシュファイルに変換
    //--------------------------------------------------------
    D3DVERTEXELEMENT9 decl[4] { };

    {
        decl[0].Stream = 0;
        decl[0].Offset = 0;
        decl[0].Type = D3DDECLTYPE_FLOAT3;
        decl[0].Method = D3DDECLMETHOD_DEFAULT;
        decl[0].Usage = D3DDECLUSAGE_POSITION;
        decl[0].UsageIndex = 0;

        decl[1].Stream = 0;
        decl[1].Offset = 12;
        decl[1].Type = D3DDECLTYPE_FLOAT3;
        decl[1].Method = D3DDECLMETHOD_DEFAULT;
        decl[1].Usage = D3DDECLUSAGE_NORMAL;
        decl[1].UsageIndex = 0;

        decl[2].Stream = 0;
        decl[2].Offset = 24;
        decl[2].Type = D3DDECLTYPE_FLOAT2;
        decl[2].Method = D3DDECLMETHOD_DEFAULT;
        decl[2].Usage = D3DDECLUSAGE_TEXCOORD;
        decl[2].UsageIndex = 0;

        decl[3] = D3DDECL_END();
    }


    LPD3DXMESH tempMesh = nullptr;
    hResult = m_D3DMesh->CloneMesh(D3DXMESH_MANAGED | D3DXMESH_32BIT,
                                   decl,
                                   Common::D3DDevice(),
                                   &tempMesh);

    assert(hResult == S_OK);

    m_D3DMesh->Release();
    m_D3DMesh = tempMesh;

    DWORD* adjacencyList = (DWORD*)adjacencyBuffer->GetBufferPointer();

    //--------------------------------------------------------
    // 法線情報を再計算
    //--------------------------------------------------------
    hResult = D3DXComputeNormals(m_D3DMesh, adjacencyList);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // 面と頂点を並べ替えてメッシュを生成し、描画パフォーマンスを最適化
    //--------------------------------------------------------
    hResult = m_D3DMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE,
                                         adjacencyList,
                                         nullptr,
                                         nullptr,
                                         nullptr);

    SAFE_RELEASE(adjacencyBuffer);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // マテリアル情報の読み込み
    //--------------------------------------------------------
    D3DXMATERIAL* materialList = (D3DXMATERIAL*)materialBuffer->GetBufferPointer();

    // Xファイルのディレクトリ
    std::wstring xFileDir = m_meshName;
    std::size_t lastPos = xFileDir.find_last_of(L"\\");
    xFileDir = xFileDir.substr(0, lastPos + 1);

    for (DWORD i = 0; i < m_materialCount; ++i)
    {
        //--------------------------------------------------------
        // 拡散反射色の読み込み
        //--------------------------------------------------------
        D3DXVECTOR4 diffuce(1.0f, 1.0f, 1.0f, 1.0f);

        if (true)
        {
            diffuce.x = materialList[i].MatD3D.Diffuse.r;
            diffuce.y = materialList[i].MatD3D.Diffuse.g;
            diffuce.z = materialList[i].MatD3D.Diffuse.b;
            diffuce.w = materialList[i].MatD3D.Diffuse.a;
        }

        m_vecDiffuse.push_back(diffuce);

        //--------------------------------------------------------
        // テクスチャの読み込み
        //--------------------------------------------------------
        if (materialList[i].pTextureFilename != nullptr &&
            strlen(materialList[i].pTextureFilename) != 0)
        {
            std::wstring texturePath = xFileDir;
            texturePath += Util::Utf8ToWstring(materialList[i].pTextureFilename);
            LPDIRECT3DTEXTURE9 tempTexture = nullptr;
            hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                texturePath.c_str(),
                                                &tempTexture);

            assert(hResult == S_OK);

            m_vecTexture.push_back(tempTexture);
        }
    }

    SAFE_RELEASE(materialBuffer);

    HRESULT hr = D3DXCreateTexture(Common::D3DDevice(),
                                   1600,
                                   900,
                                   1,
                                   D3DUSAGE_RENDERTARGET,
                                   D3DFMT_R32F,
                                   D3DPOOL_DEFAULT,
                                   &g_rtFrontDepth);

    assert(SUCCEEDED(hr));

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           1600,
                           900,
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_R32F,
                           D3DPOOL_DEFAULT,
                           &g_rtBackDepth);

    assert(SUCCEEDED(hr));


    m_bLoaded = true;
}

void MeshSSS::SetPos(const D3DXVECTOR3& pos)
{
    m_pos = pos;
}

void MeshSSS::SetRotY(const float rotY)
{
    m_rotate.y = rotY;
}

D3DXVECTOR3 MeshSSS::GetPos() const
{
    return m_pos;
}

float MeshSSS::GetScale() const
{
    return m_scale;
}

/*
void MeshSSS::Render()
{
    HRESULT hResult = E_FAIL;

    //--------------------------------------------------------
    // 初期化が終わっていないなら描画しない
    // （別スレッドで初期化を行う場合を考慮）
    //--------------------------------------------------------
    if (m_bLoaded == false)
    {
        return;
    }

    //--------------------------------------------------------
    // 光源の方向を設定
    //--------------------------------------------------------
    D3DXVECTOR4 normal = Light::GetLightNormal();

    float work = m_rotate.y * -1.f;
    normal.x = std::sin(work + D3DX_PI);
    normal.z = std::cos(work + D3DX_PI);
    D3DXVec4Normalize(&normal, &normal);

    hResult = m_D3DEffect->SetVector("g_lightNormal", &normal);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // ワールド変換行列を設定
    //--------------------------------------------------------
    D3DXMATRIX worldViewProjMatrix { };
    D3DXMatrixIdentity(&worldViewProjMatrix);

    {
        D3DXMATRIX mat;

        D3DXMatrixScaling(&mat, m_scale, m_scale, m_scale);
        worldViewProjMatrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, m_rotate.y, m_rotate.x, m_rotate.z);
        worldViewProjMatrix *= mat;

        D3DXMatrixTranslation(&mat, m_pos.x, m_pos.y, m_pos.z);
        worldViewProjMatrix *= mat;
    }

    //    hResult = m_D3DEffect->SetMatrix("g_matWorld", &worldViewProjMatrix);
    //    assert(hResult == S_OK);

    //--------------------------------------------------------
    // カメラの位置を設定
    //--------------------------------------------------------
    D3DXVECTOR4 cameraPos { };
    cameraPos.x = Camera::GetEyePos().x;
    cameraPos.y = Camera::GetEyePos().y;
    cameraPos.z = Camera::GetEyePos().z;
    cameraPos.w = 0.f;

    //    hResult = m_D3DEffect->SetVector("g_vecCameraPos", &cameraPos);
    //    assert(hResult == S_OK);

    //--------------------------------------------------------
    // ワールドビュー射影変換行列を設定
    //--------------------------------------------------------
    worldViewProjMatrix *= Camera::GetViewMatrix();
    worldViewProjMatrix *= Camera::GetProjMatrix();

    hResult = m_D3DEffect->SetMatrix("g_matWorldViewProj", &worldViewProjMatrix);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // 描画開始
    //--------------------------------------------------------
    hResult = m_D3DEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    hResult = m_D3DEffect->Begin(nullptr, 0);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->BeginPass(0);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // マテリアルの数だけ色とテクスチャを設定して描画
    //--------------------------------------------------------
    for (DWORD i = 0; i < m_materialCount; ++i)
    {
        //        hResult = m_D3DEffect->SetVector("g_vecDiffuse", &m_vecDiffuse.at(i));
        //        assert(hResult == S_OK);

        if (i < m_vecTexture.size())
        {
            hResult = m_D3DEffect->SetTexture("g_texture", m_vecTexture.at(i));
            assert(hResult == S_OK);
        }

        hResult = m_D3DEffect->CommitChanges();
        assert(hResult == S_OK);

        hResult = m_D3DMesh->DrawSubset(i);
        assert(hResult == S_OK);
    }

    hResult = m_D3DEffect->EndPass();
    assert(hResult == S_OK);

    hResult = m_D3DEffect->End();
    assert(hResult == S_OK);
}
*/

void MeshSSS::Render()
{
    static float timeSeconds = 0.0f;
    timeSeconds += 0.01f;

    D3DXVECTOR3 eyePosition(8.0f * sinf(timeSeconds), 5.0f, -8.0f * cosf(timeSeconds));
    D3DXVECTOR3 targetPosition(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 upVector(0.0f, 1.0f, 0.0f);

    D3DXMATRIX viewMatrix;
    D3DXMatrixLookAtLH(&viewMatrix, &eyePosition, &targetPosition, &upVector);

    D3DXMATRIX projMatrix;
    D3DXMatrixPerspectiveFovLH(&projMatrix,
                               D3DXToRadian(45.0f),
                               (float)1600 / 900,
                               0.5f,
                               1000.0f);

    D3DXMATRIX worldOpaque;
    D3DXMatrixTranslation(&worldOpaque, 0.0f, -2.0f, 0.0f);

    D3DXMATRIX worldFog;
    D3DXMatrixTranslation(&worldFog, 0.0f, 0.0f, 0.0f);

    D3DXVECTOR2 invSize(1.0f / 1600, 1.0f / 900);

    LPDIRECT3DSURFACE9 backBuffer = NULL;
    LPDIRECT3DSURFACE9 depthSurfaceScene = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &backBuffer);
    Common::D3DDevice()->GetDepthStencilSurface(&depthSurfaceScene);

    D3DVIEWPORT9 viewport;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = static_cast<DWORD>(1600);
    viewport.Height = static_cast<DWORD>(900);
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    Common::D3DDevice()->SetViewport(&viewport);

    // --------------------------------------------------------
    // Pass 1 : Opaque (Lambert)
    // --------------------------------------------------------
    Common::D3DDevice()->SetRenderTarget(0, backBuffer);
    Common::D3DDevice()->SetDepthStencilSurface(depthSurfaceScene);

    m_D3DEffect->SetTechnique("TechniquePass0");
    m_D3DEffect->SetMatrix("gView", &viewMatrix);
    m_D3DEffect->SetMatrix("gProj", &projMatrix);

    D3DXVECTOR4 lightDir(cosf(timeSeconds) * 0.6f, 0.7f, sinf(timeSeconds) * 0.6f, 0.0f);
    D3DXVECTOR4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
    D3DXVECTOR4 ambient(0.25f, 0.25f, 0.25f, 1.0f);

    m_D3DEffect->SetVector("gLightDirW", &lightDir);
    m_D3DEffect->SetVector("gLightColor", &lightColor);
    m_D3DEffect->SetVector("gAmbient", &ambient);

    UINT passCount = 0;
    m_D3DEffect->Begin(&passCount, 0);
    m_D3DEffect->BeginPass(0);

    m_D3DEffect->SetMatrix("gWorld", &worldOpaque);
    m_D3DEffect->CommitChanges();

    m_D3DEffect->SetTexture("g_texCube", m_vecTexture.at(0));
    m_D3DEffect->SetMatrix("gWorld", &worldFog);
    m_D3DEffect->CommitChanges();
    m_D3DMesh->DrawSubset(0);

    m_D3DEffect->EndPass();
    m_D3DEffect->End();

    Common::D3DDevice()->EndScene();

    // 共有のオフスクリーン用深度ステンシルを取得
    LPDIRECT3DSURFACE9 dsOffscreen = AcquireOffscreenDepth(1600, 900);

    // --------------------------------------------------------
    // Pass 2 : Fog Front Depth
    //   Surface は都度取得してローカルで保持・解放
    // --------------------------------------------------------
    LPDIRECT3DSURFACE9 surfFrontRT = NULL;
    g_rtFrontDepth->GetSurfaceLevel(0, &surfFrontRT);

    Common::D3DDevice()->SetRenderTarget(0, surfFrontRT);
    Common::D3DDevice()->SetDepthStencilSurface(dsOffscreen);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);

    Common::D3DDevice()->BeginScene();

    m_D3DEffect->SetTechnique("Technique_FrontDepth");
    m_D3DEffect->SetMatrix("gWorld", &worldFog);
    m_D3DEffect->SetMatrix("gView", &viewMatrix);
    m_D3DEffect->SetMatrix("gProj", &projMatrix);
    m_D3DEffect->SetVector("gInvTexSize", reinterpret_cast<D3DXVECTOR4*>(&invSize));

    m_D3DEffect->Begin(&passCount, 0);
    m_D3DEffect->BeginPass(0);
    m_D3DMesh->DrawSubset(0);
    m_D3DEffect->EndPass();
    m_D3DEffect->End();

    Common::D3DDevice()->EndScene();

    SAFE_RELEASE(surfFrontRT);

    // --------------------------------------------------------
    // Pass 3 : Fog Back Depth
    // --------------------------------------------------------
    LPDIRECT3DSURFACE9 surfBackRT = NULL;
    g_rtBackDepth->GetSurfaceLevel(0, &surfBackRT);

    Common::D3DDevice()->SetRenderTarget(0, surfBackRT);
    Common::D3DDevice()->SetDepthStencilSurface(dsOffscreen);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 0.0f, 0);

    Common::D3DDevice()->BeginScene();

    m_D3DEffect->SetTechnique("Technique_BackDepth");
    m_D3DEffect->SetMatrix("gWorld", &worldFog);
    m_D3DEffect->SetMatrix("gView", &viewMatrix);
    m_D3DEffect->SetMatrix("gProj", &projMatrix);
    m_D3DEffect->SetVector("gInvTexSize", reinterpret_cast<D3DXVECTOR4*>(&invSize));

    m_D3DEffect->Begin(&passCount, 0);
    m_D3DEffect->BeginPass(0);
    m_D3DMesh->DrawSubset(0);
    m_D3DEffect->EndPass();
    m_D3DEffect->End();

    Common::D3DDevice()->EndScene();

    SAFE_RELEASE(surfBackRT);

    // --------------------------------------------------------
    // Pass 4 : Fog Composite
    //   シーンの深度を使用するため、元の depthSurfaceScene に戻す
    // --------------------------------------------------------
    Common::D3DDevice()->SetRenderTarget(0, backBuffer);
    Common::D3DDevice()->SetDepthStencilSurface(depthSurfaceScene);

    Common::D3DDevice()->BeginScene();

    m_D3DEffect->SetTechnique("Technique_FogComposite");
    m_D3DEffect->SetMatrix("gWorld", &worldFog);
    m_D3DEffect->SetMatrix("gView", &viewMatrix);
    m_D3DEffect->SetMatrix("gProj", &projMatrix);
    m_D3DEffect->SetVector("gInvTexSize", reinterpret_cast<D3DXVECTOR4*>(&invSize));

    m_D3DEffect->SetTexture("gFrontDepthTex", g_rtFrontDepth);
    m_D3DEffect->SetTexture("gBackDepthTex", g_rtBackDepth);
    m_D3DEffect->SetTexture("g_texMonkey", m_vecTexture.at(0));

    m_D3DEffect->SetFloat("gSigmaT", 1.0f);
    m_D3DEffect->SetFloat("gSSSPow", 1.1f);
    m_D3DEffect->SetFloat("gTexStrength", 0.3f);

    D3DXVECTOR4 fogColor(0.8f, 1.0f, 0.2f, 1.0f);
    m_D3DEffect->SetVector("gFogColor", &fogColor);

    m_D3DEffect->Begin(&passCount, 0);
    m_D3DEffect->BeginPass(0);
    m_D3DMesh->DrawSubset(0);
    m_D3DEffect->EndPass();
    m_D3DEffect->End();

    SAFE_RELEASE(backBuffer);
    SAFE_RELEASE(depthSurfaceScene);
}


LPD3DXMESH MeshSSS::GetD3DMesh() const
{
    return m_D3DMesh;
}

float MeshSSS::GetRadius() const
{
    return m_radius;
}

std::wstring MeshSSS::GetMeshName()
{
    return m_meshName;
}

void MeshSSS::OnDeviceLost()
{
    HRESULT hr = m_D3DEffect->OnLostDevice();
    assert(hr == S_OK);
}

void MeshSSS::OnDeviceReset()
{
    HRESULT hr = m_D3DEffect->OnResetDevice();
    assert(hr == S_OK);
}

}

