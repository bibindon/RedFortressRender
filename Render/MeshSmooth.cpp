#include "MeshSmooth.h"

#include "Camera.h"

namespace NSRender
{

void MeshSmooth::Initialize(const std::wstring& filename,
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

}

void MeshSmooth::Finalize()
{
    SAFE_RELEASE(m_D3DEffect);

    for (auto& texture : m_textureList)
    {
        SAFE_RELEASE(texture);
    }

    SAFE_RELEASE(m_D3DMesh);
}

void MeshSmooth::Draw()
{
    HRESULT hResult = E_FAIL;

    static float f = 0.0f;
    f += 0.01f;

    D3DXMATRIX mat;
    D3DXMatrixIdentity(&mat);

    D3DXMatrixTranslation(&mat, m_pos.x, m_pos.y, m_pos.z);

    D3DXVECTOR4 cameraPos { };
    cameraPos.x = Camera::GetEyePos().x;
    cameraPos.y = Camera::GetEyePos().y;
    cameraPos.z = Camera::GetEyePos().z;
    cameraPos.w = 0.f;

    mat *= Camera::GetViewMatrix();
    mat *= Camera::GetProjMatrix();

    D3DXVECTOR3 lightPos(30.f * sinf(f), 10.f, -30.f * cosf(f));
    D3DXVECTOR4 lightPos2(lightPos.x, lightPos.y, lightPos.z, 0.f);

    hResult = m_D3DEffect->SetVector("g_cameraPos", &cameraPos);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->SetVector("g_lightPos", &lightPos2);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->SetMatrix("g_matWorldViewProj", &mat);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    UINT numPass;
    hResult = m_D3DEffect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = m_D3DEffect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < m_materialCount; i++)
    {
        hResult = m_D3DEffect->SetTexture("texture1", m_textureList[i]);
        assert(hResult == S_OK);

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

void MeshSmooth::OnDeviceLost()
{
    auto result = m_D3DEffect->OnLostDevice();
    assert(result == S_OK);
}

void MeshSmooth::OnDeviceReset()
{
    auto result = m_D3DEffect->OnResetDevice();
    assert(result == S_OK);
}

}
