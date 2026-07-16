#include "MeshMix2Hierarchy.h"

#include "Common.h"
#include "CustomXLoader.h"
#include "Util.h"

#include <cmath>
#include <stdexcept>

namespace NSRender
{
namespace
{

struct TangentVertex
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
    D3DXVECTOR2 texcoord;
    D3DXVECTOR3 tangent;
    D3DXVECTOR3 binormal;
};

static_assert(sizeof(TangentVertex) == 56, "Unexpected MeshMix2 tangent vertex layout.");

bool NormalizeVector(D3DXVECTOR3& vector)
{
    constexpr float kMinimumLengthSquared = 1.0e-12f;
    if (D3DXVec3LengthSq(&vector) <= kMinimumLengthSquared)
    {
        return false;
    }

    D3DXVec3Normalize(&vector, &vector);
    return true;
}

D3DXVECTOR3 BuildPerpendicularVector(const D3DXVECTOR3& normal)
{
    D3DXVECTOR3 referenceAxis(0.0f, 1.0f, 0.0f);
    if (std::fabs(normal.y) >= 0.999f)
    {
        referenceAxis = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    }

    D3DXVECTOR3 perpendicular;
    D3DXVec3Cross(&perpendicular, &referenceAxis, &normal);
    NormalizeVector(perpendicular);
    return perpendicular;
}

HRESULT ComputeTangentFrameFallback(LPD3DXMESH mesh)
{
    if (mesh == nullptr ||
        !(mesh->GetOptions() & D3DXMESH_32BIT) ||
        mesh->GetNumBytesPerVertex() != sizeof(TangentVertex))
    {
        return E_INVALIDARG;
    }

    TangentVertex* vertices = nullptr;
    HRESULT result = mesh->LockVertexBuffer(0, reinterpret_cast<void**>(&vertices));
    if (FAILED(result) || vertices == nullptr)
    {
        return E_FAIL;
    }

    DWORD* indices = nullptr;
    result = mesh->LockIndexBuffer(D3DLOCK_READONLY, reinterpret_cast<void**>(&indices));
    if (FAILED(result) || indices == nullptr)
    {
        mesh->UnlockVertexBuffer();
        return E_FAIL;
    }

    const DWORD vertexCount = mesh->GetNumVertices();
    const DWORD faceCount = mesh->GetNumFaces();
    for (DWORD vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
    {
        vertices[vertexIndex].tangent = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        vertices[vertexIndex].binormal = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    }

    bool hasInvalidIndex = false;
    constexpr float kMinimumUvDeterminant = 1.0e-12f;
    for (DWORD faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        const DWORD indexOffset = faceIndex * 3;
        const DWORD index0 = indices[indexOffset];
        const DWORD index1 = indices[indexOffset + 1];
        const DWORD index2 = indices[indexOffset + 2];
        if (index0 >= vertexCount || index1 >= vertexCount || index2 >= vertexCount)
        {
            hasInvalidIndex = true;
            break;
        }

        const TangentVertex& vertex0 = vertices[index0];
        const TangentVertex& vertex1 = vertices[index1];
        const TangentVertex& vertex2 = vertices[index2];
        const D3DXVECTOR3 edge1 = vertex1.position - vertex0.position;
        const D3DXVECTOR3 edge2 = vertex2.position - vertex0.position;
        const float deltaU1 = vertex1.texcoord.x - vertex0.texcoord.x;
        const float deltaV1 = vertex1.texcoord.y - vertex0.texcoord.y;
        const float deltaU2 = vertex2.texcoord.x - vertex0.texcoord.x;
        const float deltaV2 = vertex2.texcoord.y - vertex0.texcoord.y;
        const float determinant = deltaU1 * deltaV2 - deltaV1 * deltaU2;

        D3DXVECTOR3 tangent;
        D3DXVECTOR3 binormal;
        if (std::fabs(determinant) > kMinimumUvDeterminant)
        {
            const float inverseDeterminant = 1.0f / determinant;
            tangent = (edge1 * deltaV2 - edge2 * deltaV1) * inverseDeterminant;
            binormal = (edge2 * deltaU1 - edge1 * deltaU2) * inverseDeterminant;
        }
        else
        {
            D3DXVECTOR3 faceNormal;
            D3DXVec3Cross(&faceNormal, &edge1, &edge2);
            if (!NormalizeVector(faceNormal))
            {
                continue;
            }

            tangent = edge1;
            const float normalComponent = D3DXVec3Dot(&tangent, &faceNormal);
            tangent -= faceNormal * normalComponent;
            if (!NormalizeVector(tangent))
            {
                tangent = BuildPerpendicularVector(faceNormal);
            }
            D3DXVec3Cross(&binormal, &faceNormal, &tangent);
        }

        vertices[index0].tangent += tangent;
        vertices[index1].tangent += tangent;
        vertices[index2].tangent += tangent;
        vertices[index0].binormal += binormal;
        vertices[index1].binormal += binormal;
        vertices[index2].binormal += binormal;
    }

    if (!hasInvalidIndex)
    {
        for (DWORD vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            TangentVertex& vertex = vertices[vertexIndex];
            D3DXVECTOR3 normal = vertex.normal;
            if (!NormalizeVector(normal))
            {
                normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
            }

            D3DXVECTOR3 tangent = vertex.tangent;
            const float normalComponent = D3DXVec3Dot(&tangent, &normal);
            tangent -= normal * normalComponent;
            if (!NormalizeVector(tangent))
            {
                tangent = BuildPerpendicularVector(normal);
            }

            D3DXVECTOR3 binormal;
            D3DXVec3Cross(&binormal, &normal, &tangent);
            if (D3DXVec3Dot(&binormal, &vertex.binormal) < 0.0f)
            {
                binormal *= -1.0f;
            }
            NormalizeVector(binormal);

            vertex.normal = normal;
            vertex.tangent = tangent;
            vertex.binormal = binormal;
        }
    }

    mesh->UnlockIndexBuffer();
    mesh->UnlockVertexBuffer();
    if (hasInvalidIndex)
    {
        return E_FAIL;
    }
    return S_OK;
}

}

MeshMix2MeshAlloc::MeshMix2MeshAlloc(const std::wstring &xFilename)
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

MeshMix2MeshAlloc::~MeshMix2MeshAlloc()
{
    ClearTextureCache();
}

STDMETHODIMP MeshMix2MeshAlloc::CreateFrame(LPCSTR name, LPD3DXFRAME *newFrame)
{
    auto animFrame = NEW MeshMix2Frame();

    if (name != nullptr)
    {
        auto len = strlen(name);
        animFrame->Name = NEW char[len + 1];
        strcpy_s(animFrame->Name, len + 1, name);
    }

    D3DXMatrixIdentity(&animFrame->TransformationMatrix);
    D3DXMatrixIdentity(&animFrame->m_combinedMatrix);

    *newFrame = animFrame;

    return S_OK;
}

STDMETHODIMP MeshMix2MeshAlloc::CreateMeshContainer(LPCSTR meshName,
                                                       CONST D3DXMESHDATA *meshData,
                                                       CONST D3DXMATERIAL *materials,
                                                       CONST D3DXEFFECTINSTANCE *,
                                                       DWORD materialCount,
                                                       CONST DWORD *adjacency,
                                                       LPD3DXSKININFO,
                                                       LPD3DXMESHCONTAINER *meshContainer)
{
    m_container = NEW MeshMix2MeshContainer();

    if (meshName != nullptr)
    {
        const std::string meshFilename(meshName);
        m_container->Name = NEW char[meshFilename.length() + 1];
        strcpy_s(m_container->Name, meshFilename.length() + 1, meshFilename.c_str());
    }

    const DWORD sourceFvf = meshData->pMesh->GetFVF();
    DWORD requiredFvf = sourceFvf;
    bool needsNormal = false;
    if (!(sourceFvf & D3DFVF_NORMAL))
    {
        requiredFvf |= D3DFVF_NORMAL;
        needsNormal = true;
    }

    if (!(sourceFvf & D3DFVF_TEX1))
    {
        requiredFvf |= D3DFVF_TEX1;
    }

    HRESULT result = E_FAIL;
    if (requiredFvf != sourceFvf)
    {
        m_container->MeshData.Type = D3DXMESHTYPE_MESH;
        result = meshData->pMesh->CloneMeshFVF(meshData->pMesh->GetOptions() | D3DXMESH_32BIT,
                                      requiredFvf,
                                      Common::D3DDevice(),
                                      &m_container->MeshData.pMesh);

        if (FAILED(result))
        {
            CUSTOM_X_LOADER_LOG(L"MeshMix2 allocator failed: CloneMeshFVF. Mesh=" +
                                Util::Utf8ToWstring(meshName) +
                                L" HR=" + FormatHRESULT(result));
            return E_FAIL;
        }

        if (needsNormal)
        {
            D3DXComputeNormals(m_container->MeshData.pMesh, nullptr);
        }
    }
    else
    {
        m_container->MeshData.pMesh = meshData->pMesh;
        m_container->MeshData.Type = D3DXMESHTYPE_MESH;
        meshData->pMesh->AddRef();
    }

    D3DVERTEXELEMENT9 tangentDeclaration[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
        { 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
        D3DDECL_END()
    };

    LPD3DXMESH tangentMesh = nullptr;
    result = m_container->MeshData.pMesh->CloneMesh(D3DXMESH_MANAGED | D3DXMESH_32BIT,
                                                     tangentDeclaration,
                                                     Common::D3DDevice(),
                                                     &tangentMesh);
    if (FAILED(result) || tangentMesh == nullptr)
    {
        CUSTOM_X_LOADER_LOG(L"MeshMix2 allocator failed: tangent CloneMesh. Mesh=" +
                            Util::Utf8ToWstring(meshName) +
                            L" HR=" + FormatHRESULT(result));
        return E_FAIL;
    }

    std::vector<DWORD> tangentAdjacency(tangentMesh->GetNumFaces() * 3);
    result = tangentMesh->GenerateAdjacency(1e-6f, tangentAdjacency.data());
    if (FAILED(result))
    {
        CUSTOM_X_LOADER_LOG(L"MeshMix2 allocator failed: tangent GenerateAdjacency. Mesh=" +
                            Util::Utf8ToWstring(meshName) +
                            L" HR=" + FormatHRESULT(result));
        SAFE_RELEASE(tangentMesh);
        return E_FAIL;
    }

    result = D3DXComputeTangentFrameEx(tangentMesh,
                                       D3DDECLUSAGE_TEXCOORD,
                                       0,
                                       D3DDECLUSAGE_TANGENT,
                                       0,
                                       D3DDECLUSAGE_BINORMAL,
                                       0,
                                       D3DDECLUSAGE_NORMAL,
                                       0,
                                       D3DXTANGENT_WEIGHT_BY_AREA | D3DXTANGENT_GENERATE_IN_PLACE,
                                       tangentAdjacency.data(),
                                       0.01f,
                                       0.01f,
                                       0.0f,
                                       nullptr,
                                       nullptr);
    if (FAILED(result))
    {
        CUSTOM_X_LOADER_LOG(L"MeshMix2 allocator D3DXComputeTangentFrameEx failed; using fallback. Mesh=" +
                            Util::Utf8ToWstring(meshName) +
                            L" HR=" + FormatHRESULT(result));
        result = ComputeTangentFrameFallback(tangentMesh);
        if (FAILED(result))
        {
            CUSTOM_X_LOADER_LOG(L"MeshMix2 allocator tangent fallback failed. Mesh=" +
                                Util::Utf8ToWstring(meshName) +
                                L" HR=" + FormatHRESULT(result));
            SAFE_RELEASE(tangentMesh);
            return E_FAIL;
        }
        CUSTOM_X_LOADER_LOG(L"MeshMix2 allocator tangent fallback succeeded. Mesh=" +
                            Util::Utf8ToWstring(meshName));
    }

    SAFE_RELEASE(m_container->MeshData.pMesh);
    m_container->MeshData.pMesh = tangentMesh;

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

STDMETHODIMP MeshMix2MeshAlloc::DestroyFrame(LPD3DXFRAME frame)
{
    SAFE_DELETE_ARRAY(frame->Name);
    SAFE_DELETE(frame);
    return S_OK;
}

STDMETHODIMP MeshMix2MeshAlloc::DestroyMeshContainer(LPD3DXMESHCONTAINER meshContainerBase)
{
    auto *meshContainer = (MeshMix2MeshContainer*)meshContainerBase;

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

void MeshMix2MeshAlloc::InitializeMaterials(const DWORD materialCount,
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

void MeshMix2MeshAlloc::ClearTextureCache()
{
    for (auto& texturePair : m_textureCache)
    {
        SAFE_RELEASE(texturePair.second);
    }

    m_textureCache.clear();
}

std::wstring MeshMix2MeshAlloc::ResolveTexturePath(const char* textureFilename) const
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

LPDIRECT3DTEXTURE9 MeshMix2MeshAlloc::LoadTextureCached(const std::wstring& texturePath)
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
        const std::string message = "MeshMix2 failed to load a referenced texture: " +
                                    Util::WstringToUtf8(texturePath);
        throw std::runtime_error(message);
    }

    texture->AddRef();
    m_textureCache[texturePath] = texture;
    return texture;
}

}


