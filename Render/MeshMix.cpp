#include "MeshMix.h"

#include "Util.h"
#include "Camera.h"
#include "Light.h"

namespace NSRender
{

MeshMix::MeshMix(const std::wstring& xFilename,
                 const D3DXVECTOR3& position,
                 const D3DXVECTOR3& rotation,
                 const float scale,
                 const stMeshParam& param)
    : m_meshName(xFilename)
    , m_pos(position)
    , m_rotate(rotation)
    , m_scale(scale)
    , m_param(param)
{
}

void MeshMix::Initialize()
{
    HRESULT hResult = E_FAIL;

    auto tempPath = Util::GetExeDir() + SHADER_FILENAME.c_str();

    //--------------------------------------------------------
    // エフェクトの作成
    //--------------------------------------------------------
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       tempPath.c_str(),
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
                                D3DXMESH_MANAGED,
                                Common::D3DDevice(),
                                &adjacencyBuffer,
                                &materialBuffer,
                                nullptr,
                                &m_materialCount,
                                &m_D3DMesh);

    assert(hResult == S_OK);

    // なめらかなライティングのために法線情報を計算しなおす
    // 例えば半球の法線を再計算するとキノコのようになり、
    // 再計算しないとダイヤモンドのような見た目になる。

    DWORD* adjacencyList = (DWORD*)adjacencyBuffer->GetBufferPointer();
    if (m_param.smooth)
    {
        // ModifyMeshForNormalMapping関数でやるから必要ないはず
    //    HRESULT hr = D3DXComputeNormals(m_D3DMesh, adjacencyList);
    }

    //--------------------------------------------------------
    // UV情報を生成
    //--------------------------------------------------------
    ModifyMeshForNormalMapping(m_D3DMesh);

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
        if (i == 0)
        {
            //--------------------------------------------------------
            // 拡散反射色の読み込み
            // Diffuseの色は(1, 1, 1)が基本とする。
            //--------------------------------------------------------
            D3DXVECTOR4 diffuce(1.0f, 1.0f, 1.0f, 1.0f);

            diffuce.x = materialList[i].MatD3D.Diffuse.r;
            diffuce.y = materialList[i].MatD3D.Diffuse.g;
            diffuce.z = materialList[i].MatD3D.Diffuse.b;
            diffuce.w = materialList[i].MatD3D.Diffuse.a;

            m_vecDiffuse.push_back(diffuce);
        }

        if (materialList[i].pTextureFilename != nullptr &&
            strlen(materialList[i].pTextureFilename) != 0)
        {
            LPDIRECT3DTEXTURE9 tempTexture = NULL;

            std::wstring texturePath = xFileDir;
            texturePath += Util::Utf8ToWstring(materialList[i].pTextureFilename);
            hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                texturePath.c_str(),
                                                &tempTexture);

            assert(hResult == S_OK);

            m_vecTexture.push_back(tempTexture);
        }
    }

    SAFE_RELEASE(materialBuffer);

    Common::AddDeviceLostResource(this);

    m_bLoaded = true;
}

void MeshMix::ModifyMeshForNormalMapping(LPD3DXMESH& pMesh)
{
    // 目標の頂点宣言（POSITION, NORMAL, TEXCOORD0, TANGENT0, BINORMAL0）
    D3DVERTEXELEMENT9 declTB[] =
    {
        {0,  0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0},
        {0, 12,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,    0},
        {0, 24,  D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0},
        {0, 32,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,   0},
        {0, 44,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,  0},
        D3DDECL_END()
    };

    LPD3DXMESH pCloned = NULL;
    HRESULT hr = pMesh->CloneMesh(D3DXMESH_MANAGED | D3DXMESH_32BIT,
                                  declTB,
                                  Common::D3DDevice(),
                                  &pCloned);

    assert(SUCCEEDED(hr));

    // 隣接情報
    std::vector<DWORD> adj(pCloned->GetNumFaces() * 3);
    hr = pCloned->GenerateAdjacency(1e-6f, adj.data());

    assert(SUCCEEDED(hr));

    // オプション（必要に応じて調整）
    DWORD options = D3DXTANGENT_WEIGHT_BY_AREA |
                    D3DXTANGENT_GENERATE_IN_PLACE;  // 入力メッシュを書き換える

    if (m_param.smooth)
    {
        options += D3DXTANGENT_CALCULATE_NORMALS;
    }

    // 正しいシグネチャ順で 16 引数を渡す
    hr = D3DXComputeTangentFrameEx(pCloned,                   // pMesh
                                   D3DDECLUSAGE_TEXCOORD, 0,  // どのUVを使うか（ここでは TEXCOORD0）
                                   D3DDECLUSAGE_TANGENT,  0,  // U偏微分の出力先 → TANGENT0
                                   D3DDECLUSAGE_BINORMAL, 0,  // V偏微分の出力先 → BINORMAL0
                                   D3DDECLUSAGE_NORMAL,   0,  // 法線の出力先   → NORMAL0（再計算）
                                   options,                   // dwOptions
                                   adj.data(),                // 隣接
                                   0.01f,                     // fPartialEdgeThreshold
                                   0.01f,                     // fSingularPointThreshold
                                   // 角の平滑化を防止。平面に対してだけ平滑化を行うようにする
                                   // これをしないと視差遮蔽マッピングで問題になる。
                                   0.999f,                     // fNormalEdgeThreshold
                                   NULL,                      // ppMeshOut（IN_PLACE 指定なので不要）
                                   NULL                       // ppVertexMapping（不要なら NULL）
    );

    // 原因不明
    if (FAILED(hr))
    {
        return;
    }

    // UV情報がないメッシュファイルを読み込むと、ここでエラー
    assert(SUCCEEDED(hr));

    pMesh->Release();
    pMesh = pCloned;
}

void MeshMix::SetPos(const D3DXVECTOR3& pos)
{
    m_pos = pos;
}

void MeshMix::SetRotY(const float rotY)
{
    m_rotate.y = rotY;
}

D3DXVECTOR3 MeshMix::GetRot() const
{
    return m_rotate;
}

D3DXVECTOR3 MeshMix::GetPos() const
{
    return m_pos;
}

float MeshMix::GetScale() const
{
    return m_scale;
}

void MeshMix::Render()
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
    // TODO LightNormalとLightDirでは方向が逆になる。間違っている
    //--------------------------------------------------------
    D3DXVECTOR4 normal = Light::GetLightDir();

    // 文字列で指定すると遅くなるらしい
    hResult = m_D3DEffect->SetVector("g_lightDir", &normal);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // ワールド変換行列を設定
    //--------------------------------------------------------
    D3DXMATRIX matWorld{ };
    D3DXMatrixIdentity(&matWorld);

    D3DXMATRIX matWork;
    D3DXMatrixIdentity(&matWork);

    D3DXMatrixScaling(&matWork, m_scale, m_scale, m_scale);
    matWorld *= matWork;

    D3DXMatrixRotationYawPitchRoll(&matWork, m_rotate.y, m_rotate.x, m_rotate.z);
    matWorld *= matWork;

    D3DXMatrixTranslation(&matWork, m_pos.x, m_pos.y, m_pos.z);
    matWorld *= matWork;

    hResult = m_D3DEffect->SetMatrix("g_matWorld", &matWorld);

    //--------------------------------------------------------
    // ワールドビュー射影変換行列を設定
    //--------------------------------------------------------
    D3DXMATRIX matViewProj{ };
    D3DXMatrixIdentity(&matViewProj);

    matViewProj *= Camera::GetViewMatrix();
    matViewProj *= Camera::GetProjMatrix();

    hResult = m_D3DEffect->SetMatrix("g_matViewProj", &matViewProj);

    D3DXMATRIX worldViewProjMatrix { };
    D3DXMatrixIdentity(&worldViewProjMatrix);

    worldViewProjMatrix = matWorld * matViewProj;

    hResult = m_D3DEffect->SetMatrix("g_matWorldViewProj", &worldViewProjMatrix);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // カメラ
    //--------------------------------------------------------
    D3DXVECTOR4 cameraPos = D3DXVECTOR4(Camera::GetEyePos(), 1.f);

    hResult = m_D3DEffect->SetVector("g_cameraPos", &cameraPos);
    assert(hResult == S_OK);

    const float screenSize[2] =
    {
        static_cast<float>(Common::ScreenW()),
        static_cast<float>(Common::ScreenH())
    };
    hResult = m_D3DEffect->SetFloatArray("g_screenSize", screenSize, 2);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // 描画開始
    //--------------------------------------------------------
    hResult = m_D3DEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    hResult = m_D3DEffect->Begin(nullptr, 0);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // マテリアルの数だけ色とテクスチャを設定して描画
    //--------------------------------------------------------
    hResult = m_D3DEffect->SetVector("g_diffuse", &m_vecDiffuse.at(0));
    assert(hResult == S_OK);

    hResult = m_D3DEffect->SetTexture("g_texture", m_vecTexture.at(0));
    assert(hResult == S_OK);

    hResult = m_D3DEffect->SetTexture("g_texCubeMap", m_vecTexture.at(1));
    assert(hResult == S_OK);

    hResult = m_D3DEffect->SetTexture("g_texNormalMap", m_vecTexture.at(2));
    assert(hResult == S_OK);

    // 高さマップがないときはある。あっても問題ない
    if (m_vecTexture.size() >= 4)
    {
        hResult = m_D3DEffect->SetTexture("g_texHeightMap", m_vecTexture.at(3));
        assert(hResult == S_OK);
    }

    // 時間パラメータを設定
    static float f = 0.f;
    f += 0.001f;
    hResult = m_D3DEffect->SetFloat("g_time", f);
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // 揺らし効果
    // 画面を揺らす、という意味ではなく、モデルを草や水面のように揺らす効果
    //--------------------------------------------------------
    if (m_param.sway)
    {
        hResult = m_D3DEffect->SetBool("g_swayEnable", TRUE);

        // 揺らしの強度を設定
        hResult = m_D3DEffect->SetFloat("g_swayAmount", 2.5f);
        assert(hResult == S_OK);

        // 揺らしの速度を設定
        hResult = m_D3DEffect->SetFloat("g_swaySpeed", 1.0f);
        assert(hResult == S_OK);
    }

    //--------------------------------------------------------
    // ポイントライト
    //--------------------------------------------------------
    if (m_param.pointLight)
    {
        auto pointLightList = Light::GetPointLightList();

        D3DXVECTOR4 pos[16];
        float brightness[16] { };
        D3DXVECTOR4 color[16];

        ZeroMemory(pos, sizeof(pos));
        ZeroMemory(color, sizeof(color));

        for (int i = 0; i < 16; ++i)
        {
            if (i < pointLightList.size())
            {
                pos[i].x = pointLightList.at(i).m_pos.x;
                pos[i].y = pointLightList.at(i).m_pos.y;
                pos[i].z = pointLightList.at(i).m_pos.z;
                brightness[i] = pointLightList.at(i).m_brightness;
                color[i].x = pointLightList.at(i).m_color.r;
                color[i].y = pointLightList.at(i).m_color.g;
                color[i].z = pointLightList.at(i).m_color.b;
            }
        }
        
        hResult = m_D3DEffect->SetVectorArray("g_pointLightPos", pos, 16);
        assert(hResult == S_OK);

        hResult = m_D3DEffect->SetFloatArray("g_pointLightBrightness", brightness, 16);
        assert(hResult == S_OK);

        hResult = m_D3DEffect->SetVectorArray("g_pointLightColor", color, 16);
        assert(hResult == S_OK);

    }

    hResult = m_D3DEffect->CommitChanges();
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // パス0
    // 通常の描画
    // 法線マッピングを含む
    //--------------------------------------------------------
    hResult = m_D3DEffect->BeginPass(0);
    assert(hResult == S_OK);

    hResult = m_D3DMesh->DrawSubset(0);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->EndPass();
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // パス1
    // 環境マッピング
    //--------------------------------------------------------
    hResult = m_D3DEffect->BeginPass(1);
    assert(hResult == S_OK);

    hResult = m_D3DMesh->DrawSubset(0);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->EndPass();
    assert(hResult == S_OK);

    //--------------------------------------------------------
    // パス2
    // ガラスエフェクト
    //--------------------------------------------------------
    if (m_param.glass)
    {
        hResult = m_D3DEffect->BeginPass(2);
        assert(hResult == S_OK);

        hResult = m_D3DMesh->DrawSubset(0);
        assert(hResult == S_OK);

        hResult = m_D3DEffect->EndPass();
        assert(hResult == S_OK);
    }

    //--------------------------------------------------------
    // パス3
    // ポイントライト
    //--------------------------------------------------------
    hResult = m_D3DEffect->BeginPass(3);
    assert(hResult == S_OK);

    hResult = m_D3DMesh->DrawSubset(0);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->EndPass();
    assert(hResult == S_OK);

    hResult = m_D3DEffect->End();
    assert(hResult == S_OK);
}

LPD3DXMESH MeshMix::GetD3DMesh() const
{
    return m_D3DMesh;
}

float MeshMix::GetRadius() const
{
    return m_param.collisionRadius;
}

std::wstring MeshMix::GetMeshName()
{
    return m_meshName;
}

void MeshMix::OnDeviceLost()
{
    HRESULT hr = m_D3DEffect->OnLostDevice();
    assert(hr == S_OK);
}

void MeshMix::OnDeviceReset()
{
    HRESULT hr = m_D3DEffect->OnResetDevice();
    assert(hr == S_OK);
}

stMeshParam GetMeshParamPreset(const eMeshParamPreset preset)
{
    stMeshParam param;

    // TODO 引数をもとにパラメータにプリセットをセットする

    return param;
}

}
