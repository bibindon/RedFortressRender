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
    m_container = NEW SkinAnimMeshContainer();

    std::string meshFilename = meshName;

    m_container->Name = NEW char[meshFilename.length() + 1];
    strcpy_s(m_container->Name, meshFilename.length() + 1, meshFilename.c_str());

    HRESULT result = E_FAIL;
    if (!(meshData->pMesh->GetFVF() & D3DFVF_NORMAL))
    {
        m_container->MeshData.Type = D3DXMESHTYPE_MESH;
        result = meshData->pMesh->CloneMeshFVF(meshData->pMesh->GetOptions(),
                                     meshData->pMesh->GetFVF() | D3DFVF_NORMAL,
                                     Common::D3DDevice(),
                                     &m_container->MeshData.pMesh);

        if (FAILED(result))
        {
            // ‚±‚ÌŠÖ”‚Ínothrow‚ªŽw’è‚³‚ê‚Ä‚¢‚³‚ê‚Ä‚¢‚é‚Ì‚ÅA—áŠO‚ð•Ô‚µ‚Ä‚Í‚¢‚¯‚È‚¢
            return E_FAIL;
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

    InitializeMaterials(materialCount, materials, m_xFilename);
    InitializeBone(skinInfo, meshData->pMesh);

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

void SkinAnimMeshAlloc::InitializeMaterials(const DWORD materialCount,
                                            const D3DXMATERIAL *materials,
                                            const std::wstring &xFilename)
{
    if (materialCount == 0)
    {
        throw std::exception("materialCount is 0. Re-output by Blender.");
    }

    m_container->NumMaterials = materialCount;
    m_container->pMaterials = NEW D3DXMATERIAL[m_container->NumMaterials];

    std::wstring dirPath;

    if (PathIsRelative(xFilename.c_str()))
    {
        dirPath = Util::GetExeDir() + xFilename;
    }
    else
    {
        dirPath = xFilename;
    }

    size_t pos = dirPath.find_last_of(L"\\/");

    if (pos == std::string::npos)
    {
        throw std::exception("xFilename is wrong.");
    }

    dirPath = dirPath.substr(0, pos);

    for (DWORD i = 0; i < materialCount; ++i)
    {
        m_container->pMaterials[i] = materials[i];

        if (m_container->pMaterials[i].pTextureFilename != nullptr)
        {
            LPDIRECT3DTEXTURE9 tempTexture = NULL;

            std::wstring filename = Util::Utf8ToWstring(m_container->pMaterials[i].pTextureFilename);

            filename = dirPath + L'\\' + filename;

            HRESULT hResult = E_FAIL;
            hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                filename.c_str(),
                                                &tempTexture);

            if (FAILED(hResult))
            {
                throw std::exception("texture file is not found.");
            }

            m_container->m_textureList.push_back(tempTexture);
        }
    }
}

void SkinAnimMeshAlloc::InitializeBone(const LPD3DXSKININFO skinInfo,
                                       const LPD3DXMESH d3dMesh)
{
    if (skinInfo == NULL)
    {
        throw std::exception("Failed to get skin info.");
    }

    UINT boneCount = skinInfo->GetNumBones();
    m_container->m_boneOffsetMatrices.resize(boneCount);

    for (DWORD i = 0; i < boneCount; ++i)
    {
        m_container->m_boneOffsetMatrices[i] = *skinInfo->GetBoneOffsetMatrix(i);
    }

    DWORD MAX_MATRICES = 8;
    auto boneNum = skinInfo->GetNumBones();

    if (boneNum >= MAX_MATRICES)
    {
        boneNum = MAX_MATRICES;
    }

    m_container->m_paletteSize = boneNum;

    m_container->MeshData.pMesh->Release();

    LPD3DXBUFFER bone_buffer = NULL;
    HRESULT hResult = skinInfo->ConvertToIndexedBlendedMesh(d3dMesh,
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

    if (FAILED(hResult))
    {
        throw std::exception("Failed to get skin info.");
    }

    m_container->m_boneBuffer = bone_buffer;

    m_container->pSkinInfo = skinInfo;
    m_container->pSkinInfo->AddRef();

}

}

