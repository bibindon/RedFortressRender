#include "MeshMixManager.h"

#include "Util.h"
#include "Camera.h"
#include "Light.h"

#include <Shlwapi.h>
#include <algorithm>
#include <cwctype>
#include <unordered_map>
#include <utility>
#pragma comment(lib, "Shlwapi.lib")

namespace NSRender
{
namespace
{
using TextureCache = std::unordered_map<std::wstring, LPDIRECT3DBASETEXTURE9>;

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

float ClampSpecularEdge(const float edge)
{
    return (std::max)(0.0f, (std::min)(edge, 1.0f));
}

float ConvertSpecularEdgeToShaderPower(const float edge)
{
    return 1.0f + (ClampSpecularEdge(edge) * 127.0f);
}

float ConvertXMaterialPowerToShaderPower(const float materialPower)
{
    const float clampedPower = (std::max)(0.0f, (std::min)(materialPower, 255.0f));
    return 1.0f + ((1.0f - (clampedPower / 255.0f)) * 127.0f);
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
        *texture = found->second;
        if (*texture != nullptr)
        {
            (*texture)->AddRef();
        }
        return S_OK;
    }

    LPDIRECT3DTEXTURE9 loadedTexture = nullptr;
    const HRESULT hr = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                 texturePath.c_str(),
                                                 &loadedTexture);
    if (FAILED(hr))
    {
        return hr;
    }

    textureCache.emplace(cacheKey, loadedTexture);
    loadedTexture->AddRef();
    *texture = loadedTexture;
    return S_OK;
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

MeshMixManager::MeshMixManager(const std::wstring& filename,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rotate,
                               const float scale,
                               const stMeshParam& param)
    : m_meshName(filename)
    , m_pos(pos)
    , m_rotate(rotate)
    , m_scale(scale)
    , m_param(param)
{
}

MeshMixManager::~MeshMixManager()
{
    Finalize();
}

MeshMixManager::MeshMixManager(MeshMixManager&& other) noexcept
    : SHADER_FILENAME(other.SHADER_FILENAME)
{
    *this = std::move(other);
}

MeshMixManager& MeshMixManager::operator=(MeshMixManager&& other) noexcept
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
    m_bLoaded = other.m_bLoaded;
    m_enabled = other.m_enabled;
    m_deviceResourceRegistered = other.m_deviceResourceRegistered;
    m_param = other.m_param;

    other.m_materialCount = 0;
    other.m_subsetCount = 0;
    other.m_bLoaded = false;
    other.m_enabled = false;
    other.m_deviceResourceRegistered = false;

    return *this;
}

void MeshMixManager::Initialize()
{
    HRESULT hResult = EnsureSharedEffectCreated(SHADER_FILENAME);
    assert(hResult == S_OK);

    if (m_bLoaded)
    {
        return;
    }

    AddSharedEffectRef();

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
    m_subsetCount = (subsetCount > 0) ? subsetCount : m_materialCount;

    D3DXMATERIAL* materialList = static_cast<D3DXMATERIAL*>(materialBuffer->GetBufferPointer());

    std::wstring xFileDir = tempPath;
    const std::size_t lastPos = xFileDir.find_last_of(L"\\");
    xFileDir = xFileDir.substr(0, lastPos + 1);

    std::vector<D3DXVECTOR4> diffuseList;
    std::vector<float> specularPowerList;
    std::vector<LPDIRECT3DBASETEXTURE9> diffuseTextureList;

    for (DWORD i = 0; i < m_materialCount; ++i)
    {
        D3DXVECTOR4 diffuse(1.0f, 1.0f, 1.0f, 1.0f);
        diffuse.x = materialList[i].MatD3D.Diffuse.r;
        diffuse.y = materialList[i].MatD3D.Diffuse.g;
        diffuse.z = materialList[i].MatD3D.Diffuse.b;
        diffuse.w = materialList[i].MatD3D.Diffuse.a;
        const float specularPower = ConvertXMaterialPowerToShaderPower(materialList[i].MatD3D.Power);

        LPDIRECT3DBASETEXTURE9 tempTexture = nullptr;
        std::wstring textureFileName;

        if (materialList[i].pTextureFilename != nullptr &&
            strlen(materialList[i].pTextureFilename) != 0)
        {
            textureFileName = Util::Utf8ToWstring(materialList[i].pTextureFilename);
            const std::wstring texturePath = xFileDir + textureFileName;
            hResult = LoadTextureCached(texturePath, &tempTexture);
            assert(hResult == S_OK);
        }

        const eMeshTextureRole textureRole = ClassifyTextureRole(textureFileName);
        if (textureRole == eMeshTextureRole::Normal)
        {
            if (m_texNormalMap == nullptr)
            {
                m_texNormalMap = tempTexture;
            }
        }
        else if (textureRole == eMeshTextureRole::Height)
        {
            if (m_texHeightMap == nullptr)
            {
                m_texHeightMap = tempTexture;
            }
        }
        else if (textureRole == eMeshTextureRole::Cube)
        {
            if (m_texCubeMap == nullptr)
            {
                m_texCubeMap = tempTexture;
            }
        }
        else
        {
            diffuseList.push_back(diffuse);
            specularPowerList.push_back(specularPower);
            diffuseTextureList.push_back(tempTexture);
        }
    }

    if (!diffuseList.empty())
    {
        m_vecDiffuse = diffuseList;
        m_vecSpecularPower = specularPowerList;
        m_vecTexture = diffuseTextureList;
        m_subsetCount = (std::min)(m_subsetCount, static_cast<DWORD>(m_vecDiffuse.size()));
    }
    else
    {
        m_vecDiffuse.assign(m_subsetCount, D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f));
        m_vecSpecularPower.assign(m_subsetCount, ConvertSpecularEdgeToShaderPower(m_param.specularEdge));
        m_vecTexture.assign(m_subsetCount, nullptr);
    }

    SAFE_RELEASE(materialBuffer);

    if (!m_deviceResourceRegistered)
    {
        Common::AddDeviceLostResource(this);
        m_deviceResourceRegistered = true;
    }

    m_bLoaded = true;
}

void MeshMixManager::Finalize()
{
    if (!m_bLoaded && m_D3DMesh == nullptr && m_texCubeMap == nullptr && m_texNormalMap == nullptr && m_texHeightMap == nullptr)
    {
        return;
    }

    ReleaseOwnedResources();
    ReleaseSharedEffectRef();
    m_bLoaded = false;
}

void MeshMixManager::ReleaseOwnedResources()
{
    SAFE_RELEASE(m_D3DMesh);

    for (auto& texture : m_vecTexture)
    {
        SAFE_RELEASE(texture);
    }
    m_vecTexture.clear();
    m_vecDiffuse.clear();
    m_vecSpecularPower.clear();

    SAFE_RELEASE(m_texCubeMap);
    SAFE_RELEASE(m_texNormalMap);
    SAFE_RELEASE(m_texHeightMap);

    m_materialCount = 0;
    m_subsetCount = 0;
}

void MeshMixManager::ModifyMeshForNormalMapping(LPD3DXMESH& pMesh)
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

    hr = D3DXComputeTangentFrameEx(pCloned,
                                   D3DDECLUSAGE_TEXCOORD, 0,
                                   D3DDECLUSAGE_TANGENT, 0,
                                   D3DDECLUSAGE_BINORMAL, 0,
                                   D3DDECLUSAGE_NORMAL, 0,
                                   options,
                                   adj.data(),
                                   0.01f,
                                   0.01f,
                                   0.999f,
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

void MeshMixManager::SetPos(const D3DXVECTOR3& pos)
{
    m_pos = pos;
}

void MeshMixManager::SetSaturateShadow(const bool enabled)
{
    m_param.saturateShadow = enabled;
}

void MeshMixManager::SetSaturateShadowIntensity(const float intensity)
{
    m_param.saturateShadowIntensity = intensity;
}

void MeshMixManager::SetShadowDarkness(const float darkness)
{
    m_param.shadowDarkness = darkness;
}

void MeshMixManager::SetSpecularIntensity(const float intensity)
{
    m_param.specularIntensity = intensity;
}

void MeshMixManager::SetSpecularEdge(const float edge)
{
    m_param.specularEdge = edge;
}

void MeshMixManager::SetRotY(const float rotY)
{
    m_rotate.y = rotY;
}

D3DXVECTOR3 MeshMixManager::GetRot() const
{
    return m_rotate;
}

D3DXVECTOR3 MeshMixManager::GetPos() const
{
    return m_pos;
}

float MeshMixManager::GetScale() const
{
    return m_scale;
}

DWORD MeshMixManager::GetSubsetCount() const
{
    return m_subsetCount;
}

bool MeshMixManager::IsEnabled() const
{
    return m_enabled;
}

void MeshMixManager::SetEnabled(const bool enabled)
{
    m_enabled = enabled;
}

void MeshMixManager::Render()
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
    hResult = sharedEffect->SetVector("g_lightDir", &normal);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    assert(hResult == S_OK);

    D3DXMATRIX matWorld{ };
    D3DXMatrixIdentity(&matWorld);

    D3DXMATRIX matWork;
    D3DXMatrixIdentity(&matWork);

    D3DXMatrixScaling(&matWork, m_scale, m_scale, m_scale);
    matWorld *= matWork;

    D3DXMatrixRotationYawPitchRoll(&matWork, m_rotate.y, m_rotate.x, m_rotate.z);
    matWorld *= matWork;

    D3DXMatrixTranslation(&matWork, m_pos.x, m_pos.y, m_pos.z);
    matWorld *= matWork;

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

    const float screenSize[2] =
    {
        static_cast<float>(Common::ScreenW()),
        static_cast<float>(Common::ScreenH())
    };
    hResult = sharedEffect->SetFloatArray("g_screenSize", screenSize, 2);
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

    hResult = sharedEffect->SetBool("g_bPOM", m_param.parallaxOcclusionMapping ? TRUE : FALSE);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetBool("g_bNormalMapping", m_param.normalMapping ? TRUE : FALSE);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetBool("g_bSaturateShadow", m_param.saturateShadow ? TRUE : FALSE);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_fSaturateShadowIntensity", m_param.saturateShadowIntensity);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_fShadowDarkness", m_param.shadowDarkness);
    assert(hResult == S_OK);

    hResult = sharedEffect->SetFloat("g_specularIntensity", m_param.specularIntensity);
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

    if (m_param.pointLight)
    {
        auto pointLightList = Light::GetPointLightList();

        D3DXVECTOR4 pos[16];
        float brightness[16] { };
        D3DXVECTOR4 color[16];

        ZeroMemory(pos, sizeof(pos));
        ZeroMemory(color, sizeof(color));

        for (int i = 0; i < 16; ++i)
        {
            if (i < pointLightList.size())
            {
                pos[i].x = pointLightList.at(i).m_pos.x;
                pos[i].y = pointLightList.at(i).m_pos.y;
                pos[i].z = pointLightList.at(i).m_pos.z;
                brightness[i] = pointLightList.at(i).m_brightness;
                color[i].x = pointLightList.at(i).m_color.r;
                color[i].y = pointLightList.at(i).m_color.g;
                color[i].z = pointLightList.at(i).m_color.b;
            }
        }

        hResult = sharedEffect->SetVectorArray("g_pointLightPos", pos, 16);
        assert(hResult == S_OK);

        hResult = sharedEffect->SetFloatArray("g_pointLightBrightness", brightness, 16);
        assert(hResult == S_OK);

        hResult = sharedEffect->SetVectorArray("g_pointLightColor", color, 16);
        assert(hResult == S_OK);
    }

    hResult = sharedEffect->CommitChanges();
    assert(hResult == S_OK);

    auto drawAllSubsets = [this, sharedEffect, &hResult](const UINT passIndex)
    {
        hResult = sharedEffect->BeginPass(passIndex);
        assert(hResult == S_OK);

        const DWORD subsetCount = (m_subsetCount > 0) ? m_subsetCount : 1;
        for (DWORD subsetIndex = 0; subsetIndex < subsetCount; ++subsetIndex)
        {
            const D3DXVECTOR4 diffuse = GetSubsetDiffuse(subsetIndex);
            hResult = sharedEffect->SetVector("g_diffuse", &diffuse);
            assert(hResult == S_OK);

            hResult = sharedEffect->SetFloat("g_specularPower", GetSubsetSpecularPower(subsetIndex));
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
    };

    drawAllSubsets(0);
    drawAllSubsets(1);

    if (m_param.glass)
    {
        drawAllSubsets(2);
    }

    drawAllSubsets(3);

    hResult = sharedEffect->End();
    assert(hResult == S_OK);
}

LPD3DXMESH MeshMixManager::GetD3DMesh() const
{
    return m_D3DMesh;
}

D3DXVECTOR4 MeshMixManager::GetSubsetDiffuse(const DWORD subsetIndex) const
{
    if (subsetIndex < m_vecDiffuse.size())
    {
        return m_vecDiffuse.at(subsetIndex);
    }

    return D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
}

float MeshMixManager::GetSubsetSpecularPower(const DWORD subsetIndex) const
{
    if (subsetIndex < m_vecSpecularPower.size())
    {
        return m_vecSpecularPower.at(subsetIndex);
    }

    return ConvertSpecularEdgeToShaderPower(m_param.specularEdge);
}

LPDIRECT3DBASETEXTURE9 MeshMixManager::GetSubsetTexture(const DWORD subsetIndex) const
{
    if (subsetIndex < m_vecTexture.size())
    {
        return m_vecTexture.at(subsetIndex);
    }

    return nullptr;
}

float MeshMixManager::GetRadius() const
{
    return m_param.collisionRadius;
}

std::wstring MeshMixManager::GetMeshName()
{
    return m_meshName;
}

void MeshMixManager::OnDeviceLost()
{
    if (GetSharedEffect() != nullptr && !GetSharedEffectLostState())
    {
        const HRESULT hr = GetSharedEffect()->OnLostDevice();
        assert(hr == S_OK);
        GetSharedEffectLostState() = true;
    }
}

void MeshMixManager::OnDeviceReset()
{
    if (GetSharedEffect() != nullptr && GetSharedEffectLostState())
    {
        const HRESULT hr = GetSharedEffect()->OnResetDevice();
        assert(hr == S_OK);
        GetSharedEffectLostState() = false;
    }
}
}
