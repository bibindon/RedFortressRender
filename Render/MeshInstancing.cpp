#include "MeshInstancing.h"
#include "Camera.h"
#include "Light.h"
#include "Util.h"

#include <Shlwapi.h>
#include <cwchar>
#include <fstream>
#include <sstream>

namespace NSRender
{

namespace
{
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
}

MeshInstancing::MeshInstancing()
{
}

MeshInstancing::~MeshInstancing()
{
    Finalize();
}

void MeshInstancing::Initialize(const std::wstring& filePath)
{
    HRESULT hResult = E_FAIL;
    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;

    m_filePath = ResolvePathFromExeDir(filePath);

    hResult = D3DXLoadMeshFromX(m_filePath.c_str(),
                                D3DXMESH_SYSTEMMEM,
                                Common::D3DDevice(),
                                NULL,
                                &pD3DXMtrlBuffer,
                                NULL,
                                &m_dwNumMaterials,
                                &m_pMesh);
    assert(hResult == S_OK);

    D3DXMATERIAL* d3dxMaterials = reinterpret_cast<D3DXMATERIAL*>(pD3DXMtrlBuffer->GetBufferPointer());
    m_pMaterials.resize(m_dwNumMaterials);
    m_pTextures.resize(m_dwNumMaterials);

    for (DWORD i = 0; i < m_dwNumMaterials; ++i)
    {
        m_pMaterials[i] = d3dxMaterials[i].MatD3D;
        m_pMaterials[i].Ambient = m_pMaterials[i].Diffuse;
        m_pTextures[i] = NULL;

        const std::wstring texturePath = ResolveTexturePath(m_filePath, d3dxMaterials[i].pTextureFilename);
        if (!texturePath.empty())
        {
            hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                                texturePath.c_str(),
                                                &m_pTextures[i]);
            assert(hResult == S_OK);
        }
    }

    hResult = pD3DXMtrlBuffer->Release();
    assert(hResult == S_OK);

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

    m_loadedPlacementCsv = LoadPlacementCsv();
    Common::AddDeviceLostResource(this);
}

void MeshInstancing::Finalize()
{
    Common::RemoveDeviceLostResource(this);

    SAFE_RELEASE(m_decl);
    SAFE_RELEASE(m_worldPosBuf);
    SAFE_RELEASE(m_pEffect);
    SAFE_RELEASE(m_pMesh);

    for (auto& texture : m_pTextures)
    {
        SAFE_RELEASE(texture);
    }

    m_pTextures.clear();
    m_pMaterials.clear();
    m_dwNumMaterials = 0;
    m_instances.clear();
    m_filePath.clear();
    m_loadedPlacementCsv = false;
}

void MeshInstancing::AddInstance(const D3DXVECTOR3& pos)
{
    if (m_loadedPlacementCsv && !m_instances.empty())
    {
        return;
    }

    InstanceData instance;
    instance.x = pos.x;
    instance.y = pos.y;
    instance.z = pos.z;
    instance.rotationYRadians = 0.0f;
    instance.scale = 1.0f;

    m_instances.clear();
    m_instances.push_back(instance);
    UpdateInstanceBuffer();
}

void MeshInstancing::SetDitherAlpha(const bool enabled)
{
    m_ditherAlphaEnabled = enabled;
}

void MeshInstancing::Draw()
{
    if (m_pMesh == nullptr || m_pEffect == nullptr || m_worldPosBuf == nullptr || m_instances.empty())
    {
        return;
    }

    HRESULT hResult = E_FAIL;
    const D3DXMATRIX viewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();

    hResult = m_pEffect->SetMatrix("g_matWorldViewProj", &viewProj);
    assert(hResult == S_OK);

    hResult = m_pEffect->SetBool("g_bDitherAlpha", m_ditherAlphaEnabled ? TRUE : FALSE);
    assert(hResult == S_OK);

    const D3DXVECTOR4 ambientColor = D3DXVECTOR4(Light::GetAmbientColor());
    hResult = m_pEffect->SetVector("g_ambient", &ambientColor);
    assert(hResult == S_OK);

    hResult = m_pEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());
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

    hResult = m_pEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    UINT numPass = 0;
    hResult = m_pEffect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = m_pEffect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < m_dwNumMaterials; ++i)
    {
        hResult = m_pEffect->SetTexture("texture1", m_pTextures[i]);
        assert(hResult == S_OK);

        hResult = m_pEffect->CommitChanges();
        assert(hResult == S_OK);

        hResult = Common::D3DDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
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
    if (m_pEffect != nullptr)
    {
        m_pEffect->OnLostDevice();
    }
}

void MeshInstancing::OnDeviceReset()
{
    if (m_pEffect != nullptr)
    {
        m_pEffect->OnResetDevice();
    }
}

void MeshInstancing::UpdateInstanceBuffer()
{
    SAFE_RELEASE(m_worldPosBuf);

    if (m_instances.empty())
    {
        return;
    }

    HRESULT hResult = Common::D3DDevice()->CreateVertexBuffer(sizeof(InstanceData) * static_cast<UINT>(m_instances.size()),
                                                              0,
                                                              0,
                                                              D3DPOOL_MANAGED,
                                                              &m_worldPosBuf,
                                                              0);
    assert(hResult == S_OK);

    copyBuf(sizeof(InstanceData) * static_cast<unsigned>(m_instances.size()),
            m_instances.data(),
            m_worldPosBuf);
}

bool MeshInstancing::LoadPlacementCsv()
{
    const std::wstring csvPath = BuildPlacementCsvPath(m_filePath);
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

    m_instances = loadedInstances;
    UpdateInstanceBuffer();
    return true;
}

void MeshInstancing::copyBuf(unsigned sz, void* src, IDirect3DVertexBuffer9* buf)
{
    void* p = 0;
    buf->Lock(0, 0, &p, 0);
    memcpy(p, src, sz);
    buf->Unlock();
}

}
