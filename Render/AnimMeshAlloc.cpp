#include "AnimMeshAlloc.h"

#include <algorithm>

#include "Common.h"
#include "Util.h"

namespace NSRender
{

AnimMeshAllocator::AnimMeshAllocator(const std::wstring& xFilename)
    : ID3DXAllocateHierarchy(),
    m_xFilename(xFilename)
{
    // Nothing to do
}

STDMETHODIMP AnimMeshAllocator::CreateFrame(LPCSTR name, LPD3DXFRAME* newFrame)
{
    auto animMeshFrame = NEW AnimMeshFrame();

    auto len = strlen(name);
    animMeshFrame->Name = NEW char[len + 1];
    strcpy_s(animMeshFrame->Name, len + 1, name);

    D3DXMatrixIdentity(&animMeshFrame->TransformationMatrix);
    D3DXMatrixIdentity(&animMeshFrame->m_combinedMatrix);

    *newFrame = animMeshFrame;

    return S_OK;
}

STDMETHODIMP AnimMeshAllocator::CreateMeshContainer(LPCSTR meshName_,
                                                    CONST D3DXMESHDATA* meshData,
                                                    CONST D3DXMATERIAL* materials,
                                                    CONST D3DXEFFECTINSTANCE*,
                                                    DWORD materialsCount,
                                                    CONST DWORD* adjacency,
                                                    LPD3DXSKININFO,
                                                    LPD3DXMESHCONTAINER* meshContainer)
{
    auto container = NEW AnimMeshContainer();

    std::string meshName = meshName_;
    container->Name = NEW char[meshName.length() + 1];
    strcpy_s(container->Name, meshName.length() + 1, meshName.c_str());

    LPDIRECT3DDEVICE9 d3dDevice = NULL;
    meshData->pMesh->GetDevice(&d3dDevice);

    if (!(meshData->pMesh->GetFVF() & D3DFVF_NORMAL))
    {
        container->MeshData.Type = D3DXMESHTYPE_MESH;
        HRESULT hResult = E_FAIL;
        hResult = meshData->pMesh->CloneMeshFVF(meshData->pMesh->GetOptions(),
                                                meshData->pMesh->GetFVF() | D3DFVF_NORMAL,
                                                d3dDevice,
                                                &container->MeshData.pMesh);

        if (FAILED(hResult))
        {
            return E_FAIL;
        }

        LPD3DXMESH temp = meshData->pMesh;
        temp = container->MeshData.pMesh;
        D3DXComputeNormals(meshData->pMesh, NULL);
    }
    else
    {
        D3DXComputeNormals(meshData->pMesh, adjacency);
        container->MeshData.pMesh = meshData->pMesh;
        container->MeshData.Type = D3DXMESHTYPE_MESH;
        meshData->pMesh->AddRef();
    }

    container->NumMaterials = (std::max)(1UL, materialsCount);
    container->pMaterials = NEW D3DXMATERIAL[container->NumMaterials];
    std::vector<LPDIRECT3DTEXTURE9> tempTexture(container->NumMaterials);
    container->m_vecTexture.swap(tempTexture);

    DWORD adjacencyCount = meshData->pMesh->GetNumFaces() * 3;
    container->pAdjacency = NEW DWORD[adjacencyCount];

    for (DWORD i = 0; i < adjacencyCount; ++i)
    {
        container->pAdjacency[i] = adjacency[i];
    }

    if (materialsCount > 0)
    {
        for (DWORD i = 0; i < materialsCount; ++i)
        {
            container->pMaterials[i] = materials[i];
        }

        std::wstring xFileDir = m_xFilename;
        std::size_t lastPos = xFileDir.find_last_of(L"\\");
        xFileDir = xFileDir.substr(0, lastPos + 1);

        for (DWORD i = 0; i < materialsCount; ++i)
        {
            container->pMaterials[i].MatD3D.Ambient = D3DCOLORVALUE { 0.2f, 0.2f, 0.2f, 1.f };

            if (container->pMaterials[i].pTextureFilename != nullptr)
            {
                std::wstring texPath = xFileDir;
                texPath += Util::Utf8ToWstring(materials[i].pTextureFilename);
                LPDIRECT3DTEXTURE9 tempTexture { nullptr };

                if (FAILED(D3DXCreateTextureFromFile(d3dDevice,
                                                     texPath.c_str(),
                                                     &tempTexture)))
                {
                    throw std::exception("texture file is not found.");
                }
                else
                {
                    SAFE_RELEASE(container->m_vecTexture.at(i));
                    container->m_vecTexture.at(i) = tempTexture;
                }
            }
        }
    }
    else
    {
        container->pMaterials[0].MatD3D.Diffuse = D3DCOLORVALUE { 0.5f, 0.5f, 0.5f, 1.f };
        container->pMaterials[0].MatD3D.Ambient = D3DCOLORVALUE { 0.5f, 0.5f, 0.5f, 1.f };
        container->pMaterials[0].MatD3D.Specular = D3DCOLORVALUE { 0.5f, 0.5f, 0.5f, 1.f };
    }

    *meshContainer = (LPD3DXMESHCONTAINER)container;

    return S_OK;
}

STDMETHODIMP AnimMeshAllocator::DestroyFrame(LPD3DXFRAME frame)
{
    SAFE_DELETE_ARRAY(frame->Name);
    SAFE_DELETE(frame);

    return S_OK;
}

STDMETHODIMP AnimMeshAllocator::DestroyMeshContainer(LPD3DXMESHCONTAINER meshContainerBase)
{
    AnimMeshContainer* meshContainer = static_cast<AnimMeshContainer*>(meshContainerBase);

    SAFE_RELEASE(meshContainer->pSkinInfo);
    SAFE_DELETE_ARRAY(meshContainer->Name);
    SAFE_DELETE_ARRAY(meshContainer->pAdjacency);
    SAFE_DELETE_ARRAY(meshContainer->pMaterials);
    SAFE_RELEASE(meshContainer->MeshData.pMesh);

    for (size_t i = 0; i < meshContainer->m_vecTexture.size(); ++i)
    {
        SAFE_RELEASE(meshContainer->m_vecTexture.at(i));
    }

    SAFE_DELETE(meshContainer);

    return S_OK;
}

}

