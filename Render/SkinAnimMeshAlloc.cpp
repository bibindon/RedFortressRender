#include "SkinAnimMesh.h"

#include "Common.h"
#include "Util.h"
#include "SkinAnimMeshAlloc.h"
#include <stdexcept>

namespace NSRender
{

//---------------------------------------------------------------
// SkinAnimMeshAlloc
//---------------------------------------------------------------

SkinAnimMeshAlloc::SkinAnimMeshAlloc(const std::wstring &xFilename)
    : m_xFilename(xFilename)
{
    std::wstring resolvedPath;
    if (PathIsRelative(xFilename.c_str()))
    {
        resolvedPath = Util::GetExeDir() + xFilename;
    }
    else
    {
        resolvedPath = xFilename;
    }

    size_t pos = resolvedPath.find_last_of(L"\\/");
    if (pos != std::string::npos)
    {
        m_baseDirectory = resolvedPath.substr(0, pos);
    }
}

SkinAnimMeshAlloc::~SkinAnimMeshAlloc()
{
    ClearTextureCache();
}

STDMETHODIMP SkinAnimMeshAlloc::CreateFrame(LPCSTR name, LPD3DXFRAME *newFrame)
{
    auto animMeshFrame = NEW SkinAnimMeshFrame();

    if (name == nullptr)
    {
        animMeshFrame->Name = nullptr;
    }
    else
    {
        auto len = strlen(name);
        animMeshFrame->Name = NEW char[len + 1];
        strcpy_s(animMeshFrame->Name, len + 1, name);
    }

    D3DXMatrixIdentity(&animMeshFrame->TransformationMatrix);
    D3DXMatrixIdentity(&animMeshFrame->m_combinedMatrix);
    D3DXQuaternionIdentity(&animMeshFrame->m_animationRotation);
    animMeshFrame->m_animationScale = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
    animMeshFrame->m_animationPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

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

    if (meshName == nullptr)
    {
        m_container->Name = nullptr;
    }
    else
    {
        std::string meshFilename = meshName;
        m_container->Name = NEW char[meshFilename.length() + 1];
        strcpy_s(m_container->Name, meshFilename.length() + 1, meshFilename.c_str());
    }

    HRESULT result = E_FAIL;
    if (!(meshData->pMesh->GetFVF() & D3DFVF_NORMAL))
    {
        m_container->MeshData.Type = D3DXMESHTYPE_MESH;
        result = meshData->pMesh->CloneMeshFVF(meshData->pMesh->GetOptions() | D3DXMESH_32BIT,
                                     meshData->pMesh->GetFVF() | D3DFVF_NORMAL,
                                     Common::D3DDevice(),
                                     &m_container->MeshData.pMesh);

        if (FAILED(result))
        {
            // この関数はnothrowが指定されていされているので、例外を返してはいけない
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
    SAFE_RELEASE(meshContainer->m_boneBuffer);
    SAFE_DELETE_ARRAY(meshContainer->Name);
    SAFE_DELETE_ARRAY(meshContainer->pAdjacency);
    SAFE_DELETE_ARRAY(meshContainer->pMaterials);
    SAFE_RELEASE(meshContainer->MeshData.pMesh);

    for (size_t i = 0; i < meshContainer->m_textureList.size(); ++i)
    {
        SAFE_RELEASE(meshContainer->m_textureList.at(i));
    }

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

    for (DWORD i = 0; i < materialCount; ++i)
    {
        m_container->pMaterials[i] = materials[i];
        LPDIRECT3DTEXTURE9 tempTexture = nullptr;

        if (m_container->pMaterials[i].pTextureFilename != nullptr)
        {
            const std::wstring resolvedPath = ResolveTexturePath(m_container->pMaterials[i].pTextureFilename);

            if (!resolvedPath.empty())
            {
                tempTexture = LoadTextureCached(resolvedPath);
            }
        }

        m_container->m_textureList.push_back(tempTexture);
    }
}

void SkinAnimMeshAlloc::InitializeBone(const LPD3DXSKININFO skinInfo,
                                       const LPD3DXMESH d3dMesh)
{
    if (skinInfo == NULL)
    {
        return;
    }

    UINT boneCount = skinInfo->GetNumBones();
    m_container->m_boneOffsetMatrices.resize(boneCount);
    m_container->m_boneNames.resize(boneCount);

    for (DWORD i = 0; i < boneCount; ++i)
    {
        m_container->m_boneOffsetMatrices[i] = *skinInfo->GetBoneOffsetMatrix(i);
        const char* boneName = skinInfo->GetBoneName(i);
        if (boneName != nullptr)
        {
            m_container->m_boneNames[i] = boneName;
        }
    }

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

void SkinAnimMeshAlloc::ClearTextureCache()
{
    for (auto& texturePair : m_textureCache)
    {
        SAFE_RELEASE(texturePair.second);
    }

    m_textureCache.clear();
}

std::wstring SkinAnimMeshAlloc::ResolveTexturePath(const char* textureFilename) const
{
    if (textureFilename == nullptr || textureFilename[0] == '\0')
    {
        return L"";
    }

    std::wstring textureName = Util::Utf8ToWstring(textureFilename);

    std::wstring combinedPath;
    if (PathIsRelative(textureName.c_str()))
    {
        combinedPath = m_baseDirectory + L"\\" + textureName;
    }
    else
    {
        combinedPath = textureName;
    }

    wchar_t fullPath[MAX_PATH] { };
    const DWORD length = GetFullPathNameW(
        combinedPath.c_str(),
        _countof(fullPath),
        fullPath,
        nullptr);

    if (length > 0 && length < _countof(fullPath))
    {
        return fullPath;
    }

    return combinedPath;
}

LPDIRECT3DTEXTURE9 SkinAnimMeshAlloc::LoadTextureCached(const std::wstring& texturePath)
{
    auto foundTexture = m_textureCache.find(texturePath);
    if (foundTexture != m_textureCache.end())
    {
        foundTexture->second->AddRef();
        return foundTexture->second;
    }

    LPDIRECT3DTEXTURE9 texture = nullptr;
    HRESULT hr = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                           texturePath.c_str(),
                                           &texture);
    if (FAILED(hr) || texture == nullptr)
    {
        const std::string message = "Failed to load a skin animation texture: " +
                                    Util::WstringToUtf8(texturePath);
        throw std::runtime_error(message);
    }

    texture->AddRef();
    m_textureCache[texturePath] = texture;
    return texture;
}

}

