#include "SkinAnimMesh.h"

#include "Common.h"
#include "Util.h"
#include "SkinAnimMeshAlloc.h"

namespace NSRender
{

//---------------------------------------------------------------
// SkinAnimMeshAlloc
//---------------------------------------------------------------

SkinAnimMeshAlloc::SkinAnimMeshAlloc(const std::wstring &xFilename)
    : m_xFilename(xFilename)
{
    // Nothing to do.
}

STDMETHODIMP SkinAnimMeshAlloc::CreateFrame(LPCSTR name, LPD3DXFRAME *newFrame)
{
    auto animMeshFrame = NEW SkinAnimMeshFrame();

    auto len = strlen(name);
    animMeshFrame->Name = NEW char[len + 1];
    strcpy_s(animMeshFrame->Name, len + 1, name);

    D3DXMatrixIdentity(&animMeshFrame->TransformationMatrix);
    D3DXMatrixIdentity(&animMeshFrame->m_combinedMatrix);

    *newFrame = animMeshFrame;

    return S_OK;
}

STDMETHODIMP SkinAnimMeshAlloc::CreateMeshContainer(LPCSTR meshName,
                                                    CONST D3DXMESHDATA *meshData,
                                                    CONST D3DXMATERIAL *materials,
                                                    CONST D3DXEFFECTINSTANCE *,
                                                    DWORD materialCount,
                                                    CONST DWORD *adjacency,
                                                    LPD3DXSKININFO skinInfo,
                                                    LPD3DXMESHCONTAINER *meshContainer)
{
    /*
    auto m_container = NEW SkinAnimMeshContainer(m_xFilename,
                                                       meshName,
                                                       meshData->pMesh,
                                                       materials,
                                                       materialCount,
                                                       adjacency,
                                                       skinInfo);
*/
    m_container = NEW SkinAnimMeshContainer();

    std::string meshFilename = meshName;

    m_container->Name = NEW char[meshFilename.length() + 1];
    strcpy_s(m_container->Name, meshFilename.length() + 1, meshFilename.c_str());

    LPDIRECT3DDEVICE9 d3d_device = NULL;
    meshData->pMesh->GetDevice(&d3d_device);

    HRESULT result = E_FAIL;
    if (!(meshData->pMesh->GetFVF() & D3DFVF_NORMAL))
    {
        m_container->MeshData.Type = D3DXMESHTYPE_MESH;
        result = meshData->pMesh->CloneMeshFVF(meshData->pMesh->GetOptions(),
                                     meshData->pMesh->GetFVF() | D3DFVF_NORMAL,
                                     d3d_device,
                                     &m_container->MeshData.pMesh);

        if (FAILED(result))
        {
            throw std::exception("Failed 'CloneMeshFVF' function.");
        }

        LPD3DXMESH temp = meshData->pMesh;
        temp = m_container->MeshData.pMesh;
        D3DXComputeNormals(temp, nullptr);
    }
    else
    {
        m_container->MeshData.pMesh = meshData->pMesh;
        m_container->MeshData.Type = D3DXMESHTYPE_MESH;
        meshData->pMesh->AddRef();
    }

    DWORD adjacency_count = meshData->pMesh->GetNumFaces() * 3;
    m_container->pAdjacency = NEW DWORD[adjacency_count];

    for (DWORD i = 0; i < adjacency_count; ++i)
    {
        m_container->pAdjacency[i] = adjacency[i];
    }

    InitializeMaterials(materialCount, materials, m_xFilename, d3d_device);
    InitializeBone(skinInfo, meshData->pMesh);
    InitializeFVF(d3d_device);
    InitializeVertexElement();

    *meshContainer = m_container;

    return S_OK;
}

STDMETHODIMP SkinAnimMeshAlloc::DestroyFrame(LPD3DXFRAME frame)
{
    SAFE_DELETE_ARRAY(frame->Name);
    SAFE_DELETE(frame);
    return S_OK;
}

STDMETHODIMP SkinAnimMeshAlloc::DestroyMeshContainer(LPD3DXMESHCONTAINER meshContainerBase)
{
    auto *meshContainer = (SkinAnimMeshContainer*)meshContainerBase;

    SAFE_RELEASE(meshContainer->pSkinInfo);
    SAFE_DELETE_ARRAY(meshContainer->Name);
    SAFE_DELETE_ARRAY(meshContainer->pAdjacency);
    SAFE_DELETE_ARRAY(meshContainer->pMaterials);
    SAFE_RELEASE(meshContainer->MeshData.pMesh);
    SAFE_DELETE(meshContainer);

    return S_OK;
}

//---------------------------------------------------------------
// SkinAnimMeshContainer
//---------------------------------------------------------------

//SkinAnimMeshContainer::SkinAnimMeshContainer(const std::wstring &xFilename,
//                                             const std::string &meshFilename,
//                                             LPD3DXMESH pMesh,
//                                             const D3DXMATERIAL *materials,
//                                             const DWORD materialCount,
//                                             const DWORD *adjacency,
//                                             LPD3DXSKININFO skinInfo)
//    : D3DXMESHCONTAINER { }
//{
//    LPDIRECT3DDEVICE9 d3d_device = NULL;
//    pMesh->GetDevice(&d3d_device);
//
//    /*
//    Name = NEW char[meshFilename.length() + 1];
//    strcpy_s(Name, meshFilename.length() + 1, meshFilename.c_str());
//
//    HRESULT result = E_FAIL;
//    if (!(pMesh->GetFVF() & D3DFVF_NORMAL))
//    {
//        MeshData.Type = D3DXMESHTYPE_MESH;
//        result = pMesh->CloneMeshFVF(pMesh->GetOptions(),
//                                    pMesh->GetFVF() | D3DFVF_NORMAL,
//                                    d3d_device,
//                                    &MeshData.pMesh);
//
//        if (FAILED(result))
//        {
//            throw std::exception("Failed 'CloneMeshFVF' function.");
//        }
//
//        pMesh = MeshData.pMesh;
//        D3DXComputeNormals(pMesh, nullptr);
//    }
//    else
//    {
//        MeshData.pMesh = pMesh;
//        MeshData.Type = D3DXMESHTYPE_MESH;
//        pMesh->AddRef();
//    }
//
//    DWORD adjacency_count = pMesh->GetNumFaces() * 3;
//    pAdjacency = NEW DWORD[adjacency_count];
//
//    for (DWORD i = 0; i < adjacency_count; ++i)
//    {
//        pAdjacency[i] = adjacency[i];
//    }
//    */
//
//    /*
//    InitializeMaterials(materialCount, materials, xFilename, d3d_device);
//    InitializeBone(skinInfo, pMesh);
//    InitializeFVF(d3d_device);
//    InitializeVertexElement();
//    */
//}

void SkinAnimMeshAlloc::InitializeMaterials(const DWORD &materialCount,
                                                const D3DXMATERIAL *materials,
                                                const std::wstring &xFilename,
                                                const LPDIRECT3DDEVICE9 &d3dDevice)
{
    m_container->NumMaterials = (std::max)(1UL, materialCount);
    m_container->pMaterials = NEW D3DXMATERIAL[m_container->NumMaterials];

    if (materialCount > 0)
    {
        for (DWORD i = 0; i < materialCount; ++i)
        {
            m_container->pMaterials[i] = materials[i];
            if (m_container->pMaterials[i].pTextureFilename != nullptr)
            {
                LPDIRECT3DTEXTURE9 tempTexture = NULL;
                std::wstring filename = Util::Utf8ToWstring(m_container->pMaterials[i].pTextureFilename);

                size_t pos = xFilename.find_last_of(L"\\/");

                std::wstring directory;
                if (pos != std::string::npos)
                {
                    directory = xFilename.substr(0, pos);
                }

                filename = directory + L'\\' + filename;

                if (FAILED(D3DXCreateTextureFromFile(d3dDevice,
                                                     filename.c_str(),
                                                     &tempTexture)))
                {
                    throw std::exception("texture file is not found.");
                }
                else
                {
                    m_container->m_textureList.push_back(tempTexture);
                }
            }
        }
    }
    else
    {
        m_container->pMaterials[0].MatD3D.Diffuse = D3DCOLORVALUE{0.5f, 0.5f, 0.5f, 0};
        m_container->pMaterials[0].MatD3D.Ambient = D3DCOLORVALUE{0.5f, 0.5f, 0.5f, 0};
        m_container->pMaterials[0].MatD3D.Specular = m_container->pMaterials[0].MatD3D.Diffuse;
    }
}

void SkinAnimMeshAlloc::InitializeBone(const LPD3DXSKININFO &skinInfo,
                                           const LPD3DXMESH &mesh)
{
    if (skinInfo == NULL)
    {
        throw std::exception("Failed to get skin info.");
    }
    m_container->pSkinInfo = skinInfo;
    m_container->pSkinInfo->AddRef();

    UINT boneCount = m_container->pSkinInfo->GetNumBones();
    m_container->m_boneOffsetMatrices.resize(boneCount);

    for (DWORD i = 0; i < boneCount; ++i)
    {
        m_container->m_boneOffsetMatrices[i] = *m_container->pSkinInfo->GetBoneOffsetMatrix(i);
    }

    // TODO Improve.
    DWORD MAX_MATRICES = 26;
    auto boneNum = m_container->pSkinInfo->GetNumBones();
    m_container->m_paletteSize = (std::min)(MAX_MATRICES, boneNum);

    SAFE_RELEASE(m_container->MeshData.pMesh);

    LPD3DXBUFFER bone_buffer = NULL;
    HRESULT hr = m_container->pSkinInfo->ConvertToIndexedBlendedMesh(mesh,
                                                      0 /* not used */, 
                                                      m_container->m_paletteSize,
                                                      m_container->pAdjacency,
                                                      nullptr,
                                                      nullptr,
                                                      nullptr,
                                                      &m_container->m_influenceCount,
                                                      &m_container->m_boneCount,
                                                      &bone_buffer,
                                                                     &m_container->MeshData.pMesh);
    if (hr != S_OK)
    {
        throw std::exception("Failed to get skin info.");
    }

    m_container->m_boneBuffer = bone_buffer;
}

void SkinAnimMeshAlloc::InitializeFVF(const LPDIRECT3DDEVICE9 &d3dDevice)
{
    DWORD newFVF = (m_container->MeshData.pMesh->GetFVF() &
                    D3DFVF_POSITION_MASK) |
                    D3DFVF_NORMAL |
                    D3DFVF_TEX1 |
                    D3DFVF_LASTBETA_UBYTE4;

    if (newFVF != m_container->MeshData.pMesh->GetFVF())
    {
        LPD3DXMESH pMesh = NULL;
        HRESULT hresult = m_container->MeshData.pMesh->CloneMeshFVF(m_container->MeshData.pMesh->GetOptions(),
                                                       newFVF,
                                                       d3dDevice,
                                                       &pMesh);

        if (SUCCEEDED(hresult))
        {
            m_container->MeshData.pMesh->Release();
            m_container->MeshData.pMesh = pMesh;
            pMesh = NULL;
        }
    }
}

void SkinAnimMeshAlloc::InitializeVertexElement()
{
    D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
    LPD3DVERTEXELEMENT9 currentDecl = NULL;
    HRESULT result = m_container->MeshData.pMesh->GetDeclaration(decl);
    if (FAILED(result))
    {
        throw std::exception("Failed to get skin info.");
    }

    currentDecl = decl;
    while (currentDecl->Stream != 0xff)
    {
        if ((currentDecl->Usage == D3DDECLUSAGE_BLENDINDICES) && (currentDecl->UsageIndex == 0))
        {
            currentDecl->Type = D3DDECLTYPE_D3DCOLOR;
        }
        currentDecl++;
    }

    result = m_container->MeshData.pMesh->UpdateSemantics(decl);
    if (FAILED(result))
    {
        throw std::exception("Failed to get skin info.");
    }
}

}

