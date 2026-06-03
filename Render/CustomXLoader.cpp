#include "CustomXLoader.h"
#include "Common.h"
#include "Util.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

namespace NSRender
{

void WriteMeshMixSkinAnimLoadLog(const std::wstring& /*message*/)
{
}

std::wstring FormatHRESULT(const HRESULT hr)
{
    wchar_t buffer[32] { };
    std::swprintf(buffer, _countof(buffer), L"0x%08X", static_cast<unsigned int>(hr));
    return buffer;
}

std::wstring AnsiTextToWideText(const std::string& text)
{
    if (text.empty())
    {
        return L"";
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0)
    {
        std::wstring fallback;
        fallback.reserve(text.size());
        for (char ch : text)
        {
            fallback.push_back(static_cast<unsigned char>(ch));
        }
        return fallback;
    }

    std::wstring result(required, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &result[0], required);
    return result;
}

namespace
{

class XTextTokenizer
{
public:
    explicit XTextTokenizer(const std::string& text)
        : m_text(text)
    {
    }

    bool ReadToken(std::string& token)
    {
        token.clear();
        SkipWhitespace();
        if (m_pos >= m_text.size())
        {
            return false;
        }

        const char ch = m_text[m_pos];
        if (ch == '{' || ch == '}')
        {
            token.push_back(ch);
            ++m_pos;
            return true;
        }

        if (ch == ',' || ch == ';')
        {
            token.push_back(ch);
            ++m_pos;
            return true;
        }

        if (ch == '"')
        {
            return ReadQuotedToken(token);
        }

        const std::size_t begin = m_pos;
        while (m_pos < m_text.size())
        {
            const char current = m_text[m_pos];
            if (current == '{' ||
                current == '}' ||
                current == ',' ||
                current == ';' ||
                current == '"' ||
                IsWhitespace(current))
            {
                break;
            }

            ++m_pos;
        }

        token.assign(m_text.begin() + begin, m_text.begin() + m_pos);
        return !token.empty();
    }

    bool SkipSeparatorsAndReadFloat(float& value)
    {
        while (m_pos < m_text.size())
        {
            SkipWhitespace();
            if (m_pos >= m_text.size())
            {
                return false;
            }

            const char ch = m_text[m_pos];
            if (ch == ',' || ch == ';')
            {
                ++m_pos;
                continue;
            }

            char* endPtr = nullptr;
            value = std::strtof(m_text.c_str() + m_pos, &endPtr);
            if (endPtr == m_text.c_str() + m_pos)
            {
                return false;
            }

            m_pos = static_cast<std::size_t>(endPtr - m_text.c_str());
            return true;
        }

        return false;
    }

    bool SkipSeparatorsAndReadUInt(DWORD& value)
    {
        while (m_pos < m_text.size())
        {
            SkipWhitespace();
            if (m_pos >= m_text.size())
            {
                return false;
            }

            const char ch = m_text[m_pos];
            if (ch == ',' || ch == ';')
            {
                ++m_pos;
                continue;
            }

            char* endPtr = nullptr;
            const unsigned long parsed = std::strtoul(m_text.c_str() + m_pos, &endPtr, 10);
            if (endPtr == m_text.c_str() + m_pos)
            {
                return false;
            }

            m_pos = static_cast<std::size_t>(endPtr - m_text.c_str());
            value = static_cast<DWORD>(parsed);
            return true;
        }

        return false;
    }

private:
    static bool IsWhitespace(const char ch)
    {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    }

    void SkipWhitespace()
    {
        bool skipped = true;
        while (skipped)
        {
            skipped = false;
            while (m_pos < m_text.size() && IsWhitespace(m_text[m_pos]))
            {
                ++m_pos;
                skipped = true;
            }

            if (SkipComment())
            {
                skipped = true;
            }
        }
    }

    bool SkipComment()
    {
        if (m_pos + 1 >= m_text.size() || m_text[m_pos] != '/')
        {
            return false;
        }

        if (m_text[m_pos + 1] == '/')
        {
            m_pos += 2;
            while (m_pos < m_text.size() && m_text[m_pos] != '\r' && m_text[m_pos] != '\n')
            {
                ++m_pos;
            }
            return true;
        }

        if (m_text[m_pos + 1] == '*')
        {
            m_pos += 2;
            while (m_pos + 1 < m_text.size() && (m_text[m_pos] != '*' || m_text[m_pos + 1] != '/'))
            {
                ++m_pos;
            }

            if (m_pos + 1 < m_text.size())
            {
                m_pos += 2;
            }
            return true;
        }

        return false;
    }

    bool ReadQuotedToken(std::string& token)
    {
        ++m_pos;
        const std::size_t begin = m_pos;
        while (m_pos < m_text.size() && m_text[m_pos] != '"')
        {
            ++m_pos;
        }

        token.assign(m_text.begin() + begin, m_text.begin() + m_pos);
        if (m_pos < m_text.size() && m_text[m_pos] == '"')
        {
            ++m_pos;
        }

        return true;
    }

    const std::string& m_text;
    std::size_t m_pos = 0;
};

bool IsXTextSeparatorToken(const std::string& token)
{
    return token == "," || token == ";";
}

struct CustomXMaterialData
{
    D3DMATERIAL9 material { };
    std::string textureFilename;
};

struct CustomXSkinWeightsData
{
    std::string boneName;
    std::vector<DWORD> vertexIndices;
    std::vector<float> weights;
    D3DXMATRIX offsetMatrix;
};

struct CustomXMeshData
{
    std::vector<D3DXVECTOR3> positions;
    std::vector<std::vector<DWORD>> faces;
    std::vector<D3DXVECTOR2> texCoords;
    std::vector<DWORD> faceMaterialIndices;
    std::vector<CustomXMaterialData> materials;
    std::vector<CustomXSkinWeightsData> skinWeights;
};

SkinAnimMeshFrame* CreateCustomXFrame(const std::string& name)
{
    SkinAnimMeshFrame* frame = NEW SkinAnimMeshFrame();
    ZeroMemory(frame, sizeof(SkinAnimMeshFrame));

    frame->Name = NEW char[name.size() + 1];
    strcpy_s(frame->Name, name.size() + 1, name.c_str());

    D3DXMatrixIdentity(&frame->TransformationMatrix);
    D3DXMatrixIdentity(&frame->m_combinedMatrix);
    D3DXQuaternionIdentity(&frame->m_animationRotation);
    frame->m_animationScale = D3DXVECTOR3(1.0f, 1.0f, 1.0f);
    frame->m_animationPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    return frame;
}

void AppendCustomXChildFrame(SkinAnimMeshFrame* parent, SkinAnimMeshFrame* child)
{
    if (parent->pFrameFirstChild == nullptr)
    {
        parent->pFrameFirstChild = child;
        return;
    }

    LPD3DXFRAME sibling = parent->pFrameFirstChild;
    while (sibling->pFrameSibling != nullptr)
    {
        sibling = sibling->pFrameSibling;
    }

    sibling->pFrameSibling = child;
}

void AppendCustomXMeshContainer(SkinAnimMeshFrame* frame, LPD3DXMESHCONTAINER meshContainer)
{
    if (frame->pMeshContainer == nullptr)
    {
        frame->pMeshContainer = meshContainer;
        return;
    }

    LPD3DXMESHCONTAINER container = frame->pMeshContainer;
    while (container->pNextMeshContainer != nullptr)
    {
        container = container->pNextMeshContainer;
    }

    container->pNextMeshContainer = meshContainer;
}

bool ReadExpectedXToken(XTextTokenizer& tokenizer, const char* expected)
{
    std::string token;
    if (!tokenizer.ReadToken(token))
    {
        return false;
    }

    return token == expected;
}

bool ReadXFloatToken(XTextTokenizer& tokenizer, float& value)
{
    return tokenizer.SkipSeparatorsAndReadFloat(value);
}

bool ReadXUIntToken(XTextTokenizer& tokenizer, DWORD& value)
{
    return tokenizer.SkipSeparatorsAndReadUInt(value);
}

bool ReadXStringToken(XTextTokenizer& tokenizer, std::string& value)
{
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        value = token;
        return true;
    }

    return false;
}

bool ParseCustomXFrameTransformMatrix(XTextTokenizer& tokenizer, SkinAnimMeshFrame* frame)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    float values[16] { };
    for (int i = 0; i < 16; ++i)
    {
        if (!ReadXFloatToken(tokenizer, values[i]))
        {
            return false;
        }
    }

    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            frame->TransformationMatrix(row, column) = values[(row * 4) + column];
        }
    }

    int depth = 1;
    std::string token;
    while (depth > 0 && tokenizer.ReadToken(token))
    {
        if (token == "{")
        {
            ++depth;
        }
        else if (token == "}")
        {
            --depth;
        }
    }

    return depth == 0;
}

bool SkipCustomXObjectBody(XTextTokenizer& tokenizer)
{
    int depth = 1;
    std::string token;
    while (depth > 0 && tokenizer.ReadToken(token))
    {
        if (token == "{")
        {
            ++depth;
        }
        else if (token == "}")
        {
            --depth;
        }
    }

    return depth == 0;
}

bool SkipCustomXObject(XTextTokenizer& tokenizer)
{
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (token == "{")
        {
            return SkipCustomXObjectBody(tokenizer);
        }

        if (token == "}")
        {
            return true;
        }
    }

    return true;
}

bool ReadCustomXOpenBrace(XTextTokenizer& tokenizer)
{
    std::string token;
    if (!tokenizer.ReadToken(token))
    {
        return false;
    }

    if (token == "{")
    {
        return true;
    }

    return ReadExpectedXToken(tokenizer, "{");
}

bool ReadCustomXMatrix(XTextTokenizer& tokenizer, D3DXMATRIX& matrix)
{
    float values[16] { };
    for (int i = 0; i < 16; ++i)
    {
        if (!ReadXFloatToken(tokenizer, values[i]))
        {
            return false;
        }
    }

    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            matrix(row, column) = values[(row * 4) + column];
        }
    }

    return true;
}

bool ParseCustomXMaterial(XTextTokenizer& tokenizer, CustomXMaterialData& materialData)
{
    if (!ReadCustomXOpenBrace(tokenizer))
    {
        return false;
    }

    ZeroMemory(&materialData.material, sizeof(materialData.material));
    materialData.material.Diffuse.a = 1.0f;
    materialData.material.Ambient.a = 1.0f;

    if (!ReadXFloatToken(tokenizer, materialData.material.Diffuse.r) ||
        !ReadXFloatToken(tokenizer, materialData.material.Diffuse.g) ||
        !ReadXFloatToken(tokenizer, materialData.material.Diffuse.b) ||
        !ReadXFloatToken(tokenizer, materialData.material.Diffuse.a) ||
        !ReadXFloatToken(tokenizer, materialData.material.Power) ||
        !ReadXFloatToken(tokenizer, materialData.material.Specular.r) ||
        !ReadXFloatToken(tokenizer, materialData.material.Specular.g) ||
        !ReadXFloatToken(tokenizer, materialData.material.Specular.b) ||
        !ReadXFloatToken(tokenizer, materialData.material.Emissive.r) ||
        !ReadXFloatToken(tokenizer, materialData.material.Emissive.g) ||
        !ReadXFloatToken(tokenizer, materialData.material.Emissive.b))
    {
        return false;
    }

    materialData.material.Ambient = materialData.material.Diffuse;

    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "}")
        {
            return true;
        }

        if (token == "TextureFilename")
        {
            if (!ReadExpectedXToken(tokenizer, "{") ||
                !ReadXStringToken(tokenizer, materialData.textureFilename) ||
                !SkipCustomXObjectBody(tokenizer))
            {
                return false;
            }
            continue;
        }

        if (!SkipCustomXObject(tokenizer))
        {
            return false;
        }
    }

    return false;
}

bool ParseCustomXMeshMaterialList(XTextTokenizer& tokenizer, CustomXMeshData& meshData)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    DWORD materialCount = 0;
    DWORD faceMaterialCount = 0;
    if (!ReadXUIntToken(tokenizer, materialCount) ||
        !ReadXUIntToken(tokenizer, faceMaterialCount))
    {
        return false;
    }

    meshData.faceMaterialIndices.clear();
    meshData.faceMaterialIndices.reserve(faceMaterialCount);
    for (DWORD i = 0; i < faceMaterialCount; ++i)
    {
        DWORD materialIndex = 0;
        if (!ReadXUIntToken(tokenizer, materialIndex))
        {
            return false;
        }
        meshData.faceMaterialIndices.push_back(materialIndex);
    }

    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "}")
        {
            return true;
        }

        if (token == "Material")
        {
            CustomXMaterialData materialData;
            if (!ParseCustomXMaterial(tokenizer, materialData))
            {
                return false;
            }
            meshData.materials.push_back(materialData);
            continue;
        }

        if (!SkipCustomXObject(tokenizer))
        {
            return false;
        }
    }

    return meshData.materials.size() == materialCount;
}

bool ParseCustomXMeshTextureCoords(XTextTokenizer& tokenizer, CustomXMeshData& meshData)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    DWORD textureCoordCount = 0;
    if (!ReadXUIntToken(tokenizer, textureCoordCount))
    {
        return false;
    }

    meshData.texCoords.resize(textureCoordCount);
    for (DWORD i = 0; i < textureCoordCount; ++i)
    {
        if (!ReadXFloatToken(tokenizer, meshData.texCoords[i].x) ||
            !ReadXFloatToken(tokenizer, meshData.texCoords[i].y))
        {
            return false;
        }
    }

    return SkipCustomXObjectBody(tokenizer);
}

bool ParseCustomXSkinWeights(XTextTokenizer& tokenizer, CustomXMeshData& meshData)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    CustomXSkinWeightsData weightsData;
    D3DXMatrixIdentity(&weightsData.offsetMatrix);
    if (!ReadXStringToken(tokenizer, weightsData.boneName))
    {
        return false;
    }

    DWORD weightCount = 0;
    if (!ReadXUIntToken(tokenizer, weightCount))
    {
        return false;
    }

    weightsData.vertexIndices.resize(weightCount);
    for (DWORD i = 0; i < weightCount; ++i)
    {
        if (!ReadXUIntToken(tokenizer, weightsData.vertexIndices[i]))
        {
            return false;
        }
    }

    weightsData.weights.resize(weightCount);
    for (DWORD i = 0; i < weightCount; ++i)
    {
        if (!ReadXFloatToken(tokenizer, weightsData.weights[i]))
        {
            return false;
        }
    }

    if (!ReadCustomXMatrix(tokenizer, weightsData.offsetMatrix))
    {
        return false;
    }

    if (!SkipCustomXObjectBody(tokenizer))
    {
        return false;
    }

    meshData.skinWeights.push_back(weightsData);
    return true;
}

bool ParseCustomXMeshNormals(XTextTokenizer& tokenizer)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    return SkipCustomXObjectBody(tokenizer);
}

bool FillCustomXMeshBuffers(const CustomXMeshData& meshData,
                            LPD3DXMESH mesh,
                            const std::vector<DWORD>& triangleMaterialIndices)
{
    struct CustomXVertex
    {
        float x;
        float y;
        float z;
        float nx;
        float ny;
        float nz;
        float u;
        float v;
    };

    CustomXVertex* vertices = nullptr;
    if (FAILED(mesh->LockVertexBuffer(0, reinterpret_cast<void**>(&vertices))))
    {
        return false;
    }

    for (DWORD i = 0; i < static_cast<DWORD>(meshData.positions.size()); ++i)
    {
        vertices[i].x = meshData.positions[i].x;
        vertices[i].y = meshData.positions[i].y;
        vertices[i].z = meshData.positions[i].z;
        vertices[i].nx = 0.0f;
        vertices[i].ny = 1.0f;
        vertices[i].nz = 0.0f;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
        if (i < meshData.texCoords.size())
        {
            vertices[i].u = meshData.texCoords[i].x;
            vertices[i].v = meshData.texCoords[i].y;
        }
    }
    mesh->UnlockVertexBuffer();

    DWORD* indices = nullptr;
    if (FAILED(mesh->LockIndexBuffer(0, reinterpret_cast<void**>(&indices))))
    {
        return false;
    }

    DWORD triangleIndex = 0;
    for (const auto& face : meshData.faces)
    {
        if (face.size() < 3)
        {
            continue;
        }

        for (std::size_t i = 1; i + 1 < face.size(); ++i)
        {
            indices[(triangleIndex * 3) + 0] = face[0];
            indices[(triangleIndex * 3) + 1] = face[i];
            indices[(triangleIndex * 3) + 2] = face[i + 1];
            ++triangleIndex;
        }
    }
    mesh->UnlockIndexBuffer();

    DWORD* attributes = nullptr;
    if (FAILED(mesh->LockAttributeBuffer(0, &attributes)))
    {
        return false;
    }

    for (DWORD i = 0; i < static_cast<DWORD>(triangleMaterialIndices.size()); ++i)
    {
        attributes[i] = triangleMaterialIndices[i];
    }
    mesh->UnlockAttributeBuffer();
    return true;
}

bool CreateCustomXSkinInfo(const CustomXMeshData& meshData,
                           const DWORD fvf,
                           LPD3DXSKININFO* skinInfo)
{
    if (skinInfo == nullptr || meshData.skinWeights.empty())
    {
        return false;
    }

    *skinInfo = nullptr;
    HRESULT hr = D3DXCreateSkinInfoFVF(static_cast<UINT>(meshData.positions.size()),
                                       fvf,
                                       static_cast<UINT>(meshData.skinWeights.size()),
                                       skinInfo);
    if (FAILED(hr) || *skinInfo == nullptr)
    {
        return false;
    }

    for (DWORD i = 0; i < static_cast<DWORD>(meshData.skinWeights.size()); ++i)
    {
        const CustomXSkinWeightsData& weightsData = meshData.skinWeights[i];
        (*skinInfo)->SetBoneName(i, weightsData.boneName.c_str());
        (*skinInfo)->SetBoneOffsetMatrix(i, &weightsData.offsetMatrix);
        if (!weightsData.vertexIndices.empty())
        {
            hr = (*skinInfo)->SetBoneInfluence(i,
                                                static_cast<DWORD>(weightsData.vertexIndices.size()),
                                                &weightsData.vertexIndices[0],
                                                &weightsData.weights[0]);
            if (FAILED(hr))
            {
                SAFE_RELEASE(*skinInfo);
                return false;
            }
        }
    }

    return true;
}

bool CreateCustomXMeshContainer(const std::string& meshName,
                                const CustomXMeshData& meshData,
                                SkinAnimMeshAlloc* allocator,
                                LPD3DXMESHCONTAINER* meshContainer)
{
    if (allocator == nullptr || meshContainer == nullptr ||
        meshData.positions.empty() || meshData.faces.empty() ||
        meshData.materials.empty() || meshData.skinWeights.empty())
    {
        return false;
    }

    std::vector<DWORD> triangleMaterialIndices;
    DWORD triangleCount = 0;
    for (std::size_t faceIndex = 0; faceIndex < meshData.faces.size(); ++faceIndex)
    {
        const std::vector<DWORD>& face = meshData.faces[faceIndex];
        if (face.size() < 3)
        {
            continue;
        }

        DWORD materialIndex = 0;
        if (faceIndex < meshData.faceMaterialIndices.size())
        {
            materialIndex = meshData.faceMaterialIndices[faceIndex];
        }

        for (std::size_t i = 1; i + 1 < face.size(); ++i)
        {
            triangleMaterialIndices.push_back(materialIndex);
            ++triangleCount;
        }
    }

    if (triangleCount == 0)
    {
        return false;
    }

    const DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
    LPD3DXMESH mesh = nullptr;
    HRESULT hr = D3DXCreateMeshFVF(triangleCount,
                                    static_cast<DWORD>(meshData.positions.size()),
                                    D3DXMESH_MANAGED | D3DXMESH_32BIT,
                                    fvf,
                                    Common::D3DDevice(),
                                    &mesh);
    if (FAILED(hr) || mesh == nullptr)
    {
        return false;
    }

    if (!FillCustomXMeshBuffers(meshData, mesh, triangleMaterialIndices))
    {
        SAFE_RELEASE(mesh);
        return false;
    }

    std::vector<DWORD> adjacency(triangleCount * 3, 0xffffffff);

    hr = mesh->GenerateAdjacency(0.0001f, &adjacency[0]);
    if (FAILED(hr))
    {
        WriteMeshMixSkinAnimLoadLog(L"Custom mesh failed: GenerateAdjacency failed. HR=" +
                                    FormatHRESULT(hr));
        SAFE_RELEASE(mesh);
        return false;
    }

    hr = D3DXComputeNormals(mesh, &adjacency[0]);
    if (FAILED(hr))
    {
        WriteMeshMixSkinAnimLoadLog(L"Custom mesh failed: D3DXComputeNormals failed. HR=" +
                                    FormatHRESULT(hr));
        SAFE_RELEASE(mesh);
        return false;
    }

    LPD3DXSKININFO skinInfo = nullptr;
    if (!CreateCustomXSkinInfo(meshData, fvf, &skinInfo))
    {
        SAFE_RELEASE(mesh);
        return false;
    }

    std::vector<D3DXMATERIAL> materials(meshData.materials.size());
    for (std::size_t i = 0; i < meshData.materials.size(); ++i)
    {
        materials[i].MatD3D = meshData.materials[i].material;
        materials[i].pTextureFilename = nullptr;
        if (!meshData.materials[i].textureFilename.empty())
        {
            materials[i].pTextureFilename = const_cast<char*>(meshData.materials[i].textureFilename.c_str());
        }
    }

    D3DXMESHDATA meshDataForAllocator { };
    meshDataForAllocator.Type = D3DXMESHTYPE_MESH;
    meshDataForAllocator.pMesh = mesh;

    hr = allocator->CreateMeshContainer(meshName.c_str(),
                                         &meshDataForAllocator,
                                         &materials[0],
                                         nullptr,
                                         static_cast<DWORD>(materials.size()),
                                         &adjacency[0],
                                         skinInfo,
                                         meshContainer);
    SAFE_RELEASE(skinInfo);
    SAFE_RELEASE(mesh);
    return SUCCEEDED(hr) && *meshContainer != nullptr;
}

bool ParseCustomXMesh(XTextTokenizer& tokenizer,
                      SkinAnimMeshFrame* frame,
                      SkinAnimMeshAlloc* allocator)
{
    std::string token;
    std::string meshName = "CustomXMesh";
    if (!tokenizer.ReadToken(token))
    {
        return false;
    }

    if (token != "{")
    {
        meshName = token;
        if (!ReadExpectedXToken(tokenizer, "{"))
        {
            return false;
        }
    }

    CustomXMeshData meshData;
    DWORD vertexCount = 0;
    if (!ReadXUIntToken(tokenizer, vertexCount))
    {
        return false;
    }

    meshData.positions.resize(vertexCount);
    for (DWORD i = 0; i < vertexCount; ++i)
    {
        if (!ReadXFloatToken(tokenizer, meshData.positions[i].x) ||
            !ReadXFloatToken(tokenizer, meshData.positions[i].y) ||
            !ReadXFloatToken(tokenizer, meshData.positions[i].z))
        {
            return false;
        }
    }

    DWORD faceCount = 0;
    if (!ReadXUIntToken(tokenizer, faceCount))
    {
        return false;
    }

    meshData.faces.resize(faceCount);
    for (DWORD faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        DWORD indexCount = 0;
        if (!ReadXUIntToken(tokenizer, indexCount))
        {
            return false;
        }

        meshData.faces[faceIndex].resize(indexCount);
        for (DWORD i = 0; i < indexCount; ++i)
        {
            if (!ReadXUIntToken(tokenizer, meshData.faces[faceIndex][i]))
            {
                return false;
            }
        }
    }

    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "}")
        {
            LPD3DXMESHCONTAINER meshContainer = nullptr;
            if (allocator != nullptr)
            {
                if (!CreateCustomXMeshContainer(meshName, meshData, allocator, &meshContainer))
                {
                    return false;
                }

                AppendCustomXMeshContainer(frame, meshContainer);
            }
            return true;
        }

        if (token == "MeshNormals")
        {
            if (!ParseCustomXMeshNormals(tokenizer))
            {
                return false;
            }
            continue;
        }

        if (token == "MeshTextureCoords")
        {
            if (!ParseCustomXMeshTextureCoords(tokenizer, meshData))
            {
                return false;
            }
            continue;
        }

        if (token == "MeshMaterialList")
        {
            if (!ParseCustomXMeshMaterialList(tokenizer, meshData))
            {
                return false;
            }
            continue;
        }

        if (token == "SkinWeights")
        {
            if (!ParseCustomXSkinWeights(tokenizer, meshData))
            {
                return false;
            }
            continue;
        }

        if (!SkipCustomXObject(tokenizer))
        {
            return false;
        }
    }

    return false;
}

bool ParseCustomXFrameBody(XTextTokenizer& tokenizer, SkinAnimMeshFrame* frame, SkinAnimMeshAlloc* allocator, CustomXLoadPurpose loadPurpose);

SkinAnimMeshFrame* ParseCustomXFrame(XTextTokenizer& tokenizer, SkinAnimMeshAlloc* allocator, CustomXLoadPurpose loadPurpose)
{
    std::string frameName;
    if (!tokenizer.ReadToken(frameName))
    {
        return nullptr;
    }

    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return nullptr;
    }

    SkinAnimMeshFrame* frame = CreateCustomXFrame(frameName);
    if (!ParseCustomXFrameBody(tokenizer, frame, allocator, loadPurpose))
    {
        SAFE_DELETE_ARRAY(frame->Name);
        SAFE_DELETE(frame);
        return nullptr;
    }

    return frame;
}

bool ParseCustomXFrameBody(XTextTokenizer& tokenizer, SkinAnimMeshFrame* frame, SkinAnimMeshAlloc* allocator, CustomXLoadPurpose loadPurpose)
{
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "}")
        {
            return true;
        }

        if (token == "Frame")
        {
            SkinAnimMeshFrame* child = ParseCustomXFrame(tokenizer, allocator, loadPurpose);
            if (child == nullptr)
            {
                return false;
            }

            AppendCustomXChildFrame(frame, child);
            continue;
        }

        if (token == "FrameTransformMatrix")
        {
            if (!ParseCustomXFrameTransformMatrix(tokenizer, frame))
            {
                return false;
            }
            continue;
        }

        if (token == "Mesh")
        {
            if (loadPurpose == CustomXLoadPurpose::AnimationOnly)
            {
                if (!SkipCustomXObject(tokenizer))
                {
                    return false;
                }
                continue;
            }

            if (!ParseCustomXMesh(tokenizer, frame, allocator))
            {
                return false;
            }
            continue;
        }

        if (!SkipCustomXObject(tokenizer))
        {
            return false;
        }
    }

    return false;
}

bool ParseCustomXAnimationKeyBody(XTextTokenizer& tokenizer, CustomXAnimationKey& key)
{
    DWORD keyType = 0;
    if (!ReadXUIntToken(tokenizer, keyType))
    {
        return false;
    }
    key.keyType = keyType;

    switch (keyType)
    {
        case 0: key.valueCount = 4; break;
        case 1: key.valueCount = 3; break;
        case 2: key.valueCount = 3; break;
        case 4: key.valueCount = 16; break;
        default: return false;
    }

    DWORD keyCount = 0;
    if (!ReadXUIntToken(tokenizer, keyCount))
    {
        return false;
    }

    key.times.resize(keyCount);
    key.values.resize(static_cast<std::size_t>(keyCount) * key.valueCount);

    for (DWORD i = 0; i < keyCount; ++i)
    {
        float timeFloat = 0.0f;
        if (!ReadXFloatToken(tokenizer, timeFloat))
        {
            return false;
        }
        key.times[i] = static_cast<double>(timeFloat);

        DWORD valueCount = 0;
        if (!ReadXUIntToken(tokenizer, valueCount))
        {
            return false;
        }

        if (valueCount != key.valueCount)
        {
            return false;
        }

        for (DWORD v = 0; v < key.valueCount; ++v)
        {
            float value = 0.0f;
            if (!ReadXFloatToken(tokenizer, value))
            {
                return false;
            }
            key.values[static_cast<std::size_t>(i) * key.valueCount + v] = value;
        }
    }

    return true;
}

bool ParseCustomXAnimationKey(XTextTokenizer& tokenizer, CustomXAnimationKey& key)
{
    if (!ReadCustomXOpenBrace(tokenizer))
    {
        return false;
    }

    if (!ParseCustomXAnimationKeyBody(tokenizer, key))
    {
        return false;
    }

    return SkipCustomXObjectBody(tokenizer);
}

bool ParseCustomXAnimation(XTextTokenizer& tokenizer, CustomXAnimation& anim)
{
    return false;
}

bool ParseCustomXAnimationBlockBody(XTextTokenizer& tokenizer, CustomXAnimation& anim)
{
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token)) continue;
        if (token == "}") return true;

        if (token == "AnimationKey")
        {
            CustomXAnimationKey key;
            if (!ParseCustomXAnimationKey(tokenizer, key)) return false;
            anim.keys.push_back(key);
            continue;
        }
        if (token == "{")
        {
            if (!SkipCustomXObjectBody(tokenizer)) return false;
            continue;
        }
        if (!SkipCustomXObject(tokenizer)) return false;
    }
    return false;
}

bool ParseCustomXAnimationBlock(XTextTokenizer& tokenizer,
                                std::vector<CustomXAnimation>& outAnimations)
{
    std::string token;
    if (!tokenizer.ReadToken(token))
    {
        return false;
    }

    std::string optionalAnimName;
    if (token != "{")
    {
        optionalAnimName = token;
        if (!ReadExpectedXToken(tokenizer, "{"))
        {
            return false;
        }
    }

    CustomXAnimation* currentAnim = nullptr;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "}")
        {
            return true;
        }

        if (token == "{")
        {
            std::string boneName;
            if (!tokenizer.ReadToken(boneName))
            {
                return false;
            }

            if (boneName == "}")
            {
                continue;
            }

            if (!ReadExpectedXToken(tokenizer, "}"))
            {
                return false;
            }

            CustomXAnimation subAnim;
            subAnim.frameName = boneName;
            outAnimations.push_back(subAnim);
            currentAnim = &outAnimations.back();
            continue;
        }

        if (token == "AnimationKey")
        {
            if (currentAnim == nullptr)
            {
                if (!SkipCustomXObject(tokenizer))
                {
                    return false;
                }
                continue;
            }

            CustomXAnimationKey key;
            if (!ParseCustomXAnimationKey(tokenizer, key))
            {
                return false;
            }

            currentAnim->keys.push_back(key);
            continue;
        }

        if (!SkipCustomXObject(tokenizer))
        {
            return false;
        }
    }

    return false;
}

bool ParseCustomXAnimationOptions(XTextTokenizer& tokenizer, double& ticksPerSecond)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    DWORD openClosed = 0;
    if (!ReadXUIntToken(tokenizer, openClosed))
    {
        return false;
    }

    DWORD positionQuality = 0;
    if (!ReadXUIntToken(tokenizer, positionQuality))
    {
        return false;
    }

    return SkipCustomXObjectBody(tokenizer);
}

bool ParseCustomXAnimationSet(XTextTokenizer& tokenizer, CustomXAnimationSet& animSet)
{
    if (!ReadXStringToken(tokenizer, animSet.name))
    {
        return false;
    }

    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "}")
        {
            return true;
        }

        if (token == "Animation")
        {
            std::vector<CustomXAnimation> animations;
            if (!ParseCustomXAnimationBlock(tokenizer, animations))
            {
                return false;
            }
            for (auto& anim : animations)
                animSet.animations.push_back(std::move(anim));
            continue;
        }

        if (token == "AnimationOptions")
        {
            ParseCustomXAnimationOptions(tokenizer, animSet.ticksPerSecond);
            continue;
        }

        if (token == "{")
        {
            if (!SkipCustomXObjectBody(tokenizer))
            {
                return false;
            }
            continue;
        }

        if (!SkipCustomXObject(tokenizer))
        {
            return false;
        }
    }

    return false;
}

} // anonymous namespace

HRESULT CreateAnimationControllerFromParsedData(const std::vector<CustomXAnimationSet>& animationSets,
                                                 LPD3DXFRAME frameRoot,
                                                 LPD3DXANIMATIONCONTROLLER* outController)
{
    if (outController == nullptr)
    {
        return E_POINTER;
    }

    *outController = nullptr;

    if (animationSets.empty() || frameRoot == nullptr)
    {
        WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: skipping, animSets=" +
                                    std::to_wstring(animationSets.size()) +
                                    L" frameRoot=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(frameRoot)));
        return S_OK;
    }

    std::set<std::string> animatedFrameNames;
    for (const auto& animSet : animationSets)
    {
        for (const auto& anim : animSet.animations)
        {
            if (!anim.frameName.empty())
            {
                animatedFrameNames.insert(anim.frameName);
            }
        }
    }

    WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: uniqueAnimatedFrames=" +
                                std::to_wstring(animatedFrameNames.size()));

    if (animatedFrameNames.empty())
    {
        WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: no animated frame names, skipping.");
        return S_OK;
    }

    const UINT maxOutputs = static_cast<UINT>(animatedFrameNames.size());
    const UINT maxSets = static_cast<UINT>(animationSets.size());
    const UINT maxTracks = 1;
    const UINT maxEvents = 16;

    LPD3DXANIMATIONCONTROLLER controller = nullptr;
    HRESULT hr = D3DXCreateAnimationController(maxOutputs, maxSets, maxTracks, maxEvents, &controller);
    WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: D3DXCreateAnimationController HR=" + FormatHRESULT(hr) +
                                L" ptr=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(controller)) +
                                L" maxOutputs=" + std::to_wstring(maxOutputs) +
                                L" maxSets=" + std::to_wstring(maxSets) +
                                L" maxEvents=" + std::to_wstring(maxEvents));
    if (FAILED(hr) || controller == nullptr)
    {
        SAFE_RELEASE(controller);
        return E_FAIL;
    }

    WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: controller created OK. Registering " +
                                std::to_wstring(maxOutputs) + L" outputs...");

    int registeredOutputs = 0;
    int failedOutputs = 0;
    for (const auto& name : animatedFrameNames)
    {
        LPD3DXFRAME rawFrame = D3DXFrameFind(frameRoot, name.c_str());
        if (rawFrame == nullptr)
        {
            ++failedOutputs;
            WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: frame NOT FOUND for output '" +
                                        AnsiTextToWideText(name) + L"'");
            continue;
        }

        SkinAnimMeshFrame* skinFrame = reinterpret_cast<SkinAnimMeshFrame*>(rawFrame);
        hr = controller->RegisterAnimationOutput(name.c_str(),
                                                  &skinFrame->TransformationMatrix,
                                                  nullptr,
                                                  nullptr,
                                                  nullptr);
        if (FAILED(hr))
        {
            ++failedOutputs;
            WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: RegisterAnimationOutput FAILED for '" +
                                        AnsiTextToWideText(name) +
                                        L"'. HR=" + FormatHRESULT(hr));
            continue;
        }

        ++registeredOutputs;
    }

    WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: output registration done. OK=" +
                                std::to_wstring(registeredOutputs) +
                                L" FAIL=" + std::to_wstring(failedOutputs));

    if (registeredOutputs == 0)
    {
        WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: zero outputs registered, aborting.");
        SAFE_RELEASE(controller);
        return E_FAIL;
    }

    DWORD animSetIndex = 0;
    for (const auto& animSet : animationSets)
    {
        const DWORD numAnimations = static_cast<DWORD>(animSet.animations.size());
        WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: creating animSet '" +
                                    AnsiTextToWideText(animSet.name) +
                                    L"' numAnimations=" + std::to_wstring(numAnimations) +
                                    L" tps=" + std::to_wstring(animSet.ticksPerSecond));

        LPD3DXKEYFRAMEDANIMATIONSET d3dxAnimSet = nullptr;
        hr = D3DXCreateKeyframedAnimationSet(animSet.name.c_str(),
                                              animSet.ticksPerSecond,
                                              D3DXPLAY_LOOP,
                                              numAnimations,
                                              0,
                                              nullptr,
                                              &d3dxAnimSet);
        if (FAILED(hr) || d3dxAnimSet == nullptr)
        {
            WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: D3DXCreateKeyframedAnimationSet FAILED. HR=" +
                                        FormatHRESULT(hr));
            SAFE_RELEASE(controller);
            if (FAILED(hr))
            {
                return hr;
            }
            return E_FAIL;
        }

        bool anySrtKeyFailed = false;
        for (DWORD animIndex = 0; animIndex < numAnimations; ++animIndex)
        {
            const CustomXAnimation& anim = animSet.animations[animIndex];
            std::vector<D3DXKEY_QUATERNION> rotKeys;
            std::vector<D3DXKEY_VECTOR3> scaleKeys;
            std::vector<D3DXKEY_VECTOR3> posKeys;

            for (const auto& key : anim.keys)
            {
                for (std::size_t k = 0; k < key.times.size(); ++k)
                {
                    const double t = key.times[k];
                    const float* vals = &key.values[k * key.valueCount];

                    if (key.keyType == 0)
                    {
                        D3DXKEY_QUATERNION qKey;
                        qKey.Time = static_cast<float>(t);
                        qKey.Value.w = vals[0];
                        qKey.Value.x = vals[1];
                        qKey.Value.y = vals[2];
                        qKey.Value.z = vals[3];
                        rotKeys.push_back(qKey);
                    }
                    else if (key.keyType == 1)
                    {
                        D3DXKEY_VECTOR3 vKey;
                        vKey.Time = static_cast<float>(t);
                        vKey.Value.x = vals[0];
                        vKey.Value.y = vals[1];
                        vKey.Value.z = vals[2];
                        scaleKeys.push_back(vKey);
                    }
                    else if (key.keyType == 2)
                    {
                        D3DXKEY_VECTOR3 vKey;
                        vKey.Time = static_cast<float>(t);
                        vKey.Value.x = vals[0];
                        vKey.Value.y = vals[1];
                        vKey.Value.z = vals[2];
                        posKeys.push_back(vKey);
                    }
                    else if (key.keyType == 4)
                    {
                        D3DXMATRIX mat;
                        for (int row = 0; row < 4; ++row)
                        {
                            for (int col = 0; col < 4; ++col)
                            {
                                mat(row, col) = vals[row * 4 + col];
                            }
                        }

                        D3DXVECTOR3 s;
                        D3DXQUATERNION r;
                        D3DXVECTOR3 p;
                        D3DXMatrixDecompose(&s, &r, &p, &mat);

                        D3DXKEY_VECTOR3 sk;
                        sk.Time = static_cast<float>(t);
                        sk.Value.x = s.x;
                        sk.Value.y = s.y;
                        sk.Value.z = s.z;
                        scaleKeys.push_back(sk);

                        D3DXKEY_QUATERNION rk;
                        rk.Time = static_cast<float>(t);
                        rk.Value.w = r.w;
                        rk.Value.x = r.x;
                        rk.Value.y = r.y;
                        rk.Value.z = r.z;
                        rotKeys.push_back(rk);

                        D3DXKEY_VECTOR3 pk;
                        pk.Time = static_cast<float>(t);
                        pk.Value.x = p.x;
                        pk.Value.y = p.y;
                        pk.Value.z = p.z;
                        posKeys.push_back(pk);
                    }
                }
            }

            DWORD registeredIndex = 0;
            const D3DXKEY_VECTOR3* pScaleKeys = nullptr;
            if (!scaleKeys.empty())
            {
                pScaleKeys = &scaleKeys[0];
            }

            const D3DXKEY_QUATERNION* pRotKeys = nullptr;
            if (!rotKeys.empty())
            {
                pRotKeys = &rotKeys[0];
            }

            const D3DXKEY_VECTOR3* pPosKeys = nullptr;
            if (!posKeys.empty())
            {
                pPosKeys = &posKeys[0];
            }

            hr = d3dxAnimSet->RegisterAnimationSRTKeys(anim.frameName.c_str(),
                                                        static_cast<UINT>(scaleKeys.size()),
                                                        static_cast<UINT>(rotKeys.size()),
                                                        static_cast<UINT>(posKeys.size()),
                                                        pScaleKeys,
                                                        pRotKeys,
                                                        pPosKeys,
                                                        &registeredIndex);
            if (FAILED(hr))
            {
                WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: RegisterAnimationSRTKeys FAILED for '" +
                                            AnsiTextToWideText(anim.frameName) +
                                            L"' HR=" + FormatHRESULT(hr));
                anySrtKeyFailed = true;
            }
        }

        if (anySrtKeyFailed)
        {
            WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: one or more RegisterAnimationSRTKeys failed, aborting animSet '" +
                                        AnsiTextToWideText(animSet.name) + L"'");
            SAFE_RELEASE(d3dxAnimSet);
            SAFE_RELEASE(controller);
            return E_FAIL;
        }

        hr = controller->RegisterAnimationSet(d3dxAnimSet);
        WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: RegisterAnimationSet '" +
                                    AnsiTextToWideText(animSet.name) +
                                    L"' HR=" + FormatHRESULT(hr));
        if (FAILED(hr))
        {
            WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: RegisterAnimationSet FAILED, aborting.");
            SAFE_RELEASE(d3dxAnimSet);
            SAFE_RELEASE(controller);
            return hr;
        }

        if (animSetIndex == 0)
        {
            hr = controller->SetTrackAnimationSet(0, d3dxAnimSet);
            WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: SetTrackAnimationSet(0) HR=" + FormatHRESULT(hr));

            controller->SetTrackEnable(0, TRUE);
            controller->SetTrackWeight(0, 1.0f);
            controller->SetTrackSpeed(0, 1.0f);
            controller->SetTrackPosition(0, 0.0);

            WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: Track 0 initialized with animSet '" +
                                        AnsiTextToWideText(animSet.name) + L"'");
        }

        SAFE_RELEASE(d3dxAnimSet);
        ++animSetIndex;
    }

    WriteMeshMixSkinAnimLoadLog(L"BuildCtrl: SUCCESS. controller=" +
                                std::to_wstring(reinterpret_cast<std::uintptr_t>(controller)) +
                                L" animSets=" + std::to_wstring(animSetIndex));
    *outController = controller;
    return S_OK;
}

HRESULT LoadCustomXFrameHierarchyFromText(const std::string& fileText,
                                          SkinAnimMeshAlloc* allocator,
                                          LPD3DXFRAME* frameRoot,
                                          std::vector<CustomXAnimationSet>* outAnimationSets,
                                          CustomXLoadPurpose loadPurpose)
{
    if (frameRoot == nullptr)
    {
        WriteMeshMixSkinAnimLoadLog(L"Custom parser failed: frameRoot output pointer is null.");
        return E_POINTER;
    }

    *frameRoot = nullptr;
    if (outAnimationSets != nullptr)
    {
        outAnimationSets->clear();
    }

    WriteMeshMixSkinAnimLoadLog(L"Custom parser start. Bytes=" + std::to_wstring(fileText.size()));

    XTextTokenizer tokenizer(fileText);
    std::string token;
    bool frameFound = false;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "Frame")
        {
            WriteMeshMixSkinAnimLoadLog(L"Custom parser found top-level Frame.");
            SkinAnimMeshFrame* frame = ParseCustomXFrame(tokenizer, allocator, loadPurpose);
            if (frame == nullptr)
            {
                WriteMeshMixSkinAnimLoadLog(L"Custom parser failed while parsing top-level Frame.");
                return E_FAIL;
            }

            std::wstring frameName = L"(null)";
            if (frame->Name != nullptr)
            {
                frameName = AnsiTextToWideText(frame->Name);
            }
            WriteMeshMixSkinAnimLoadLog(L"Custom parser loaded top-level Frame: " + frameName);
            *frameRoot = frame;
            frameFound = true;
            continue;
        }

        if (token == "AnimationSet" && outAnimationSets != nullptr)
        {
            CustomXAnimationSet animSet;
            if (ParseCustomXAnimationSet(tokenizer, animSet))
            {
                WriteMeshMixSkinAnimLoadLog(L"Custom parser loaded AnimationSet: " +
                                            AnsiTextToWideText(animSet.name));
                outAnimationSets->push_back(animSet);
            }
            else
            {
                WriteMeshMixSkinAnimLoadLog(L"Custom parser failed while parsing AnimationSet.");
            }
            continue;
        }

        if (token == "template")
        {
            if (!SkipCustomXObject(tokenizer))
            {
                WriteMeshMixSkinAnimLoadLog(L"Custom parser failed while skipping template.");
                return E_FAIL;
            }
            continue;
        }
    }

    if (!frameFound)
    {
        WriteMeshMixSkinAnimLoadLog(L"Custom parser failed: no Frame token was found.");
        return E_FAIL;
    }

    return S_OK;
}

int CountCustomXFrames(const LPD3DXFRAME frame)
{
    if (frame == nullptr)
    {
        return 0;
    }

    return 1 + CountCustomXFrames(frame->pFrameFirstChild) + CountCustomXFrames(frame->pFrameSibling);
}

int CountCustomXMeshContainers(const LPD3DXFRAME frame)
{
    if (frame == nullptr)
    {
        return 0;
    }

    int count = 0;
    LPD3DXMESHCONTAINER container = frame->pMeshContainer;
    while (container != nullptr)
    {
        ++count;
        container = container->pNextMeshContainer;
    }

    return count +
           CountCustomXMeshContainers(frame->pFrameFirstChild) +
           CountCustomXMeshContainers(frame->pFrameSibling);
}

void DestroyCustomXFrameHierarchy(LPD3DXFRAME frame)
{
    if (frame == nullptr)
    {
        return;
    }

    LPD3DXFRAME sibling = frame->pFrameSibling;
    LPD3DXFRAME child = frame->pFrameFirstChild;
    frame->pFrameSibling = nullptr;
    frame->pFrameFirstChild = nullptr;

    DestroyCustomXFrameHierarchy(child);
    DestroyCustomXFrameHierarchy(sibling);

    SAFE_DELETE_ARRAY(frame->Name);
    SAFE_DELETE(frame);
}

void DestroyCustomXFrameHierarchyWithAllocator(LPD3DXFRAME frame, SkinAnimMeshAlloc& allocator)
{
    if (frame == nullptr)
    {
        return;
    }

    LPD3DXFRAME sibling = frame->pFrameSibling;
    LPD3DXFRAME child = frame->pFrameFirstChild;
    LPD3DXMESHCONTAINER container = frame->pMeshContainer;
    frame->pFrameSibling = nullptr;
    frame->pFrameFirstChild = nullptr;
    frame->pMeshContainer = nullptr;

    DestroyCustomXFrameHierarchyWithAllocator(child, allocator);
    DestroyCustomXFrameHierarchyWithAllocator(sibling, allocator);

    while (container != nullptr)
    {
        LPD3DXMESHCONTAINER next = container->pNextMeshContainer;
        container->pNextMeshContainer = nullptr;
        allocator.DestroyMeshContainer(container);
        container = next;
    }

    allocator.DestroyFrame(frame);
}

CustomXFrameHierarchyLoadResult LoadCustomXFrameHierarchyForTest(const std::wstring& filePath,
                                                                  const bool loadMeshContainers)
{
    CustomXFrameHierarchyLoadResult result;

    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        result.hr = E_FAIL;
        result.message = L"File open failed: " + filePath;
        return result;
    }

    const std::string fileText((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    LPD3DXFRAME frameRoot = nullptr;
    SkinAnimMeshAlloc allocator(filePath);
    SkinAnimMeshAlloc* allocatorPtr = nullptr;
    if (loadMeshContainers)
    {
        allocatorPtr = &allocator;
    }

    result.hr = LoadCustomXFrameHierarchyFromText(fileText, allocatorPtr, &frameRoot);
    if (SUCCEEDED(result.hr) && frameRoot != nullptr)
    {
        result.frameCount = CountCustomXFrames(frameRoot);
        result.meshContainerCount = CountCustomXMeshContainers(frameRoot);
        if (frameRoot->Name != nullptr)
        {
            result.rootFrameName = AnsiTextToWideText(frameRoot->Name);
        }
        result.message = L"Loaded successfully.";
    }
    else
    {
        result.message = L"Parser failed. Bytes=" + std::to_wstring(fileText.size());
    }

    if (loadMeshContainers)
    {
        DestroyCustomXFrameHierarchyWithAllocator(frameRoot, allocator);
    }
    else
    {
        DestroyCustomXFrameHierarchy(frameRoot);
    }
    return result;
}

} // namespace NSRender
