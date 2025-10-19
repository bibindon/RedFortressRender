#include "MeshNormalMapping.h"

#include "Camera.h"
#include "Light.h"

namespace NSRender
{

void MeshNormalMapping::Initialize(const std::wstring& filename,
                                   const std::wstring& normalMap,
                                   const D3DXVECTOR3& pos,
                                   const D3DXVECTOR3& rot,
                                   const float scale,
                                   const float radius)
{
    HRESULT hResult = E_FAIL;

    m_pos = pos;
    m_rotate = rot;
    m_scale = scale;
    m_radius = radius;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       SHADER_FILENAME.c_str(),
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_D3DEffect,
                                       NULL);

    assert(hResult == S_OK);

    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;

    hResult = D3DXLoadMeshFromX(filename.c_str(),
                                D3DXMESH_MANAGED,
                                Common::D3DDevice(),
                                NULL,
                                &pD3DXMtrlBuffer,
                                NULL,
                                &m_materialCount,
                                &m_D3DMesh);

    assert(hResult == S_OK);

    {
        // なめらかなライティングのために法線情報を計算しなおす

        DWORD fvf = m_D3DMesh->GetFVF();
        if ((fvf & D3DFVF_NORMAL) == 0)
        {
            LPD3DXMESH meshWithN;
            m_D3DMesh->CloneMeshFVF(m_D3DMesh->GetOptions(), fvf | D3DFVF_NORMAL,
                                    Common::D3DDevice(),
                                    &meshWithN);

            m_D3DMesh->Release();
            m_D3DMesh = meshWithN;
        }

        std::vector<DWORD> adj(m_D3DMesh->GetNumFaces() * 3);

        // しきい値はモデルに合わせて
        m_D3DMesh->GenerateAdjacency(1e-6f, adj.data());

        HRESULT hr = D3DXComputeNormals(m_D3DMesh, adj.data());
    }

    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    m_materialList.resize(m_materialCount);
    m_textureList.resize(m_materialCount);

    for (DWORD i = 0; i < m_materialCount; i++)
    {
        m_materialList[i] = d3dxMaterials[i].MatD3D;
        m_textureList[i] = NULL;

        std::string pTexPath(d3dxMaterials[i].pTextureFilename);

        if (!pTexPath.empty())
        {
            {
                int len = MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, nullptr, 0);
                std::wstring pTexPathW(len, 0);
                MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, &pTexPathW[0], len);

                hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                    pTexPathW.c_str(),
                                                    &m_textureList[i]);

                assert(hResult == S_OK);
            }
        }
    }

    hResult = pD3DXMtrlBuffer->Release();
    assert(hResult == S_OK);

    // ここでメッシュに TANGENT / BINORMAL を追加・生成
    EnsureMeshHasTangentBinormal(m_D3DMesh);

    hResult = D3DXCreateTextureFromFile(Common::D3DDevice(), normalMap.c_str(), &g_pNormalTex);
    assert(SUCCEEDED(hResult));


}


void MeshNormalMapping::EnsureMeshHasTangentBinormal(LPD3DXMESH& pMesh)
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
    HRESULT hr = pMesh->CloneMesh(D3DXMESH_MANAGED, declTB, Common::D3DDevice(), &pCloned);

    if (FAILED(hr))
    {
        assert(false);
        return;
    }

    // 隣接情報
    std::vector<DWORD> adj(pCloned->GetNumFaces() * 3);
    hr = pCloned->GenerateAdjacency(1e-6f, adj.data());

    if (FAILED(hr))
    {
        assert(false);
        SAFE_RELEASE(pCloned);
        return;
    }

    // オプション（必要に応じて調整）
    DWORD options = D3DXTANGENT_CALCULATE_NORMALS |
                    D3DXTANGENT_WEIGHT_BY_AREA |
                    D3DXTANGENT_GENERATE_IN_PLACE;  // 入力メッシュを書き換える

    // 正しいシグネチャ順で 16 引数を渡す
    hr = D3DXComputeTangentFrameEx(pCloned,                   // pMesh
                                   D3DDECLUSAGE_TEXCOORD, 0,  // どのUVを使うか（ここでは TEXCOORD0）
                                   D3DDECLUSAGE_TANGENT,  0,  // U偏微分の出力先 → TANGENT0
                                   D3DDECLUSAGE_BINORMAL, 0,  // V偏微分の出力先 → BINORMAL0
                                   D3DDECLUSAGE_NORMAL,   0,  // 法線の出力先   → NORMAL0（再計算）
                                   options,                   // dwOptions
                                   adj.data(),                // 隣接
                                   0.01f,                     // fPartialEdgeThreshold
                                   0.25f,                     // fSingularPointThreshold
                                   0.01f,                     // fNormalEdgeThreshold
                                   NULL,                      // ppMeshOut（IN_PLACE 指定なので不要）
                                   NULL                       // ppVertexMapping（不要なら NULL）
    );

    if (FAILED(hr))
    {
        assert(false);
        SAFE_RELEASE(pCloned);
        return;
    }

    // 置き換え
    pMesh->Release();
    pMesh = pCloned;
}

void MeshNormalMapping::Finalize()
{
    SAFE_RELEASE(m_D3DEffect);

    for (auto& texture : m_textureList)
    {
        SAFE_RELEASE(texture);
    }

    SAFE_RELEASE(g_pNormalTex);
    SAFE_RELEASE(m_D3DMesh);
}

void MeshNormalMapping::Draw()
{
    // エフェクト定数

    D3DXMATRIX mat;
    D3DXMatrixIdentity(&mat);

    D3DXMatrixTranslation(&mat, m_pos.x, m_pos.y, m_pos.z);
    m_D3DEffect->SetMatrix("g_matWorld", &mat);

    mat *= Camera::GetViewMatrix();
    mat *= Camera::GetProjMatrix();
    m_D3DEffect->SetMatrix("g_matWorldViewProj", &mat);

    // 光の“進む方向”を設定（w=0）。Lambert では -方向 を使う。
    static float t2 = 0.0f;
    t2 += 0.02f;

    D3DXVECTOR4 lightDir(Light::GetLightDir());
    D3DXVec4Normalize(&lightDir, &lightDir);
    m_D3DEffect->SetVector("g_lightDirectionWS", &lightDir);

    // 法線マップ
    m_D3DEffect->SetTexture("g_normalMap", g_pNormalTex);

    // テクニック
    m_D3DEffect->SetTechnique("Technique_NormalMap_TBN");

    UINT nPass = 0;
    m_D3DEffect->Begin(&nPass, 0);
    m_D3DEffect->BeginPass(0);

    // サブセットごとにアルベドをセットして描画
    for (DWORD i = 0; i < m_materialCount; ++i)
    {
        m_D3DEffect->SetTexture("g_colorMap", m_textureList[i]);
        m_D3DEffect->CommitChanges();
        m_D3DMesh->DrawSubset(i);
    }

    m_D3DEffect->EndPass();
    m_D3DEffect->End();

}

void MeshNormalMapping::OnDeviceLost()
{
    auto result = m_D3DEffect->OnLostDevice();
    assert(result == S_OK);
}

void MeshNormalMapping::OnDeviceReset()
{
    auto result = m_D3DEffect->OnResetDevice();
    assert(result == S_OK);
}

}
