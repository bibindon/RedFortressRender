#include "MeshInstancing.h"

namespace NSRender
{



MeshInstancing::MeshInstancing()
{
}

void MeshInstancing::Initialize()
{
    HRESULT hResult = E_FAIL;

    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;

    hResult = D3DXLoadMeshFromX(L"cube.x",
                                D3DXMESH_SYSTEMMEM,
                                Common::D3DDevice(),
                                NULL,
                                &pD3DXMtrlBuffer,
                                NULL,
                                &m_dwNumMaterials,
                                &m_pMesh);

    assert(hResult == S_OK);

    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    m_pMaterials.resize(m_dwNumMaterials);
    m_pTextures.resize(m_dwNumMaterials);

    for (DWORD i = 0; i < m_dwNumMaterials; i++)
    {
        m_pMaterials[i] = d3dxMaterials[i].MatD3D;
        m_pMaterials[i].Ambient = m_pMaterials[i].Diffuse;
        m_pTextures[i] = NULL;

        //--------------------------------------------------------------
        // Unicode文字セットでもマルチバイト文字セットでも
        // "d3dxMaterials[i].pTextureFilename"はマルチバイト文字セットになる。
        // 
        // 一方で、D3DXCreateTextureFromFileはプロジェクト設定で
        // Unicode文字セットかマルチバイト文字セットか変わる。
        //--------------------------------------------------------------

        std::string pTexPath(d3dxMaterials[i].pTextureFilename);

        if (!pTexPath.empty())
        {
            bool bUnicode = false;

            int len = MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, nullptr, 0);
            std::wstring pTexPathW(len, 0);
            MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, &pTexPathW[0], len);

            hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                pTexPathW.c_str(),
                                                &m_pTextures[i]);
            assert(hResult == S_OK);
        }
    }

    hResult = pD3DXMtrlBuffer->Release();
    assert(hResult == S_OK);

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       L"res\\shader\\MeshInstancing.fx",
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_pEffect,
                                       NULL);

    assert(hResult == S_OK);

    // ワールド座標位置バッファ
    WorldPos* worldPos = new WorldPos[tipNum];

    for (int d = 0; d < D; d++)
    {
        for (int w = 0; w < W; w++)
        {
            for (int h = 0; h < H; h++)
            {
                int e = H * W * d + h * W + w;
                worldPos[e].x = 10.f * (w - (W / 2));
                worldPos[e].y = 10.f * (h - (H / 2));
                worldPos[e].z = 10.f * (d - (D / 2));
            }
        }
    }

    Common::D3DDevice()->CreateVertexBuffer(sizeof(WorldPos) * tipNum,
                                     0,
                                     0,
                                     D3DPOOL_MANAGED,
                                     &m_worldPosBuf,
                                     0);

    copyBuf(sizeof(WorldPos) * tipNum, worldPos, m_worldPosBuf);

    delete[] worldPos;

    // 頂点宣言作成
    D3DVERTEXELEMENT9 declElems[] =
    {
        // POSITION
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },

        // NORMAL0
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },

        // TEXCOORD0
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },

        // TEXCOORD1
        // ワールド位置
        { 1, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },

        D3DDECL_END()
    };

    Common::D3DDevice()->CreateVertexDeclaration(declElems, &m_decl);

}

void MeshInstancing::Finalize()
{

}

void MeshInstancing::AddInstance(const D3DXVECTOR3& pos)
{

}

void MeshInstancing::Draw()
{
    HRESULT hResult = E_FAIL;

    static float f = 0.0f;
    f += 0.025f;

    D3DXMATRIX mat;
    D3DXMATRIX View, Proj;

    D3DXMatrixPerspectiveFovLH(&Proj,
                               D3DXToRadian(45),
                               1920.0f / 1080.0f,
                               1.0f,
                               10000.0f);

    D3DXVECTOR3 vec1(200 * sinf(f), 10, -200 * cosf(f));
    D3DXVECTOR3 vec2(0, 0, 0);
    D3DXVECTOR3 vec3(0, 1, 0);
    D3DXMatrixLookAtLH(&View, &vec1, &vec2, &vec3);
    D3DXMatrixIdentity(&mat);
    mat = mat * View * Proj;

    hResult = m_pEffect->SetMatrix("g_matWorldViewProj", &mat);
    assert(hResult == S_OK);

    LPDIRECT3DVERTEXBUFFER9 pVB = nullptr;
    m_pMesh->GetVertexBuffer(&pVB);
    Common::D3DDevice()->SetStreamSource(0, pVB, 0, m_pMesh->GetNumBytesPerVertex());
    pVB->Release();

    Common::D3DDevice()->SetStreamSource(1, m_worldPosBuf, 0, sizeof(WorldPos));

    Common::D3DDevice()->SetVertexDeclaration(m_decl);

    Common::D3DDevice()->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | tipNum);
    Common::D3DDevice()->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1);

    // インデックスバッファをセット
    LPDIRECT3DINDEXBUFFER9 pIB = nullptr;
    m_pMesh->GetIndexBuffer(&pIB);
    Common::D3DDevice()->SetIndices(pIB);
    pIB->Release();

    hResult = m_pEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    UINT numPass;
    hResult = m_pEffect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = m_pEffect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < m_dwNumMaterials; i++)
    {
        hResult = m_pEffect->SetTexture("texture1", m_pTextures[i]);
        assert(hResult == S_OK);

        hResult = m_pEffect->CommitChanges();
        assert(hResult == S_OK);

        DWORD numVertices = m_pMesh->GetNumVertices();
        DWORD numFaces = m_pMesh->GetNumFaces();

        // 描画 (インスタンス数は SetStreamSourceFreq で決まる)
        Common::D3DDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                           0,
                                           0,
                                           m_pMesh->GetNumVertices(),
                                           0,
                                           m_pMesh->GetNumFaces());

        assert(hResult == S_OK);
    }

    hResult = m_pEffect->EndPass();
    assert(hResult == S_OK);

    hResult = m_pEffect->End();
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetStreamSourceFreq(0, 1);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetStreamSourceFreq(1, 1);
    assert(hResult == S_OK);
}


void MeshInstancing::OnDeviceLost()
{
    
}

void MeshInstancing::OnDeviceReset()
{

}

void MeshInstancing::copyBuf(unsigned sz, void* src, IDirect3DVertexBuffer9* buf)
{
    void* p = 0;
    buf->Lock(0, 0, &p, 0);
    memcpy(p, src, sz);
    buf->Unlock();
}

}
