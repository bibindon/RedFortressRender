#include "MeshMixAnimNoBoneAlloc.h"

#include "Common.h"
#include "Util.h"

namespace NSRender
{

AnimNoBoneMeshAlloc::AnimNoBoneMeshAlloc(const std::wstring &xFilename)
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

AnimNoBoneMeshAlloc::~AnimNoBoneMeshAlloc()
{
    ClearTextureCache();
}

STDMETHODIMP AnimNoBoneMeshAlloc::CreateFrame(LPCSTR name, LPD3DXFRAME *newFrame)
{
    auto animFrame = NEW AnimNoBoneFrame();

    if (name != nullptr)
    {
        auto len = strlen(name);
        animFrame->Name = NEW char[len + 1];
        strcpy_s(animFrame->Name, len + 1, name);
    }

    D3DXMatrixIdentity(&animFrame->TransformationMatrix);
    D3DXMatrixIdentity(&animFrame->m_combinedMatrix);
    D3DXQuaternionIdentity(&animFrame->m_animationRotation);
    animFrame->m_animationScale = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
    animFrame->m_animationPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    *newFrame = animFrame;

    return S_OK;
}

STDMETHODIMP AnimNoBoneMeshAlloc::CreateMeshContainer(LPCSTR meshName,
                                                       CONST D3DXMESHDATA *meshData,
                                                       CONST D3DXMATERIAL *materials,
                                                       CONST D3DXEFFECTINSTANCE *,
                                                       DWORD materialCount,
                                                       CONST DWORD *adjacency,
                                                       LPD3DXSKININFO,
                                                       LPD3DXMESHCONTAINER *meshContainer)
{
    m_container = NEW AnimNoBoneMeshContainer();

    std::string meshFilename = meshName;

    m_container->Name = NEW char[meshFilename.length() + 1];
    strcpy_s(m_container->Name, meshFilename.length() + 1, meshFilename.c_str());

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

    *meshContainer = m_container;

    return S_OK;
}

STDMETHODIMP AnimNoBoneMeshAlloc::DestroyFrame(LPD3DXFRAME frame)
{
    SAFE_DELETE_ARRAY(frame->Name);
    SAFE_DELETE(frame);
    return S_OK;
}

STDMETHODIMP AnimNoBoneMeshAlloc::DestroyMeshContainer(LPD3DXMESHCONTAINER meshContainerBase)
{
    auto *meshContainer = (AnimNoBoneMeshContainer*)meshContainerBase;

    SAFE_RELEASE(meshContainer->pSkinInfo);
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

void AnimNoBoneMeshAlloc::InitializeMaterials(const DWORD materialCount,
                                              const D3DXMATERIAL *materials,
                                              const std::wstring &xFilename)
{
    if (materialCount == 0)
    {
        return;
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

void AnimNoBoneMeshAlloc::ClearTextureCache()
{
    for (auto& texturePair : m_textureCache)
    {
        SAFE_RELEASE(texturePair.second);
    }

    m_textureCache.clear();
}

std::wstring AnimNoBoneMeshAlloc::ResolveTexturePath(const char* textureFilename) const
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

LPDIRECT3DTEXTURE9 AnimNoBoneMeshAlloc::LoadTextureCached(const std::wstring& texturePath)
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
        return nullptr;
    }

    texture->AddRef();
    m_textureCache[texturePath] = texture;
    return texture;
}

}
