#include "MeshPBRManager.h"

#include "Util.h"
#include "Camera.h"
#include "Light.h"

#include <Shlwapi.h>
#include <algorithm>
#include <cwctype>
#include <sstream>
#include <unordered_map>
#include <utility>
#pragma comment(lib, "Shlwapi.lib")

namespace NSRender
{
namespace
{
struct CachedTexture
{
    LPDIRECT3DBASETEXTURE9 texture = nullptr;
    int userCount = 0;
};

using TextureCache = std::unordered_map<std::wstring, CachedTexture>;

float PointLightShapeToShaderValue(const PointLightShape shape)
{
    return static_cast<float>(static_cast<int>(shape));
}

D3DXCOLOR ConvertRgbDwordToColor(const DWORD color)
{
    return D3DXCOLOR(static_cast<float>((color >> 16) & 0xff) / 255.0f,
                     static_cast<float>((color >> 8) & 0xff) / 255.0f,
                     static_cast<float>(color & 0xff) / 255.0f,
                     1.0f);
}

std::wstring BuildAutoPointLightOwnerTag(const void* owner)
{
    std::wstringstream ss;
    ss << L"emit_mesh_" << owner;
    return ss.str();
}

TextureCache& GetTextureCache()
{
    static TextureCache textureCache;
    return textureCache;
}

LPD3DXEFFECT& GetSharedEffect()
{
    static LPD3DXEFFECT sharedEffect = nullptr;
    return sharedEffect;
}

long& GetSharedEffectRefCount()
{
    static long refCount = 0;
    return refCount;
}

bool& GetSharedEffectLostState()
{
    static bool lostState = false;
    return lostState;
}

LPDIRECT3DTEXTURE9& GetSharedThicknessTexture()
{
    static LPDIRECT3DTEXTURE9 sharedThicknessTexture = nullptr;
    return sharedThicknessTexture;
}

LPDIRECT3DTEXTURE9& GetSharedMirrorTexture()
{
    static LPDIRECT3DTEXTURE9 sharedMirrorTexture = nullptr;
    return sharedMirrorTexture;
}

D3DXMATRIX& GetSharedMirrorViewProj()
{
    static D3DXMATRIX sharedMirrorViewProj = []()
    {
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        return identity;
    }();
    return sharedMirrorViewProj;
}

float ClampSpecularEdge(const float edge)
{
    return (std::max)(0.0f, (std::min)(edge, 1.0f));
}

float ConvertSpecularEdgeToShaderPower(const float edge)
{
    return 1.0f + (ClampSpecularEdge(edge) * 127.0f);
}

bool ComputeMirrorPlaneFromMesh(LPD3DXMESH pMesh,
                                D3DXVECTOR3* pPlanePoint,
                                D3DXVECTOR3* pPlaneNormal)
{
    if (pMesh == nullptr || pPlanePoint == nullptr || pPlaneNormal == nullptr)
    {
        return false;
    }

    if ((pMesh->GetFVF() & D3DFVF_XYZ) == 0 || pMesh->GetNumFaces() == 0)
    {
        return false;
    }

    void* pVertices = nullptr;
    HRESULT hResult = pMesh->LockVertexBuffer(D3DLOCK_READONLY, &pVertices);
    if (FAILED(hResult))
    {
        return false;
    }

    LPDIRECT3DINDEXBUFFER9 pIndexBuffer = nullptr;
    hResult = pMesh->GetIndexBuffer(&pIndexBuffer);
    if (FAILED(hResult))
    {
        pMesh->UnlockVertexBuffer();
        return false;
    }

    void* pIndices = nullptr;
    hResult = pIndexBuffer->Lock(0, 0, &pIndices, D3DLOCK_READONLY);
    if (FAILED(hResult))
    {
        SAFE_RELEASE(pIndexBuffer);
        pMesh->UnlockVertexBuffer();
        return false;
    }

    const UINT vertexStride = pMesh->GetNumBytesPerVertex();
    const BYTE* pVertexBytes = static_cast<const BYTE*>(pVertices);
    DWORD i0 = 0;
    DWORD i1 = 0;
    DWORD i2 = 0;

    if ((pMesh->GetOptions() & D3DXMESH_32BIT) != 0)
    {
        const DWORD* pIndexData = static_cast<const DWORD*>(pIndices);
        i0 = pIndexData[0];
        i1 = pIndexData[1];
        i2 = pIndexData[2];
    }
    else
    {
        const WORD* pIndexData = static_cast<const WORD*>(pIndices);
        i0 = pIndexData[0];
        i1 = pIndexData[1];
        i2 = pIndexData[2];
    }

    const D3DXVECTOR3& v0 = *reinterpret_cast<const D3DXVECTOR3*>(pVertexBytes + vertexStride * i0);
    const D3DXVECTOR3& v1 = *reinterpret_cast<const D3DXVECTOR3*>(pVertexBytes + vertexStride * i1);
    const D3DXVECTOR3& v2 = *reinterpret_cast<const D3DXVECTOR3*>(pVertexBytes + vertexStride * i2);

    const D3DXVECTOR3 edge1 = v1 - v0;
    const D3DXVECTOR3 edge2 = v2 - v0;
    D3DXVECTOR3 normal;
    D3DXVec3Cross(&normal, &edge1, &edge2);

    const bool isValidNormal = D3DXVec3LengthSq(&normal) > 0.0f;
    if (isValidNormal)
    {
        D3DXVec3Normalize(&normal, &normal);
        *pPlanePoint = v0;
        *pPlaneNormal = normal;
    }

    pIndexBuffer->Unlock();
    SAFE_RELEASE(pIndexBuffer);
    pMesh->UnlockVertexBuffer();
    return isValidNormal;
}

float ConvertXMaterialPowerToShaderPower(const float materialPower)
{
    float clampedPower = (std::max)(0.0f, (std::min)(materialPower, 255.0f));
    return clampedPower;
}

float ConvertXMaterialPowerToSpecularIntensity(const float materialPower)
{
    const float clampedPower = (std::max)(0.0f, (std::min)(materialPower, 255.0f));
    return (clampedPower / 255.0f) * 0.5f;
}

float GetMaterialSpecularIntensity(const D3DMATERIAL9& material)
{
    if (true)
    {
        return ConvertXMaterialPowerToSpecularIntensity(material.Power);
    }
    else
    {
        return (std::max)(material.Specular.r,
               (std::max)(material.Specular.g, material.Specular.b));
    }
}

std::wstring ToLowerString(std::wstring text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t ch)
    {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return text;
}

bool ContainsToken(const std::wstring& text, const std::wstring& token)
{
    return text.find(token) != std::wstring::npos;
}

std::wstring NormalizeTextureCacheKey(const std::wstring& texturePath)
{
    std::wstring normalized = texturePath;
    std::replace(normalized.begin(), normalized.end(), L'/', L'\\');
    return ToLowerString(normalized);
}

HRESULT LoadTextureCached(const std::wstring& texturePath, LPDIRECT3DBASETEXTURE9* texture)
{
    if (texture == nullptr)
    {
        return E_POINTER;
    }

    *texture = nullptr;

    TextureCache& textureCache = GetTextureCache();
    const std::wstring cacheKey = NormalizeTextureCacheKey(texturePath);
    const auto found = textureCache.find(cacheKey);
    if (found != textureCache.end())
    {
        *texture = found->second.texture;
        if (*texture != nullptr)
        {
            ++found->second.userCount;
            (*texture)->AddRef();
        }
        return S_OK;
    }

    LPDIRECT3DTEXTURE9 loadedTexture = nullptr;

    HRESULT hr = E_FAIL;

    if (true)
    {
        hr = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                       texturePath.c_str(),
                                       &loadedTexture);
    }
    else
    {
        hr = D3DXCreateTextureFromFileEx(Common::D3DDevice(),
                                         texturePath.c_str(),
                                         D3DX_DEFAULT,
                                         D3DX_DEFAULT,
                                         1,
                                         0,
                                         D3DFMT_UNKNOWN,
                                         D3DPOOL_MANAGED,
                                         D3DX_DEFAULT,
                                         D3DX_DEFAULT,
                                         0,
                                         nullptr,
                                         nullptr,
                                         &loadedTexture);
    }
    
    if (FAILED(hr))
    {
        return hr;
    }

    CachedTexture cachedTexture;
    cachedTexture.texture = loadedTexture;
    cachedTexture.userCount = 1;
    textureCache.emplace(cacheKey, cachedTexture);
    loadedTexture->AddRef();
    *texture = loadedTexture;
    return S_OK;
}

HRESULT LoadCubeTextureCached(const std::wstring& texturePath, LPDIRECT3DBASETEXTURE9* texture)
{
    if (texture == nullptr)
    {
        return E_POINTER;
    }

    *texture = nullptr;

    TextureCache& textureCache = GetTextureCache();
    const std::wstring cacheKey = NormalizeTextureCacheKey(texturePath) + L"__cube";
    const auto found = textureCache.find(cacheKey);
    if (found != textureCache.end())
    {
        *texture = found->second.texture;
        if (*texture != nullptr)
        {
            ++found->second.userCount;
            (*texture)->AddRef();
        }
        return S_OK;
    }

    LPDIRECT3DCUBETEXTURE9 loadedTexture = nullptr;
    const HRESULT hr = D3DXCreateCubeTextureFromFile(Common::D3DDevice(),
                                                     texturePath.c_str(),
                                                     &loadedTexture);
    if (FAILED(hr))
    {
        return hr;
    }

    CachedTexture cachedTexture;
    cachedTexture.texture = loadedTexture;
    cachedTexture.userCount = 1;
    textureCache.emplace(cacheKey, cachedTexture);
    loadedTexture->AddRef();
    *texture = loadedTexture;
    return S_OK;
}

void ReleaseTextureCached(LPDIRECT3DBASETEXTURE9& texture)
{
    if (texture == nullptr)
    {
        return;
    }

    TextureCache& textureCache = GetTextureCache();
    for (auto it = textureCache.begin(); it != textureCache.end(); ++it)
    {
        if (it->second.texture != texture)
        {
            continue;
        }

        if (it->second.userCount > 0)
        {
            --it->second.userCount;
        }

        SAFE_RELEASE(texture);

        if (it->second.userCount == 0)
        {
            LPDIRECT3DBASETEXTURE9 cachedTexture = it->second.texture;
            SAFE_RELEASE(cachedTexture);
            textureCache.erase(it);
        }
        return;
    }

    SAFE_RELEASE(texture);
}

enum class eMeshTextureRole
{
    Diffuse,
    Normal,
    Height,
    Cube
};

eMeshTextureRole ClassifyTextureRole(const std::wstring& textureFileName)
{
    const std::wstring lower = ToLowerString(textureFileName);

    if (ContainsToken(lower, L"normal") || ContainsToken(lower, L"nrm") || ContainsToken(lower, L"bump"))
    {
        return eMeshTextureRole::Normal;
    }

    if (ContainsToken(lower, L"height") || ContainsToken(lower, L"disp") || ContainsToken(lower, L"parallax"))
    {
        return eMeshTextureRole::Height;
    }

    if (ContainsToken(lower, L"cubemap") || ContainsToken(lower, L"cube_map") || ContainsToken(lower, L"env"))
    {
        return eMeshTextureRole::Cube;
    }

    return eMeshTextureRole::Diffuse;
}

HRESULT EnsureSharedEffectCreated(const std::wstring& shaderFilename)
{
    if (GetSharedEffect() != nullptr)
    {
        return S_OK;
    }

    const std::wstring shaderPath = Util::GetExeDir() + shaderFilename;
    return D3DXCreateEffectFromFile(Common::D3DDevice(),
                                    shaderPath.c_str(),
                                    nullptr,
                                    nullptr,
                                    D3DXSHADER_OPTIMIZATION_LEVEL3,
                                    nullptr,
                                    &GetSharedEffect(),
                                    nullptr);
}

void AddSharedEffectRef()
{
    ++GetSharedEffectRefCount();
}

void ReleaseSharedEffectRef()
{
    long& refCount = GetSharedEffectRefCount();
    if (refCount > 0)
    {
        --refCount;
    }

    if (refCount == 0)
    {
        SAFE_RELEASE(GetSharedEffect());
        GetSharedEffectLostState() = false;
    }
}

}

MeshPBRManager::MeshPBRManager(const std::wstring& filename,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rotate,
                               const float scale,
                               const stMeshPBRParam& param)
    : m_meshName(filename)
    , m_pos(pos)
    , m_rotate(rotate)
    , m_scale(scale)
    , m_param(param)
{
}

MeshPBRManager::~MeshPBRManager()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }
    Finalize();
}

MeshPBRManager::MeshPBRManager(MeshPBRManager&& other) noexcept
    : SHADER_FILENAME(other.SHADER_FILENAME)
{
    *this = std::move(other);
}

MeshPBRManager& MeshPBRManager::operator=(MeshPBRManager&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Finalize();

    m_meshName = std::move(other.m_meshName);
    m_D3DMesh = other.m_D3DMesh;
    other.m_D3DMesh = nullptr;

    m_materialCount = other.m_materialCount;
    m_subsetCount = other.m_subsetCount;
    m_vecDiffuse = std::move(other.m_vecDiffuse);
    m_vecSpecularIntensity = std::move(other.m_vecSpecularIntensity);
    m_vecSpecularPower = std::move(other.m_vecSpecularPower);
    m_vecTexture = std::move(other.m_vecTexture);

    m_texCubeMap = other.m_texCubeMap;
    other.m_texCubeMap = nullptr;
    m_texNormalMap = other.m_texNormalMap;
    other.m_texNormalMap = nullptr;
    m_texHeightMap = other.m_texHeightMap;
    other.m_texHeightMap = nullptr;

    m_pos = other.m_pos;
    m_rotate = other.m_rotate;
    m_scale = other.m_scale;
    m_bLoaded = other.m_bLoaded.load();
    m_enabled = other.m_enabled;
    m_autoPointLightAdded = other.m_autoPointLightAdded;
    m_deviceResourceRegistered = other.m_deviceResourceRegistered;
    m_hasMirrorPlane = other.m_hasMirrorPlane;
    m_param = other.m_param;
    m_autoPointLightOwnerTag = std::move(other.m_autoPointLightOwnerTag);
    m_mirrorPlanePointLocal = other.m_mirrorPlanePointLocal;
    m_mirrorPlaneNormalLocal = other.m_mirrorPlaneNormalLocal;
    m_loadThread = std::move(other.m_loadThread);

    other.m_materialCount = 0;
    other.m_subsetCount = 0;
    other.m_bLoaded = false;
    other.m_enabled = false;
    other.m_autoPointLightAdded = false;
    other.m_deviceResourceRegistered = false;
    other.m_hasMirrorPlane = false;

    return *this;
}

void MeshPBRManager::Initialize(bool async)
{
    // シェーダーの作成はメインスレッドで行う必要があるため、ここで実行する
    HRESULT hResult = EnsureSharedEffectCreated(SHADER_FILENAME);
    assert(hResult == S_OK);

    if (m_bLoaded)
    {
        return;
    }

    AddSharedEffectRef();

    if (async)
    {
        if (m_loadThread.joinable())
        {
            m_loadThread.join();
        }
        m_loadThread = std::thread([this]() { InitializeInternal(); });
    }
    else
    {
        InitializeInternal();
    }
}

void MeshPBRManager::WaitForLoad()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }
}

void MeshPBRManager::InitializeInternal()
{
    HRESULT hResult = S_OK;

    LPD3DXBUFFER adjacencyBuffer = nullptr;
    LPD3DXBUFFER materialBuffer = nullptr;

    std::wstring tempPath;
    if (PathIsRelative(m_meshName.c_str()))
    {
        tempPath = Util::GetExeDir() + m_meshName;
    }
    else
    {
        tempPath = m_meshName;
    }

    hResult = D3DXLoadMeshFromX(tempPath.c_str(),
                                D3DXMESH_MANAGED,
                                Common::D3DDevice(),
                                &adjacencyBuffer,
                                &materialBuffer,
                                nullptr,
                                &m_materialCount,
                                &m_D3DMesh);

    assert(hResult == S_OK);

    if (m_param.mirror)
    {
        m_hasMirrorPlane = ComputeMirrorPlaneFromMesh(m_D3DMesh,
                                                      &m_mirrorPlanePointLocal,
                                                      &m_mirrorPlaneNormalLocal);
    }
    else
    {
        m_hasMirrorPlane = false;
    }

    DWORD* adjacencyList = static_cast<DWORD*>(adjacencyBuffer->GetBufferPointer());

    ModifyMeshForNormalMapping(m_D3DMesh);

    hResult = m_D3DMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE,
                                         adjacencyList,
                                         nullptr,
                                         nullptr,
                                         nullptr);

    SAFE_RELEASE(adjacencyBuffer);
    assert(hResult == S_OK);

    DWORD subsetCount = 0;
    hResult = m_D3DMesh->GetAttributeTable(nullptr, &subsetCount);
    assert(SUCCEEDED(hResult));
    if (subsetCount > 0)
    {
        m_subsetCount = subsetCount;
    }
    else
    {
        m_subsetCount = m_materialCount;
    }

    D3DXMATERIAL* materialList = static_cast<D3DXMATERIAL*>(materialBuffer->GetBufferPointer());

    std::wstring xFileDir = tempPath;
    const std::size_t lastPos = xFileDir.find_last_of(L"\\");
    xFileDir = xFileDir.substr(0, lastPos + 1);

    std::vector<D3DXVECTOR4> diffuseList;
    std::vector<float> specularIntensityList;
    std::vector<float> specularPowerList;
    std::vector<LPDIRECT3DBASETEXTURE9> diffuseTextureList;

    if (!m_param.envMapTexturePath.empty())
    {
        const std::wstring envMapPath = PathIsRelative(m_param.envMapTexturePath.c_str())
                                      ? xFileDir + m_param.envMapTexturePath
                                      : m_param.envMapTexturePath;
        hResult = LoadCubeTextureCached(envMapPath, &m_texCubeMap);
        assert(hResult == S_OK);
        m_param.cubeMapping = true;
    }

    for (DWORD i = 0; i < m_materialCount; ++i)
    {
        D3DXVECTOR4 diffuse(1.0f, 1.0f, 1.0f, 1.0f);
        diffuse.x = materialList[i].MatD3D.Diffuse.r;
        diffuse.y = materialList[i].MatD3D.Diffuse.g;
        diffuse.z = materialList[i].MatD3D.Diffuse.b;
        diffuse.w = materialList[i].MatD3D.Diffuse.a;
        const float specularIntensity = GetMaterialSpecularIntensity(materialList[i].MatD3D);
        const float specularPower = ConvertXMaterialPowerToShaderPower(materialList[i].MatD3D.Power);

        LPDIRECT3DBASETEXTURE9 tempTexture = nullptr;
        std::wstring textureFileName;

        if (materialList[i].pTextureFilename != nullptr &&
            strlen(materialList[i].pTextureFilename) != 0)
        {
            textureFileName = Util::Utf8ToWstring(materialList[i].pTextureFilename);
            const std::wstring texturePath = xFileDir + textureFileName;
            const std::wstring lowerExt = ToLowerString(textureFileName.substr(textureFileName.find_last_of(L'.') + 1));
            if ((m_param.cubeMapping || m_param.glass) && lowerExt == L"dds")
            {
                hResult = LoadCubeTextureCached(texturePath, &tempTexture);
            }
            else
            {
                hResult = LoadTextureCached(texturePath, &tempTexture);
            }
            assert(hResult == S_OK);
        }

        std::wstring lowerExtRole = L"";
        if (textureFileName.find_last_of(L'.') != std::wstring::npos)
        {
            lowerExtRole = ToLowerString(textureFileName.substr(textureFileName.find_last_of(L'.') + 1));
        }
        const bool isCubeByEnvMap =
            ((m_param.cubeMapping || m_param.glass) && lowerExtRole == L"dds");
        // 共有ローダでも単体版と同じ命名規約を使い、
        // ディフューズ以外の補助テクスチャを自動判定する。
        eMeshTextureRole textureRole = ClassifyTextureRole(textureFileName);
        if (isCubeByEnvMap)
        {
            textureRole = eMeshTextureRole::Cube;
        }
        if (textureRole == eMeshTextureRole::Normal)
        {
            if (m_texNormalMap == nullptr)
            {
                m_texNormalMap = tempTexture;
            }
            else
            {
                ReleaseTextureCached(tempTexture);
            }
        }
        else if (textureRole == eMeshTextureRole::Height)
        {
            if (m_texHeightMap == nullptr)
            {
                m_texHeightMap = tempTexture;
            }
            else
            {
                ReleaseTextureCached(tempTexture);
            }
        }
        else if (textureRole == eMeshTextureRole::Cube)
        {
            if (m_texCubeMap == nullptr)
            {
                m_texCubeMap = tempTexture;
            }
            else
            {
                ReleaseTextureCached(tempTexture);
            }
        }
        else
        {
            diffuseList.push_back(diffuse);
            specularIntensityList.push_back(specularIntensity);
            specularPowerList.push_back(specularPower);
            diffuseTextureList.push_back(tempTexture);
        }
    }

    if (!diffuseList.empty())
    {
        m_vecDiffuse = diffuseList;
        m_vecSpecularIntensity = specularIntensityList;
        m_vecSpecularPower = specularPowerList;
        m_vecTexture = diffuseTextureList;
        m_subsetCount = (std::min)(m_subsetCount, static_cast<DWORD>(m_vecDiffuse.size()));
    }
    else
    {
        m_vecDiffuse.assign(m_subsetCount, D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f));
        m_vecSpecularIntensity.assign(m_subsetCount, m_param.specularIntensity);
        m_vecSpecularPower.assign(m_subsetCount, ConvertSpecularEdgeToShaderPower(m_param.specularEdge));
        m_vecTexture.assign(m_subsetCount, nullptr);
    }

    SAFE_RELEASE(materialBuffer);

    if (!m_deviceResourceRegistered)
    {
        Common::AddDeviceLostResource(this);
        m_deviceResourceRegistered = true;
    }

    if (m_param.emit && !m_autoPointLightAdded)
    {
        if (m_autoPointLightOwnerTag.empty())
        {
            m_autoPointLightOwnerTag = BuildAutoPointLightOwnerTag(this);
        }
        Light::AddPointLight(m_pos,
                             ConvertRgbDwordToColor(m_param.emitColor),
                             m_param.emitIntensity,
                             PointLightShape::Point,
                             12.0f,
                             10.0f,
                             10.0f,
                             D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                             m_autoPointLightOwnerTag);
        m_autoPointLightAdded = true;
    }

    m_bLoaded = true;
}

void MeshPBRManager::Finalize()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }

    if (!m_bLoaded && m_D3DMesh == nullptr && m_texCubeMap == nullptr && m_texNormalMap == nullptr && m_texHeightMap == nullptr &&
        !m_autoPointLightAdded)
    {
        return;
    }

    ReleaseOwnedResources();
    if (m_autoPointLightAdded)
    {
        Light::RemovePointLightsByOwnerTag(m_autoPointLightOwnerTag);
        m_autoPointLightAdded = false;
    }
    ReleaseSharedEffectRef();
    if (m_deviceResourceRegistered)
    {
        Common::RemoveDeviceLostResource(this);
        m_deviceResourceRegistered = false;
    }
    m_bLoaded = false;
}

void MeshPBRManager::ReleaseOwnedResources()
{
    SAFE_RELEASE(m_D3DMesh);

    for (auto& texture : m_vecTexture)
    {
        ReleaseTextureCached(texture);
    }
    m_vecTexture.clear();
    m_vecDiffuse.clear();
    m_vecSpecularIntensity.clear();
    m_vecSpecularPower.clear();

    ReleaseTextureCached(m_texCubeMap);
    ReleaseTextureCached(m_texNormalMap);
    ReleaseTextureCached(m_texHeightMap);

    m_materialCount = 0;
    m_subsetCount = 0;
}

void MeshPBRManager::ModifyMeshForNormalMapping(LPD3DXMESH& pMesh)
{
    D3DVERTEXELEMENT9 declTB[] =
    {
        {0,  0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0},
        {0, 12,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,    0},
        {0, 24,  D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0},
        {0, 32,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,   0},
        {0, 44,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,  0},
        D3DDECL_END()
    };

    LPD3DXMESH pCloned = nullptr;
    HRESULT hr = pMesh->CloneMesh(D3DXMESH_MANAGED | D3DXMESH_32BIT,
                                  declTB,
                                  Common::D3DDevice(),
                                  &pCloned);
    assert(SUCCEEDED(hr));

    std::vector<DWORD> adj(pCloned->GetNumFaces() * 3);
    hr = pCloned->GenerateAdjacency(1e-6f, adj.data());
    assert(SUCCEEDED(hr));

    DWORD options = D3DXTANGENT_WEIGHT_BY_AREA | D3DXTANGENT_GENERATE_IN_PLACE;
    if (m_param.smooth)
    {
        options += D3DXTANGENT_CALCULATE_NORMALS;
    }

    // ノーマルマップ使用時は接線計算の破綻を避けるため、
    // 法線の角度差に少し余裕を持たせる。
    float normalEdgeThreshold = 0.0f;
    if (m_param.parallaxOcclusionMapping)
    {
        normalEdgeThreshold = 0.999f;
    }

    hr = D3DXComputeTangentFrameEx(pCloned,
                                   D3DDECLUSAGE_TEXCOORD, 0,
                                   D3DDECLUSAGE_TANGENT, 0,
                                   D3DDECLUSAGE_BINORMAL, 0,
                                   D3DDECLUSAGE_NORMAL, 0,
                                   options,
                                   adj.data(),
                                   0.01f,
                                   0.01f,
                                   normalEdgeThreshold,
                                   nullptr,
                                   nullptr);

    if (FAILED(hr))
    {
        SAFE_RELEASE(pCloned);
        return;
    }

    pMesh->Release();
    pMesh = pCloned;
}

void MeshPBRManager::SetPos(const D3DXVECTOR3& pos)
{
    m_pos = pos;

    if (m_autoPointLightAdded)
    {
        Light::RemovePointLightsByOwnerTag(m_autoPointLightOwnerTag);
        Light::AddPointLight(m_pos,
                             ConvertRgbDwordToColor(m_param.emitColor),
                             m_param.emitIntensity,
                             PointLightShape::Point,
                             12.0f,
                             10.0f,
                             10.0f,
                             D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                             m_autoPointLightOwnerTag);
    }
}

void MeshPBRManager::SetSaturateShadow(const bool enabled)
{
    m_param.saturateShadow = enabled;
}

void MeshPBRManager::SetSaturateShadowIntensity(const float intensity)
{
    m_param.saturateShadowIntensity = intensity;
}

void MeshPBRManager::SetShadowDarkness(const float darkness)
{
    m_param.shadowDarkness = darkness;
}

void MeshPBRManager::SetSpecularIntensity(const float intensity)
{
    m_param.specularIntensity = intensity;
}

void MeshPBRManager::SetSharedThicknessTexture(LPDIRECT3DTEXTURE9 texture)
{
    GetSharedThicknessTexture() = texture;

    LPD3DXEFFECT sharedEffect = GetSharedEffect();
    if (sharedEffect != nullptr)
    {
        sharedEffect->SetTexture("g_texThickness", texture);
    }
}

void MeshPBRManager::SetSharedMirrorTexture(LPDIRECT3DTEXTURE9 texture)
{
    GetSharedMirrorTexture() = texture;

    LPD3DXEFFECT sharedEffect = GetSharedEffect();
    if (sharedEffect != nullptr)
    {
        sharedEffect->SetTexture("g_texMirror", texture);
    }
}

void MeshPBRManager::SetSharedMirrorViewProj(const D3DXMATRIX& matrix)
{
    GetSharedMirrorViewProj() = matrix;

    LPD3DXEFFECT sharedEffect = GetSharedEffect();
    if (sharedEffect != nullptr)
    {
        sharedEffect->SetMatrix("g_matMirrorViewProj", &matrix);
    }
}

void MeshPBRManager::SetSpecularIntensityOverrideEnabled(const bool enabled)
{
    m_param.specularIntensityOverrideEnabled = enabled;
}

void MeshPBRManager::SetSpecularEdge(const float edge)
{
    m_param.specularEdge = edge;
}

void MeshPBRManager::SetCubeMappingRate(const float rate)
{
    m_param.cubeMappingRate = rate;
}

void MeshPBRManager::SetPBRRoughness(const float roughness)
{
    m_param.pbrRoughness = roughness;
}

void MeshPBRManager::SetPBRMetallic(const float metallic)
{
    m_param.pbrMetallic = metallic;
}

void MeshPBRManager::SetPBREnvReflectionIntensity(const float intensity)
{
    m_param.envReflectionIntensity = intensity;
}

void MeshPBRManager::SetPBREnvMaxMipLevel(const float mipLevel)
{
    m_param.envMaxMipLevel = mipLevel;
}

void MeshPBRManager::SetPBREnvDiffuseIntensity(const float intensity)
{
    m_param.envDiffuseIntensity = intensity;
}

void MeshPBRManager::SetPBREnvDiffuseMipLevel(const float mipLevel)
{
    m_param.envDiffuseMipLevel = mipLevel;
}

void MeshPBRManager::SetSpecularEdgeOverrideEnabled(const bool enabled)
{
    m_param.specularEdgeOverrideEnabled = enabled;
}

void MeshPBRManager::SetRotY(const float rotY)
{
    m_rotate.y = rotY;
}

D3DXVECTOR3 MeshPBRManager::GetRot() const
{
    return m_rotate;
}

D3DXVECTOR3 MeshPBRManager::GetPos() const
{
    return m_pos;
}

float MeshPBRManager::GetScale() const
{
    return m_scale;
}

D3DXMATRIX MeshPBRManager::BuildWorldMatrix() const
{
    D3DXMATRIX matWorld { };
    D3DXMatrixIdentity(&matWorld);

    D3DXMATRIX matWork;
    D3DXMatrixIdentity(&matWork);

    D3DXMatrixScaling(&matWork, m_scale, m_scale, m_scale);
    matWorld *= matWork;

    D3DXMatrixRotationYawPitchRoll(&matWork, m_rotate.y, m_rotate.x, m_rotate.z);
    matWorld *= matWork;

    D3DXMatrixTranslation(&matWork, m_pos.x, m_pos.y, m_pos.z);
    matWorld *= matWork;

    return matWorld;
}

DWORD MeshPBRManager::GetSubsetCount() const
{
    return m_subsetCount;
}

bool MeshPBRManager::IsEnabled() const
{
    return m_enabled;
}

void MeshPBRManager::SetEnabled(const bool enabled)
{
    m_enabled = enabled;
}

void MeshPBRManager::SetSSS(const bool enabled)
{
    m_param.sss = enabled;
}

void MeshPBRManager::SetSSSIntensity(const float intensity)
{
    m_param.sssIntensity = intensity;
}

void MeshPBRManager::SetSSSColor(const DWORD color)
{
    m_param.sssColor = color;
}

bool MeshPBRManager::IsLoaded() const
{
    return m_bLoaded;
}

bool MeshPBRManager::IsSsaoEnabled() const
{
    return m_param.ssao;
}

bool MeshPBRManager::IsDepthBufferShadowEnabled() const
{
    return m_param.shadow;
}

bool MeshPBRManager::IsMirror() const
{
    return m_param.mirror;
}

bool MeshPBRManager::TryGetMirrorPlaneWorld(D3DXVECTOR3& planePoint, D3DXVECTOR3& planeNormal) const
{
    if (!m_bLoaded || !m_param.mirror || !m_hasMirrorPlane)
    {
        return false;
    }

    const D3DXMATRIX matWorld = BuildWorldMatrix();
    D3DXVec3TransformCoord(&planePoint, &m_mirrorPlanePointLocal, &matWorld);
    D3DXVec3TransformNormal(&planeNormal, &m_mirrorPlaneNormalLocal, &matWorld);
    if (D3DXVec3LengthSq(&planeNormal) <= 0.0f)
    {
        return false;
    }

    D3DXVec3Normalize(&planeNormal, &planeNormal);
    return true;
}

void MeshPBRManager::Render(const bool renderAsMirrorSurface)
{
    if (!m_bLoaded || !m_enabled)
    {
        return;
    }

    LPD3DXEFFECT sharedEffect = GetSharedEffect();
    if (sharedEffect == nullptr)
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    D3DXVECTOR4 normal = Light::GetLightDir();
    D3DXVECTOR4 lightColor = D3DXVECTOR4(Light::GetLightColor());
    D3DXVECTOR4 ambientColor = D3DXVECTOR4(Light::GetAmbientColor());
    hResult = sharedEffect->SetVector("g_lightDir", &normal);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetVector("g_lightColor", &lightColor);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetVector("g_ambient", &ambientColor);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());
    assert(hResult == S_OK);

    const D3DXMATRIX matWorld = BuildWorldMatrix();

    hResult = sharedEffect->SetMatrix("g_matWorld", &matWorld);

    D3DXMATRIX matViewProj{ };
    D3DXMatrixIdentity(&matViewProj);
    matViewProj *= Camera::GetViewMatrix();
    matViewProj *= Camera::GetProjMatrix();
    hResult = sharedEffect->SetMatrix("g_matViewProj", &matViewProj);

    D3DXMATRIX worldViewProjMatrix{ };
    D3DXMatrixIdentity(&worldViewProjMatrix);
    worldViewProjMatrix = matWorld * matViewProj;

    hResult = sharedEffect->SetMatrix("g_matWorldViewProj", &worldViewProjMatrix);
    assert(hResult == S_OK);

    D3DXVECTOR4 cameraPos = D3DXVECTOR4(Camera::GetEyePos(), 1.f);
    hResult = sharedEffect->SetVector("g_cameraPos", &cameraPos);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    hResult = sharedEffect->Begin(nullptr, 0);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetTexture("g_texCubeMap", m_texCubeMap);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetTexture("g_texNormalMap", m_texNormalMap);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetTexture("g_texHeightMap", m_texHeightMap);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetTexture("g_texThickness", GetSharedThicknessTexture());
    assert(hResult == S_OK);

    hResult = sharedEffect->SetTexture("g_texMirror", GetSharedMirrorTexture());
    assert(hResult == S_OK);

    const D3DXMATRIX sharedMirrorViewProj = GetSharedMirrorViewProj();
    hResult = sharedEffect->SetMatrix("g_matMirrorViewProj", &sharedMirrorViewProj);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetBool("g_hasNormalTexture", m_texNormalMap != nullptr ? TRUE : FALSE);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetBool("g_hasEnvTexture", m_texCubeMap != nullptr ? TRUE : FALSE);
    assert(hResult == S_OK);

    D3DXVECTOR4 pbrBaseColor(m_param.pbrBaseColorFactor.r,
                             m_param.pbrBaseColorFactor.g,
                             m_param.pbrBaseColorFactor.b,
                             m_param.pbrBaseColorFactor.a);
    hResult = sharedEffect->SetVector("g_pbrBaseColorFactor", &pbrBaseColor);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_pbrRoughness", m_param.pbrRoughness);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_pbrMetallic", m_param.pbrMetallic);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetBool("g_enableSrgbToLinear", m_param.pbrEnableSrgbToLinear ? TRUE : FALSE);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetBool("g_enableLinearToSrgb", m_param.pbrEnableLinearToSrgb ? TRUE : FALSE);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_envReflectionIntensity", m_param.envReflectionIntensity);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_envMaxMipLevel", m_param.envMaxMipLevel);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_envDiffuseIntensity", m_param.envDiffuseIntensity);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_envDiffuseMipLevel", m_param.envDiffuseMipLevel);
    assert(hResult == S_OK);

    BOOL usePom = FALSE;
    if (m_param.parallaxOcclusionMapping)
    {
        usePom = TRUE;
    }
    hResult = sharedEffect->SetBool("g_bPOM", usePom);
    assert(hResult == S_OK);

    BOOL useNormalMapping = FALSE;
    if (m_param.normalMapping)
    {
        useNormalMapping = TRUE;
    }
    hResult = sharedEffect->SetBool("g_bNormalMapping", useNormalMapping);
    assert(hResult == S_OK);

    BOOL useSss = FALSE;
    if (m_param.sss)
    {
        useSss = TRUE;
    }
    hResult = sharedEffect->SetBool("g_bSSS", useSss);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_sssIntensity", m_param.sssIntensity);
    assert(hResult == S_OK);

    D3DXVECTOR4 sssColor;
    sssColor.x = static_cast<float>((m_param.sssColor >> 16) & 0xff) / 255.0f;
    sssColor.y = static_cast<float>((m_param.sssColor >> 8) & 0xff) / 255.0f;
    sssColor.z = static_cast<float>(m_param.sssColor & 0xff) / 255.0f;
    sssColor.w = static_cast<float>((m_param.sssColor >> 24) & 0xff) / 255.0f;
    hResult = sharedEffect->SetVector("g_sssColor", &sssColor);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_specularIntensity", m_param.specularIntensity);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_cubeMappingRate", m_param.cubeMappingRate);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_cubeMappingGauss", m_param.cubeMappingGauss);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_emitIntensity", m_param.emitIntensity);
    assert(hResult == S_OK);

    D3DXVECTOR4 emitColor(static_cast<float>((m_param.emitColor >> 16) & 0xff) / 255.0f,
                          static_cast<float>((m_param.emitColor >> 8) & 0xff) / 255.0f,
                          static_cast<float>(m_param.emitColor & 0xff) / 255.0f,
                          1.0f);
    hResult = sharedEffect->SetVector("g_emitColor", &emitColor);
    assert(hResult == S_OK);

    static float f = 0.f;
    f += 0.001f;
    hResult = sharedEffect->SetFloat("g_time", f);
    assert(hResult == S_OK);

    if (m_param.sway)
    {
        hResult = sharedEffect->SetBool("g_swayEnable", TRUE);
        hResult = sharedEffect->SetFloat("g_swayAmount", 2.5f);
        assert(hResult == S_OK);
        hResult = sharedEffect->SetFloat("g_swaySpeed", 1.0f);
        assert(hResult == S_OK);
    }

    auto pointLightList = Light::GetPointLightList();

    D3DXVECTOR4 pos[16];
    float brightness[16] { };
    float shape[16] { };
    float lineLength[16] { };
    float squareWidth[16] { };
    float squareHeight[16] { };
    D3DXVECTOR4 rotation[16];
    D3DXVECTOR4 color[16];

    ZeroMemory(pos, sizeof(pos));
    ZeroMemory(rotation, sizeof(rotation));
    ZeroMemory(color, sizeof(color));

    if (m_param.pointLight)
    {
        for (int i = 0; i < 16; ++i)
        {
            if (i < pointLightList.size())
            {
                pos[i].x = pointLightList.at(i).m_pos.x;
                pos[i].y = pointLightList.at(i).m_pos.y;
                pos[i].z = pointLightList.at(i).m_pos.z;
                brightness[i] = pointLightList.at(i).m_brightness;
                shape[i] = PointLightShapeToShaderValue(pointLightList.at(i).m_shape);
                lineLength[i] = pointLightList.at(i).m_lineLength;
                squareWidth[i] = pointLightList.at(i).m_squareWidth;
                squareHeight[i] = pointLightList.at(i).m_squareHeight;
                rotation[i].x = pointLightList.at(i).m_rotation.x;
                rotation[i].y = pointLightList.at(i).m_rotation.y;
                rotation[i].z = pointLightList.at(i).m_rotation.z;
                color[i].x = pointLightList.at(i).m_color.r;
                color[i].y = pointLightList.at(i).m_color.g;
                color[i].z = pointLightList.at(i).m_color.b;
            }
        }
    }

    hResult = sharedEffect->SetVectorArray("g_pointLightPos", pos, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloatArray("g_pointLightBrightness", brightness, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloatArray("g_pointLightShape", shape, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloatArray("g_pointLightLineLength", lineLength, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloatArray("g_pointLightSquareWidth", squareWidth, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloatArray("g_pointLightSquareHeight", squareHeight, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetVectorArray("g_pointLightRotation", rotation, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetVectorArray("g_pointLightColor", color, 16);
    assert(hResult == S_OK);

    hResult = sharedEffect->CommitChanges();
    assert(hResult == S_OK);

    if (renderAsMirrorSurface && m_param.mirror)
    {
        DrawAllSubsets(sharedEffect, 3);
    }
    else if (m_param.emit)
    {
        DrawAllSubsets(sharedEffect, 2);
    }
    else
    {
        DrawAllSubsets(sharedEffect, 0);
    }

    hResult = sharedEffect->End();
    assert(hResult == S_OK);
}

void MeshPBRManager::DrawAllSubsets(LPD3DXEFFECT sharedEffect, const UINT passIndex)
{
    HRESULT hResult = sharedEffect->BeginPass(passIndex);
    assert(hResult == S_OK);

    DWORD subsetCount = 1;
    if (m_subsetCount > 0)
    {
        subsetCount = m_subsetCount;
    }
    for (DWORD subsetIndex = 0; subsetIndex < subsetCount; ++subsetIndex)
    {
        const D3DXVECTOR4 diffuse = GetSubsetDiffuse(subsetIndex);
        hResult = sharedEffect->SetVector("g_diffuse", &diffuse);
        assert(hResult == S_OK);

        hResult = sharedEffect->SetBool("g_hasDiffuseTexture", GetSubsetTexture(subsetIndex) != nullptr ? TRUE : FALSE);
        assert(hResult == S_OK);

        hResult = sharedEffect->SetFloat("g_specularIntensity", GetSubsetSpecularIntensity(subsetIndex));
        assert(hResult == S_OK);

        hResult = sharedEffect->SetTexture("g_texture", GetSubsetTexture(subsetIndex));
        assert(hResult == S_OK);

        hResult = sharedEffect->CommitChanges();
        assert(hResult == S_OK);

        hResult = m_D3DMesh->DrawSubset(subsetIndex);
        assert(hResult == S_OK);
    }

    hResult = sharedEffect->EndPass();
    assert(hResult == S_OK);
}

LPD3DXMESH MeshPBRManager::GetD3DMesh() const
{
    return m_D3DMesh;
}

D3DXVECTOR4 MeshPBRManager::GetSubsetDiffuse(const DWORD subsetIndex) const
{
    if (subsetIndex < m_vecDiffuse.size())
    {
        return m_vecDiffuse.at(subsetIndex);
    }

    return D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
}

float MeshPBRManager::GetSubsetSpecularIntensity(const DWORD subsetIndex) const
{
    if (m_param.specularIntensityOverrideEnabled)
    {
        return m_param.specularIntensity;
    }

    if (subsetIndex < m_vecSpecularIntensity.size())
    {
        return m_vecSpecularIntensity.at(subsetIndex);
    }

    return m_param.specularIntensity;
}

float MeshPBRManager::GetSubsetSpecularPower(const DWORD subsetIndex) const
{
    if (m_param.specularEdgeOverrideEnabled)
    {
        return ConvertSpecularEdgeToShaderPower(m_param.specularEdge);
    }

    if (subsetIndex < m_vecSpecularPower.size())
    {
        return m_vecSpecularPower.at(subsetIndex);
    }

    return ConvertSpecularEdgeToShaderPower(m_param.specularEdge);
}

LPDIRECT3DBASETEXTURE9 MeshPBRManager::GetSubsetTexture(const DWORD subsetIndex) const
{
    if (subsetIndex < m_vecTexture.size())
    {
        return m_vecTexture.at(subsetIndex);
    }

    return nullptr;
}

float MeshPBRManager::GetRadius() const
{
    return m_param.collisionRadius;
}

std::wstring MeshPBRManager::GetMeshName()
{
    return m_meshName;
}

void MeshPBRManager::OnDeviceLost()
{
    if (GetSharedEffect() != nullptr && !GetSharedEffectLostState())
    {
        const HRESULT hr = GetSharedEffect()->OnLostDevice();
        assert(hr == S_OK);
        GetSharedEffectLostState() = true;
    }
}

void MeshPBRManager::OnDeviceReset()
{
    if (GetSharedEffect() != nullptr && GetSharedEffectLostState())
    {
        const HRESULT hr = GetSharedEffect()->OnResetDevice();
        assert(hr == S_OK);
        GetSharedEffectLostState() = false;
    }
}
}
