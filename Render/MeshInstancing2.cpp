#include "MeshInstancing2.h"
#include "Camera.h"
#include "GBuffer.h"
#include "CustomXLoader.h"
#include "Light.h"
#include "SkinAnimMeshAlloc.h"
#include "Util.h"

#include <Shlwapi.h>
#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace NSRender
{

namespace
{
bool TextureFormatHasAlpha(const D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
    case D3DFMT_A8:
    case D3DFMT_A8B8G8R8:
    case D3DFMT_A2R10G10B10:
    case D3DFMT_A16B16G16R16:
    case D3DFMT_A8P8:
    case D3DFMT_A8L8:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
    case D3DFMT_A16B16G16R16F:
    case D3DFMT_A32B32G32R32F:
        return true;
    default:
        return false;
}
}

SkinAnimMeshContainer* FindFirstMeshContainer(LPD3DXFRAME frame)
{
    if (frame == nullptr)
    {
        return nullptr;
    }

    if (frame->pMeshContainer != nullptr)
    {
        return reinterpret_cast<SkinAnimMeshContainer*>(frame->pMeshContainer);
    }

    SkinAnimMeshContainer* container = FindFirstMeshContainer(frame->pFrameFirstChild);
    if (container != nullptr)
    {
        return container;
    }

    return FindFirstMeshContainer(frame->pFrameSibling);
}

std::wstring ResolvePathFromExeDir(const std::wstring& path)
{
    if (path.empty())
    {
        return std::wstring();
}

    if (PathIsRelative(path.c_str()))
    {
        return Util::GetExeDir() + path;
    }

    return path;
}

std::wstring GetDirectoryPath(const std::wstring& path)
{
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
    {
        return std::wstring();
    }

    return path.substr(0, pos);
}

std::wstring ResolveTexturePath(const std::wstring& modelPath, const char* texturePath)
{
    if (texturePath == nullptr || texturePath[0] == '\0')
    {
        return std::wstring();
    }

    const std::wstring wideTexturePath = Util::Utf8ToWstring(texturePath);
    if (!PathIsRelative(wideTexturePath.c_str()))
    {
        return wideTexturePath;
    }

    const std::wstring modelDirectory = GetDirectoryPath(modelPath);
    if (modelDirectory.empty())
    {
        return ResolvePathFromExeDir(wideTexturePath);
    }

    return modelDirectory + L'\\' + wideTexturePath;
}

std::wstring BuildPlacementCsvPath(const std::wstring& modelPath)
{
    const std::wstring::size_type dotPos = modelPath.find_last_of(L'.');
    if (dotPos == std::wstring::npos)
    {
        return modelPath + L".csv";
    }

    return modelPath.substr(0, dotPos) + L".csv";
}

std::wstring TrimWhitespace(const std::wstring& value)
{
    const std::wstring whitespace = L" \t\r\n";
    const size_t start = value.find_first_not_of(whitespace);
    if (start == std::wstring::npos)
    {
        return std::wstring();
    }

    const size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::vector<std::wstring> SplitCsvFields(const std::wstring& line)
{
    std::vector<std::wstring> fields;
    std::wstring field;
    std::wstringstream stream(line);

    while (std::getline(stream, field, L','))
    {
        fields.push_back(TrimWhitespace(field));
    }

    if (!line.empty() && line.back() == L',')
    {
        fields.push_back(L"");
    }

    return fields;
}

bool TryParseFloat(const std::wstring& text, float& value)
{
    if (text.empty())
    {
        return false;
    }

    wchar_t* endPtr = nullptr;
    const double parsed = std::wcstod(text.c_str(), &endPtr);
    if (endPtr == text.c_str() || *endPtr != L'\0')
    {
        return false;
    }

    value = static_cast<float>(parsed);
    return true;
}

std::wstring ToLower(const std::wstring& value)
{
    std::wstring lower = value;
    for (wchar_t& ch : lower)
    {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
    return lower;
}

std::wstring NormalizeCsvKey(const std::wstring& value)
{
    std::wstring key = TrimWhitespace(value);
    if (!key.empty() && key[0] == 0xFEFF)
    {
        key.erase(key.begin());
    }
    return ToLower(key);
}

bool TryParseSwayMode(const std::wstring& text, MeshInstancing2::SwayMode& value)
{
    const std::wstring lower = ToLower(TrimWhitespace(text));
    if (lower == L"normal" || lower == L"on" || lower == L"true" || lower == L"1" ||
        lower == L"yes" || lower == L"y")
    {
        value = MeshInstancing2::SwayMode::Normal;
        return true;
    }
    if (lower == L"wave")
    {
        value = MeshInstancing2::SwayMode::Wave;
        return true;
    }
    if (lower == L"off" || lower == L"false" || lower == L"0" || lower == L"no" || lower == L"n")
    {
        value = MeshInstancing2::SwayMode::Off;
        return true;
    }
    return false;
}

bool IsCsvTrueValue(const std::wstring& text)
{
    const std::wstring lower = ToLower(TrimWhitespace(text));
    return lower == L"on" || lower == L"true" || lower == L"1" ||
           lower == L"yes" || lower == L"y";
}
}

MeshInstancing2::MeshInstancing2()
{
}

MeshInstancing2::~MeshInstancing2()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }
    Finalize();
}

void MeshInstancing2::Initialize(const std::wstring& filePath, bool async)
{
    m_filePath = ResolvePathFromExeDir(filePath);
    m_customPlacementCsvPath.clear();

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

void MeshInstancing2::Initialize(const std::wstring& filePath, const std::wstring& csvPath, bool async)
{
    m_filePath = ResolvePathFromExeDir(filePath);
    m_customPlacementCsvPath = ResolvePathFromExeDir(csvPath);

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

void MeshInstancing2::WaitForLoad()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }
}

bool MeshInstancing2::IsLoaded() const
{
    return m_bLoaded.load();
}

void MeshInstancing2::InitializeInternal()
{
    std::ifstream file(m_filePath, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("MeshInstancing2 failed to open the X file.");
    }

    const std::string fileText((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    if (fileText.empty())
    {
        throw std::runtime_error("MeshInstancing2 received an empty X file.");
    }

    SkinAnimMeshAlloc allocator(m_filePath);
    LPD3DXFRAME frameRoot = nullptr;
    HRESULT hResult = LoadCustomXFrameHierarchyFromText(fileText,
                                                         &allocator,
                                                         &frameRoot,
                                                         nullptr,
                                                         CustomXLoadPurpose::MeshAndAnimation);
    if (FAILED(hResult) || frameRoot == nullptr)
    {
        throw std::runtime_error("MeshInstancing2 failed to parse the Blender 5.1.2 X file.");
    }

    SkinAnimMeshContainer* meshContainer = FindFirstMeshContainer(frameRoot);
    if (meshContainer == nullptr || meshContainer->MeshData.pMesh == nullptr)
    {
        DestroyCustomXFrameHierarchyWithAllocator(frameRoot, allocator);
        throw std::runtime_error("MeshInstancing2 could not find mesh data in the X file.");
    }

    m_pMesh = meshContainer->MeshData.pMesh;
    m_pMesh->AddRef();
    m_dwNumMaterials = meshContainer->NumMaterials;
    m_pMaterials.resize(m_dwNumMaterials);
    m_pTextures.resize(m_dwNumMaterials);
    m_materialUsesAlpha.assign(m_dwNumMaterials, false);

    for (DWORD i = 0; i < m_dwNumMaterials; ++i)
    {
        m_pMaterials[i] = meshContainer->pMaterials[i].MatD3D;
        m_pMaterials[i].Ambient = m_pMaterials[i].Diffuse;
        m_pTextures[i] = nullptr;
        if (i < meshContainer->m_textureList.size() && meshContainer->m_textureList[i] != nullptr)
        {
            m_pTextures[i] = meshContainer->m_textureList[i];
            m_pTextures[i]->AddRef();
        }
    }

    DestroyCustomXFrameHierarchyWithAllocator(frameRoot, allocator);

    std::vector<DWORD> adjacencyList(m_pMesh->GetNumFaces() * 3, 0xffffffff);
    hResult = m_pMesh->GenerateAdjacency(0.0001f, adjacencyList.data());
    if (FAILED(hResult))
    {
        throw std::runtime_error("MeshInstancing2 failed to generate mesh adjacency.");
    }

    hResult = m_pMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE,
                                       adjacencyList.data(),
                                       nullptr,
                                       nullptr,
                                       nullptr);
    if (FAILED(hResult))
    {
        throw std::runtime_error("MeshInstancing2 failed to optimize the mesh.");
    }

    DWORD subsetCount = 0;
    hResult = m_pMesh->GetAttributeTable(nullptr, &subsetCount);
    if (FAILED(hResult))
    {
        throw std::runtime_error("MeshInstancing2 failed to read the mesh attribute table.");
    }
    if (subsetCount > 0)
    {
        m_attributeTable.resize(subsetCount);
        hResult = m_pMesh->GetAttributeTable(m_attributeTable.data(), &subsetCount);
        if (FAILED(hResult))
        {
            throw std::runtime_error("MeshInstancing2 failed to copy the mesh attribute table.");
        }
    }
    else
    {
        D3DXATTRIBUTERANGE range { };
        range.AttribId = 0;
        range.FaceStart = 0;
        range.FaceCount = m_pMesh->GetNumFaces();
        range.VertexStart = 0;
        range.VertexCount = m_pMesh->GetNumVertices();
        m_attributeTable.push_back(range);
    }

    for (DWORD i = 0; i < m_dwNumMaterials; ++i)
    {
        if (m_pTextures[i] != nullptr)
        {
            D3DSURFACE_DESC surfaceDesc { };
            hResult = m_pTextures[i]->GetLevelDesc(0, &surfaceDesc);
            if (FAILED(hResult))
            {
                throw std::runtime_error("MeshInstancing2 failed to inspect a material texture.");
            }
            m_materialUsesAlpha[i] = TextureFormatHasAlpha(surfaceDesc.Format);
        }
    }

    const std::wstring effectPath = Util::GetExeDir() + L"MeshInstancing.cso";
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       effectPath.c_str(),
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_pEffect,
                                       NULL);
    assert(hResult == S_OK);

    D3DVERTEXELEMENT9 declElems[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 1, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
        { 1, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
        D3DDECL_END()
    };

    hResult = Common::D3DDevice()->CreateVertexDeclaration(declElems, &m_decl);
    assert(hResult == S_OK);

    if (!m_customPlacementCsvPath.empty())
    {
        m_loadedPlacementCsv = LoadPlacementCsv(m_customPlacementCsvPath);
    }
    else
    {
        m_loadedPlacementCsv = LoadPlacementCsv();
    }
    m_bLoaded = true;
}

void MeshInstancing2::Finalize()
{
    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }

    m_bLoaded = false;

    SAFE_RELEASE(m_decl);
    SAFE_RELEASE(m_worldPosBuf);
    SAFE_RELEASE(m_pEffect);
    SAFE_RELEASE(m_pMesh);

    for (auto& texture : m_pTextures)
    {
        SAFE_RELEASE(texture);
    }

    m_pTextures.clear();
    m_materialUsesAlpha.clear();
    m_pMaterials.clear();
    m_attributeTable.clear();
    m_dwNumMaterials = 0;
    m_allInstances.clear();
    m_instances.clear();
    m_filePath.clear();
    m_loadedPlacementCsv = false;
    m_autoHide = false;
    m_swayMode = SwayMode::Off;
}

void MeshInstancing2::AddInstance(const D3DXVECTOR3& pos, float rotationY)
{
    if (m_loadedPlacementCsv && !m_allInstances.empty())
    {
        SetInstanceOffset(pos);
        return;
    }

    InstanceData instance;
    instance.x = pos.x;
    instance.y = pos.y;
    instance.z = pos.z;
    instance.rotationYRadians = rotationY;
    instance.scale = 1.0f;

    m_allInstances.clear();
    m_allInstances.push_back(instance);
    UpdateVisibleInstances();
}

void MeshInstancing2::SetInstanceOffset(const D3DXVECTOR3& offset)
{
    m_instanceOffset = offset;
    UpdateVisibleInstances();
}

void MeshInstancing2::SetHighQuality(const bool enabled)
{
    m_highQualityEnabled = enabled;
}

void MeshInstancing2::Draw()
{
    if (!m_bLoaded)
    {
        return;
    }

    UpdateVisibleInstances();

    if (m_pMesh == nullptr || m_pEffect == nullptr || m_worldPosBuf == nullptr || m_instances.empty())
    {
        return;
    }

    const bool hasAlphaMaterial =
        (std::find(m_materialUsesAlpha.begin(), m_materialUsesAlpha.end(), true) != m_materialUsesAlpha.end());
    if (m_highQualityEnabled && hasAlphaMaterial)
    {
        SortInstancesBackToFront();
        UpdateInstanceBuffer();
    }

    HRESULT hResult = E_FAIL;
    const D3DXMATRIX viewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();

    hResult = m_pEffect->SetMatrix("g_matWorldViewProj", &viewProj);
    assert(hResult == S_OK);

    const D3DXVECTOR4 lightDir = Light::GetLightDir();
    hResult = m_pEffect->SetVector("g_lightDir", &lightDir);
    assert(hResult == S_OK);

    const D3DXVECTOR4 lightColor = D3DXVECTOR4(Light::GetLightColor());
    hResult = m_pEffect->SetVector("g_lightColor", &lightColor);
    assert(hResult == S_OK);

    const D3DXVECTOR4 ambientColor = D3DXVECTOR4(Light::GetAmbientColor());
    hResult = m_pEffect->SetVector("g_ambient", &ambientColor);
    assert(hResult == S_OK);

    hResult = m_pEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());
    assert(hResult == S_OK);

    hResult = m_pEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    assert(hResult == S_OK);

    hResult = m_pEffect->SetInt("g_swayMode", static_cast<int>(m_swayMode));
    assert(hResult == S_OK);

    hResult = m_pEffect->SetFloat("g_time", static_cast<float>(GetTickCount64()) * 0.001f);
    assert(hResult == S_OK);
    GBuffer::ApplyIntegratedEffectParameters(m_pEffect, false);

    LPDIRECT3DVERTEXBUFFER9 pVB = nullptr;
    m_pMesh->GetVertexBuffer(&pVB);
    Common::D3DDevice()->SetStreamSource(0, pVB, 0, m_pMesh->GetNumBytesPerVertex());
    pVB->Release();

    Common::D3DDevice()->SetStreamSource(1, m_worldPosBuf, 0, sizeof(InstanceData));
    Common::D3DDevice()->SetVertexDeclaration(m_decl);
    Common::D3DDevice()->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | static_cast<UINT>(m_instances.size()));
    Common::D3DDevice()->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1);

    LPDIRECT3DINDEXBUFFER9 pIB = nullptr;
    m_pMesh->GetIndexBuffer(&pIB);
    Common::D3DDevice()->SetIndices(pIB);
    pIB->Release();

    const char* techniqueName = "TechniqueNormalIntegrated";
    if (m_highQualityEnabled)
    {
        techniqueName = "TechniqueHighQualityIntegrated";
    }
    hResult = m_pEffect->SetTechnique(techniqueName);
    assert(hResult == S_OK);

    UINT numPass = 0;
    hResult = m_pEffect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = m_pEffect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < m_attributeTable.size(); ++i)
    {
        const DWORD materialIndex = GetSubsetMaterialIndex(i);
        LPDIRECT3DTEXTURE9 texture = nullptr;
        if (materialIndex < m_pTextures.size())
        {
            texture = m_pTextures[materialIndex];
        }
        hResult = m_pEffect->SetTexture("texture1", texture);
        assert(hResult == S_OK);

        DWORD oldZWriteEnable = TRUE;
        DWORD oldDepthColorWrite = 15;
        DWORD oldPositionColorWrite = 15;
        DWORD oldNormalColorWrite = 15;
        if (m_highQualityEnabled && IsSubsetAlphaMaterial(i))
        {
            Common::D3DDevice()->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
            Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            Common::D3DDevice()->GetRenderState(D3DRS_COLORWRITEENABLE1, &oldDepthColorWrite);
            Common::D3DDevice()->GetRenderState(D3DRS_COLORWRITEENABLE2, &oldPositionColorWrite);
            Common::D3DDevice()->GetRenderState(D3DRS_COLORWRITEENABLE3, &oldNormalColorWrite);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE2, 0);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE3, 0);
        }

        hResult = m_pEffect->CommitChanges();
        assert(hResult == S_OK);
        if (m_highQualityEnabled && IsSubsetAlphaMaterial(i))
        {
            Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE1, 0);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE2, 0);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE3, 0);
        }

        hResult = DrawInstancedSubset(i);
        assert(hResult == S_OK);

        if (m_highQualityEnabled && IsSubsetAlphaMaterial(i))
        {
            Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE1, oldDepthColorWrite);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE2, oldPositionColorWrite);
            Common::D3DDevice()->SetRenderState(D3DRS_COLORWRITEENABLE3, oldNormalColorWrite);
        }
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

void MeshInstancing2::RenderToGBufferEffect(LPD3DXEFFECT effect, const char* techniqueName)
{
    if (!m_bLoaded)
    {
        return;
    }

    UpdateVisibleInstances();

    if (m_pMesh == nullptr || effect == nullptr || m_worldPosBuf == nullptr || m_instances.empty())
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    hResult = effect->SetInt("g_swayMode", static_cast<int>(m_swayMode));
    assert(hResult == S_OK);

    hResult = effect->SetFloat("g_time", static_cast<float>(GetTickCount64()) * 0.001f);
    assert(hResult == S_OK);

    LPDIRECT3DVERTEXBUFFER9 pVB = nullptr;
    m_pMesh->GetVertexBuffer(&pVB);
    Common::D3DDevice()->SetStreamSource(0, pVB, 0, m_pMesh->GetNumBytesPerVertex());
    pVB->Release();

    Common::D3DDevice()->SetStreamSource(1, m_worldPosBuf, 0, sizeof(InstanceData));
    Common::D3DDevice()->SetVertexDeclaration(m_decl);
    Common::D3DDevice()->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | static_cast<UINT>(m_instances.size()));
    Common::D3DDevice()->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1);

    LPDIRECT3DINDEXBUFFER9 pIB = nullptr;
    m_pMesh->GetIndexBuffer(&pIB);
    Common::D3DDevice()->SetIndices(pIB);
    pIB->Release();

    hResult = effect->SetTechnique(techniqueName);
    assert(hResult == S_OK);

    UINT numPass = 0;
    hResult = effect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = effect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < m_attributeTable.size(); ++i)
    {
        const DWORD materialIndex = GetSubsetMaterialIndex(i);
        LPDIRECT3DTEXTURE9 texture = nullptr;
        if (materialIndex < m_pTextures.size())
        {
            texture = m_pTextures[materialIndex];
        }
        hResult = effect->SetTexture("g_texInstancingAlpha", texture);
        assert(hResult == S_OK);

        hResult = effect->CommitChanges();
        assert(hResult == S_OK);

        hResult = DrawInstancedSubset(i);
        assert(hResult == S_OK);
    }

    hResult = effect->EndPass();
    assert(hResult == S_OK);

    hResult = effect->End();
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetStreamSourceFreq(0, 1);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetStreamSourceFreq(1, 1);
    assert(hResult == S_OK);
}

void MeshInstancing2::RenderToShadowOccluderEffect(LPD3DXEFFECT effect,
                                                  const char* techniqueName,
                                                  const float alphaClipThreshold)
{
    if (!m_bLoaded)
    {
        return;
    }

    UpdateVisibleInstances();

    if (m_pMesh == nullptr || effect == nullptr || m_worldPosBuf == nullptr || m_instances.empty())
    {
        return;
    }

    HRESULT hResult = E_FAIL;
    const D3DXMATRIX viewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();

    hResult = effect->SetMatrix("g_matWorldViewProj", &viewProj);
    assert(hResult == S_OK);

    hResult = effect->SetInt("g_swayMode", static_cast<int>(m_swayMode));
    assert(hResult == S_OK);

    hResult = effect->SetFloat("g_time", static_cast<float>(GetTickCount64()) * 0.001f);
    assert(hResult == S_OK);

    hResult = effect->SetFloat("g_instancingAlphaClipThreshold", alphaClipThreshold);
    assert(hResult == S_OK);

    LPDIRECT3DVERTEXBUFFER9 pVB = nullptr;
    m_pMesh->GetVertexBuffer(&pVB);
    Common::D3DDevice()->SetStreamSource(0, pVB, 0, m_pMesh->GetNumBytesPerVertex());
    pVB->Release();

    Common::D3DDevice()->SetStreamSource(1, m_worldPosBuf, 0, sizeof(InstanceData));
    Common::D3DDevice()->SetVertexDeclaration(m_decl);
    Common::D3DDevice()->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | static_cast<UINT>(m_instances.size()));
    Common::D3DDevice()->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1);

    LPDIRECT3DINDEXBUFFER9 pIB = nullptr;
    m_pMesh->GetIndexBuffer(&pIB);
    Common::D3DDevice()->SetIndices(pIB);
    pIB->Release();

    hResult = effect->SetTechnique(techniqueName);
    assert(hResult == S_OK);

    UINT numPass = 0;
    hResult = effect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = effect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < m_attributeTable.size(); ++i)
    {
        const DWORD materialIndex = GetSubsetMaterialIndex(i);
        LPDIRECT3DTEXTURE9 texture = nullptr;
        if (materialIndex < m_pTextures.size())
        {
            texture = m_pTextures[materialIndex];
        }
        hResult = effect->SetTexture("g_texInstancingAlpha", texture);
        assert(hResult == S_OK);

        hResult = effect->CommitChanges();
        assert(hResult == S_OK);

        hResult = DrawInstancedSubset(i);
        assert(hResult == S_OK);
    }

    hResult = effect->EndPass();
    assert(hResult == S_OK);

    hResult = effect->End();
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetStreamSourceFreq(0, 1);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetStreamSourceFreq(1, 1);
    assert(hResult == S_OK);
}

void MeshInstancing2::OnDeviceLost()
{
    if (m_pEffect != nullptr)
    {
        m_pEffect->OnLostDevice();
    }
}

void MeshInstancing2::OnDeviceReset()
{
    if (m_pEffect != nullptr)
    {
        m_pEffect->OnResetDevice();
    }
}

void MeshInstancing2::UpdateVisibleInstances()
{
    if (!m_autoHide)
    {
        if (m_instances.size() == m_allInstances.size() &&
            std::equal(m_instances.begin(), m_instances.end(), m_allInstances.begin(),
                       [](const InstanceData& lhs, const InstanceData& rhs)
                       {
                           return lhs.x == rhs.x &&
                                  lhs.y == rhs.y &&
                                  lhs.z == rhs.z &&
                                  lhs.rotationYRadians == rhs.rotationYRadians &&
                                  lhs.scale == rhs.scale;
                       }))
        {
            return;
        }

        m_instances = m_allInstances;
        UpdateInstanceBuffer();
        return;
    }

    const D3DXVECTOR3 eyePos = Camera::GetEyePos();
    const float hideDistanceSq = 30.0f * 30.0f;

    std::vector<InstanceData> visibleInstances;
    visibleInstances.reserve(m_allInstances.size());
    for (const InstanceData& instance : m_allInstances)
    {
        const float dx = instance.x - eyePos.x;
        const float dy = instance.y - eyePos.y;
        const float dz = instance.z - eyePos.z;
        const float distanceSq = dx * dx + dy * dy + dz * dz;
        if (distanceSq <= hideDistanceSq)
        {
            visibleInstances.push_back(instance);
        }
    }

    m_instances.swap(visibleInstances);
    UpdateInstanceBuffer();
}

void MeshInstancing2::SortInstancesBackToFront()
{
    const D3DXVECTOR3 eyePos = Camera::GetEyePos();
    std::stable_sort(m_instances.begin(),
                     m_instances.end(),
                     [&eyePos](const InstanceData& lhs, const InstanceData& rhs)
                     {
                         const float lhsDx = lhs.x - eyePos.x;
                         const float lhsDy = lhs.y - eyePos.y;
                         const float lhsDz = lhs.z - eyePos.z;
                         const float rhsDx = rhs.x - eyePos.x;
                         const float rhsDy = rhs.y - eyePos.y;
                         const float rhsDz = rhs.z - eyePos.z;
                         const float lhsDistanceSq = lhsDx * lhsDx + lhsDy * lhsDy + lhsDz * lhsDz;
                         const float rhsDistanceSq = rhsDx * rhsDx + rhsDy * rhsDy + rhsDz * rhsDz;
                         return lhsDistanceSq > rhsDistanceSq;
                     });
}

void MeshInstancing2::UpdateInstanceBuffer()
{
    SAFE_RELEASE(m_worldPosBuf);

    if (m_instances.empty())
    {
        return;
    }

    std::vector<InstanceData> offsetInstances;
    offsetInstances.reserve(m_instances.size());
    for (const InstanceData& instance : m_instances)
    {
        InstanceData offsetInstance = instance;
        offsetInstance.x += m_instanceOffset.x;
        offsetInstance.y += m_instanceOffset.y;
        offsetInstance.z += m_instanceOffset.z;
        offsetInstances.push_back(offsetInstance);
    }

    HRESULT hResult = Common::D3DDevice()->CreateVertexBuffer(sizeof(InstanceData) * static_cast<UINT>(offsetInstances.size()),
                                                              0,
                                                              0,
                                                              D3DPOOL_MANAGED,
                                                              &m_worldPosBuf,
                                                              0);
    assert(hResult == S_OK);

    copyBuf(sizeof(InstanceData) * static_cast<unsigned>(offsetInstances.size()),
            offsetInstances.data(),
            m_worldPosBuf);
}

bool MeshInstancing2::LoadPlacementCsv()
{
    return LoadPlacementCsv(BuildPlacementCsvPath(m_filePath));
}

bool MeshInstancing2::LoadPlacementCsv(const std::wstring& csvPath)
{
    std::wifstream file(csvPath);
    if (!file.is_open())
    {
        return false;
    }

    std::vector<InstanceData> loadedInstances;
    std::wstring line;

    while (std::getline(file, line))
    {
        const std::wstring trimmedLine = TrimWhitespace(line);
        if (trimmedLine.empty())
        {
            continue;
        }

        const std::vector<std::wstring> fields = SplitCsvFields(trimmedLine);
        if (fields.size() >= 2 && NormalizeCsvKey(fields[0]) == L"sway")
        {
            SwayMode swayMode = SwayMode::Off;
            if (TryParseSwayMode(fields[1], swayMode))
            {
                m_swayMode = swayMode;
            }
            continue;
        }
        if (fields.size() >= 2 && NormalizeCsvKey(fields[0]) == L"autohide")
        {
            m_autoHide = IsCsvTrueValue(fields[1]);
            continue;
        }

        if (fields.size() < 3)
        {
            continue;
        }

        InstanceData instance;
        if (!TryParseFloat(fields[0], instance.x) ||
            !TryParseFloat(fields[1], instance.y) ||
            !TryParseFloat(fields[2], instance.z))
        {
            continue;
        }

        float rotationYDegrees = 0.0f;
        if (fields.size() >= 4 && !fields[3].empty())
        {
            if (!TryParseFloat(fields[3], rotationYDegrees))
            {
                continue;
            }
        }
        instance.rotationYRadians = D3DXToRadian(rotationYDegrees);

        instance.scale = 1.0f;
        if (fields.size() >= 5 && !fields[4].empty())
        {
            if (!TryParseFloat(fields[4], instance.scale))
            {
                continue;
            }
        }

        loadedInstances.push_back(instance);
    }

    if (loadedInstances.empty())
    {
        return false;
    }

    m_allInstances = loadedInstances;
    UpdateVisibleInstances();
    return true;
}

void MeshInstancing2::copyBuf(unsigned sz, void* src, IDirect3DVertexBuffer9* buf)
{
    void* p = 0;
    buf->Lock(0, 0, &p, 0);
    memcpy(p, src, sz);
    buf->Unlock();
}

DWORD MeshInstancing2::GetSubsetMaterialIndex(const DWORD subsetIndex) const
{
    if (subsetIndex < m_attributeTable.size())
    {
        return m_attributeTable[subsetIndex].AttribId;
    }

    return subsetIndex;
}

bool MeshInstancing2::IsSubsetAlphaMaterial(const DWORD subsetIndex) const
{
    const DWORD materialIndex = GetSubsetMaterialIndex(subsetIndex);
    return materialIndex < m_materialUsesAlpha.size() && m_materialUsesAlpha[materialIndex];
}

HRESULT MeshInstancing2::DrawInstancedSubset(const DWORD subsetIndex) const
{
    if (subsetIndex >= m_attributeTable.size())
    {
        return E_INVALIDARG;
    }

    const D3DXATTRIBUTERANGE& range = m_attributeTable[subsetIndex];
    return Common::D3DDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                                     0,
                                                     range.VertexStart,
                                                     range.VertexCount,
                                                     range.FaceStart * 3,
                                                     range.FaceCount);
}

}
