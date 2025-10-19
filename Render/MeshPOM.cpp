#include "MeshPOM.h"

#include "Camera.h"
#include "Light.h"

namespace NSRender
{

void MeshPOM::Initialize(const std::wstring& filename,
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
                                &m_pAdjacency,
                                &pD3DXMtrlBuffer,
                                NULL,
                                &m_materialCount,
                                &m_D3DMesh);

    assert(hResult == S_OK);

    // 各頂点の法線は、面に垂直な法線である必要があるため、
    // 計算しなおしてはいけない

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

    AddTangentBinormalToMesh();
}

void MeshPOM::AddTangentBinormalToMesh()
{
    const D3DVERTEXELEMENT9 declFixed[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
        { 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,  0 },
        { 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
        D3DDECL_END()
    };

    LPD3DXMESH pCloned = NULL;
    HRESULT hr = m_D3DMesh->CloneMesh(m_D3DMesh->GetOptions(),
                                      declFixed,
                                      Common::D3DDevice(),
                                      &pCloned);
    assert(SUCCEEDED(hr));
    SAFE_RELEASE(m_D3DMesh);
    m_D3DMesh = pCloned;

    LPD3DXMESH pOut = NULL;
    hr = D3DXComputeTangentFrameEx(m_D3DMesh,
                                   D3DDECLUSAGE_TEXCOORD, 0,
                                   D3DDECLUSAGE_TANGENT,  0,
                                   D3DDECLUSAGE_BINORMAL, 0,
                                   D3DDECLUSAGE_NORMAL,   0,
                                   0,
                                   (const DWORD*)m_pAdjacency->GetBufferPointer(),
                                   0.01f,
                                   0.01f,
                                   // 角の平滑化を防止。平面に対してだけ平滑化を行うようにする
                                   0.999f,
                                   &pOut,
                                   NULL);
    assert(SUCCEEDED(hr));

    SAFE_RELEASE(m_D3DMesh);
    m_D3DMesh = pOut;
}

void MeshPOM::Finalize()
{
    SAFE_RELEASE(m_D3DEffect);
    SAFE_RELEASE(m_pAdjacency);

    for (auto& texture : m_textureList)
    {
        SAFE_RELEASE(texture);
    }

    SAFE_RELEASE(m_D3DMesh);
}

void MeshPOM::Draw()
{
    HRESULT hr = E_FAIL;

    D3DXMATRIX mWorld;
    D3DXMATRIX mWorldViewProj;

    D3DXMatrixIdentity(&mWorld);
    D3DXMatrixTranslation(&mWorld, m_pos.x, m_pos.y, m_pos.z);

    m_D3DEffect->SetMatrix("g_mWorld", &mWorld);

    auto mView = Camera::GetViewMatrix();
    auto mProj = Camera::GetProjMatrix();

    mWorldViewProj = mWorld * mView * mProj;

    m_D3DEffect->SetMatrix("g_mWorldViewProj", &mWorldViewProj);

    Camera::GetEyePos();
    D3DXVECTOR4 vEye(Camera::GetEyePos(), 1.0f);
    m_D3DEffect->SetVector("g_vEye", &vEye);

    D3DXVECTOR3 lightDir(Light::GetLightDir());
    D3DXVec3Normalize(&lightDir, &lightDir);
    m_D3DEffect->SetValue("g_LightDir", &lightDir, sizeof(D3DXVECTOR3));

    m_D3DEffect->SetFloat("g_fBaseTextureRepeat", 1.0f);
    m_D3DEffect->SetFloat("g_fHeightMapScale",    0.1f);

    m_D3DEffect->SetTexture("g_baseTexture", m_textureList.at(0));
    m_D3DEffect->SetTexture("g_normalTexture", m_textureList.at(1));
    m_D3DEffect->SetTexture("g_heightTexture", m_textureList.at(2));

    D3DXHANDLE hTech = m_D3DEffect->GetTechniqueByName("Technique0");
    m_D3DEffect->SetTechnique(hTech);

    UINT passes = 0;
    m_D3DEffect->Begin(&passes, 0);
    m_D3DEffect->BeginPass(0);

    m_D3DMesh->DrawSubset(0);

    m_D3DEffect->EndPass();
    m_D3DEffect->End();
}

void MeshPOM::OnDeviceLost()
{
    HRESULT hr = m_D3DEffect->OnLostDevice();
    assert(hr == S_OK);
}

void MeshPOM::OnDeviceReset()
{
    HRESULT hr = m_D3DEffect->OnResetDevice();
    assert(hr == S_OK);
}

}
