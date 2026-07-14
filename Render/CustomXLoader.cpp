#include "CustomXLoader.h"
#include "Common.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

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

    bool TrySkipDuplicateUIntValue(const DWORD expectedValue)
    {
        const std::size_t savedPos = m_pos;
        SkipSeparators();
        if (m_pos >= m_text.size())
        {
            m_pos = savedPos;
            return false;
        }

        char* endPtr = nullptr;
        const unsigned long parsed = std::strtoul(m_text.c_str() + m_pos, &endPtr, 10);
        if (endPtr == m_text.c_str() + m_pos)
        {
            m_pos = savedPos;
            return false;
        }

        const std::size_t parsedEnd = static_cast<std::size_t>(endPtr - m_text.c_str());
        std::size_t cursor = parsedEnd;
        while (cursor < m_text.size() && IsWhitespace(m_text[cursor]))
        {
            ++cursor;
        }

        if (cursor >= m_text.size() || (m_text[cursor] != ',' && m_text[cursor] != ';'))
        {
            m_pos = savedPos;
            return false;
        }

        if (static_cast<DWORD>(parsed) != expectedValue)
        {
            m_pos = savedPos;
            return false;
        }

        m_pos = cursor + 1;
        return true;
    }

    bool SkipObjectBodyFast()
    {
        int depth = 1;
        bool inString = false;
        bool inLineComment = false;
        bool inBlockComment = false;

        while (m_pos < m_text.size())
        {
            const char current = m_text[m_pos];

            if (inLineComment)
            {
                ++m_pos;
                if (current == '\r' || current == '\n')
                {
                    inLineComment = false;
                }
                continue;
            }

            if (inBlockComment)
            {
                if (current == '*' && (m_pos + 1) < m_text.size() && m_text[m_pos + 1] == '/')
                {
                    m_pos += 2;
                    inBlockComment = false;
                    continue;
                }

                ++m_pos;
                continue;
            }

            if (inString)
            {
                ++m_pos;
                if (current == '"')
                {
                    inString = false;
                }
                continue;
            }

            if (current == '"')
            {
                ++m_pos;
                inString = true;
                continue;
            }

            if (current == '/' && (m_pos + 1) < m_text.size())
            {
                const char next = m_text[m_pos + 1];
                if (next == '/')
                {
                    m_pos += 2;
                    inLineComment = true;
                    continue;
                }

                if (next == '*')
                {
                    m_pos += 2;
                    inBlockComment = true;
                    continue;
                }
            }

            if (current == '{')
            {
                ++depth;
                ++m_pos;
                continue;
            }

            if (current == '}')
            {
                --depth;
                ++m_pos;
                if (depth == 0)
                {
                    return true;
                }
                continue;
            }

            ++m_pos;
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

    void SkipSeparators()
    {
        bool skipped = true;
        while (skipped)
        {
            skipped = false;
            SkipWhitespace();
            while (m_pos < m_text.size() && (m_text[m_pos] == ',' || m_text[m_pos] == ';'))
            {
                ++m_pos;
                skipped = true;
                SkipWhitespace();
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
    std::vector<D3DXVECTOR3> normals;
    std::vector<std::vector<DWORD>> faces;
    std::vector<D3DXVECTOR2> texCoords;
    std::vector<DWORD> faceMaterialIndices;
    std::vector<CustomXMaterialData> materials;
    std::vector<CustomXSkinWeightsData> skinWeights;
};

struct CustomXParseContext
{
    std::map<std::string, std::string> frameParents;
    std::map<std::string, D3DXMATRIX> frameLocalMatrices;
    std::map<std::string, CustomXMaterialData> namedMaterials;
    CustomXLoadOptions options;
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

bool ReadCustomXNameAndOpenBrace(XTextTokenizer& tokenizer, std::string& name)
{
    name.clear();
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (token == "{")
        {
            return true;
        }

        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (!name.empty())
        {
            name += " ";
        }
        name += token;
    }

    return false;
}

bool ReadCustomXNameAndCloseBrace(XTextTokenizer& tokenizer, std::string& name)
{
    name.clear();
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (token == "}")
        {
            return !name.empty();
        }

        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (!name.empty())
        {
            name += " ";
        }
        name += token;
    }

    return false;
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

bool ParseCustomXMaterialBody(XTextTokenizer& tokenizer, CustomXMaterialData& materialData)
{
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

        if (token == "TextureFilename" || token == "TextureFileName")
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

bool ParseCustomXMaterial(XTextTokenizer& tokenizer, CustomXMaterialData& materialData)
{
    if (!ReadCustomXOpenBrace(tokenizer))
    {
        return false;
    }

    return ParseCustomXMaterialBody(tokenizer, materialData);
}

bool ParseCustomXNamedMaterial(XTextTokenizer& tokenizer,
                               std::string& materialName,
                               CustomXMaterialData& materialData)
{
    if (!ReadCustomXNameAndOpenBrace(tokenizer, materialName))
    {
        return false;
    }

    return ParseCustomXMaterialBody(tokenizer, materialData);
}

bool ParseCustomXMeshMaterialList(XTextTokenizer& tokenizer,
                                  CustomXMeshData& meshData,
                                  const CustomXParseContext* context)
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

        if (token == "{")
        {
            std::string materialName;
            if (!ReadCustomXNameAndCloseBrace(tokenizer, materialName))
            {
                return false;
            }

            if (context != nullptr)
            {
                const auto found = context->namedMaterials.find(materialName);
                if (found != context->namedMaterials.end())
                {
                    meshData.materials.push_back(found->second);
                    continue;
                }
            }

            return false;
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

bool ParseCustomXSkinWeights(XTextTokenizer& tokenizer,
                             CustomXMeshData& meshData,
                             const CustomXParseContext* context)
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

    if (context != nullptr && context->options.allowDuplicateSkinWeightsCount)
    {
        tokenizer.TrySkipDuplicateUIntValue(weightCount);
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

    CustomXSkinWeightsData filteredWeightsData;
    filteredWeightsData.boneName = weightsData.boneName;
    filteredWeightsData.offsetMatrix = weightsData.offsetMatrix;

    for (DWORD weightIndex = 0; weightIndex < weightCount; ++weightIndex)
    {
        if (weightsData.weights[weightIndex] <= 0.000001f)
        {
            continue;
        }

        filteredWeightsData.vertexIndices.push_back(weightsData.vertexIndices[weightIndex]);
        filteredWeightsData.weights.push_back(weightsData.weights[weightIndex]);
    }

    if (!filteredWeightsData.vertexIndices.empty())
    {
        meshData.skinWeights.push_back(filteredWeightsData);
    }

    return true;
}

bool ParseCustomXMeshNormals(XTextTokenizer& tokenizer, CustomXMeshData& meshData)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    DWORD normalCount = 0;
    if (!ReadXUIntToken(tokenizer, normalCount))
    {
        return false;
    }

    std::vector<D3DXVECTOR3> sourceNormals(normalCount);
    for (DWORD i = 0; i < normalCount; ++i)
    {
        if (!ReadXFloatToken(tokenizer, sourceNormals[i].x) ||
            !ReadXFloatToken(tokenizer, sourceNormals[i].y) ||
            !ReadXFloatToken(tokenizer, sourceNormals[i].z))
        {
            return false;
        }
    }

    DWORD faceNormalCount = 0;
    if (!ReadXUIntToken(tokenizer, faceNormalCount))
    {
        return false;
    }

    meshData.normals.assign(meshData.positions.size(), D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    for (DWORD faceIndex = 0; faceIndex < faceNormalCount; ++faceIndex)
    {
        DWORD faceNormalIndexCount = 0;
        if (!ReadXUIntToken(tokenizer, faceNormalIndexCount))
        {
            return false;
        }

        for (DWORD index = 0; index < faceNormalIndexCount; ++index)
        {
            DWORD normalIndex = 0;
            if (!ReadXUIntToken(tokenizer, normalIndex))
            {
                return false;
            }

            if (faceIndex < meshData.faces.size() &&
                index < meshData.faces[faceIndex].size() &&
                normalIndex < sourceNormals.size())
            {
                const DWORD vertexIndex = meshData.faces[faceIndex][index];
                if (vertexIndex < meshData.normals.size())
                {
                    meshData.normals[vertexIndex] = sourceNormals[normalIndex];
                }
            }
        }
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
        if (i < meshData.normals.size())
        {
            vertices[i].nx = meshData.normals[i].x;
            vertices[i].ny = meshData.normals[i].y;
            vertices[i].nz = meshData.normals[i].z;
        }
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

bool InvertCustomXMeshNormals(LPD3DXMESH mesh)
{
    if (mesh == nullptr)
    {
        return false;
    }

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

    const DWORD vertexCount = mesh->GetNumVertices();
    for (DWORD i = 0; i < vertexCount; ++i)
    {
        vertices[i].nx = -vertices[i].nx;
        vertices[i].ny = -vertices[i].ny;
        vertices[i].nz = -vertices[i].nz;
    }

    mesh->UnlockVertexBuffer();
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
    const UINT numBones = static_cast<UINT>(meshData.skinWeights.size());
    CUSTOM_X_LOADER_LOG(L"CreateSkinInfo: NumBones=" + std::to_wstring(numBones) +
                        L" VertexCount=" + std::to_wstring(meshData.positions.size()));

    HRESULT hr = D3DXCreateSkinInfoFVF(static_cast<UINT>(meshData.positions.size()),
                                        fvf,
                                        numBones,
                                        skinInfo);
    if (FAILED(hr) || *skinInfo == nullptr)
    {
        CUSTOM_X_LOADER_LOG(L"CreateSkinInfo FAILED. NumBones=" + std::to_wstring(numBones) +
                            L" HR=" + FormatHRESULT(hr));
        return false;
    }

    CUSTOM_X_LOADER_LOG(L"CreateSkinInfo OK. NumBones=" + std::to_wstring(numBones));

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
        meshData.materials.empty())
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
        CUSTOM_X_LOADER_LOG(L"Custom mesh failed: GenerateAdjacency failed. HR=" +
                                    FormatHRESULT(hr));
        SAFE_RELEASE(mesh);
        return false;
    }

    if (meshData.normals.empty() || !meshData.skinWeights.empty())
    {
        hr = D3DXComputeNormals(mesh, &adjacency[0]);
        if (FAILED(hr))
        {
            CUSTOM_X_LOADER_LOG(L"Custom mesh failed: D3DXComputeNormals failed. HR=" +
                                        FormatHRESULT(hr));
            SAFE_RELEASE(mesh);
            return false;
        }

        if (!InvertCustomXMeshNormals(mesh))
        {
            CUSTOM_X_LOADER_LOG(L"Custom mesh failed: normal inversion failed.");
            SAFE_RELEASE(mesh);
            return false;
        }
    }

    LPD3DXSKININFO skinInfo = nullptr;
    if (!meshData.skinWeights.empty() && !CreateCustomXSkinInfo(meshData, fvf, &skinInfo))
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
    if (SUCCEEDED(hr) && meshContainer != nullptr && *meshContainer != nullptr)
    {
        SkinAnimMeshContainer* skinContainer = reinterpret_cast<SkinAnimMeshContainer*>(*meshContainer);
        skinContainer->m_boneNames.clear();
        skinContainer->m_boneOffsetMatrices.clear();
        skinContainer->m_boneNames.reserve(meshData.skinWeights.size());
        skinContainer->m_boneOffsetMatrices.reserve(meshData.skinWeights.size());
        for (const CustomXSkinWeightsData& weightsData : meshData.skinWeights)
        {
            skinContainer->m_boneNames.push_back(weightsData.boneName);
            skinContainer->m_boneOffsetMatrices.push_back(weightsData.offsetMatrix);
        }
    }

    SAFE_RELEASE(skinInfo);
    SAFE_RELEASE(mesh);
    return SUCCEEDED(hr) && *meshContainer != nullptr;
}

constexpr DWORD MAX_SKININFO_BONES_PER_PART = 32;

std::string ToLowerAsciiText(const std::string& text)
{
    std::string result = text;
    for (std::size_t i = 0; i < result.size(); ++i)
    {
        if (result[i] >= 'A' && result[i] <= 'Z')
        {
            result[i] = static_cast<char>(result[i] - 'A' + 'a');
        }
    }
    return result;
}

bool ContainsText(const std::string& text, const char* pattern)
{
    return text.find(pattern) != std::string::npos;
}

bool IsCollapsiblePhysicsBoneName(const std::string& boneName)
{
    const std::string lowerName = ToLowerAsciiText(boneName);
    if (ContainsText(lowerName, "subhair") ||
        ContainsText(lowerName, "hair") ||
        ContainsText(lowerName, "coat") ||
        ContainsText(lowerName, "skirt") ||
        ContainsText(lowerName, "hat") ||
        ContainsText(lowerName, "cap"))
    {
        return true;
    }

    return false;
}

std::string FindSkinWeightCollapseTarget(const std::string& boneName,
                                         const CustomXParseContext& context)
{
    std::string currentName = boneName;
    while (IsCollapsiblePhysicsBoneName(currentName))
    {
        const auto parent = context.frameParents.find(currentName);
        if (parent == context.frameParents.end() || parent->second.empty())
        {
            return boneName;
        }

        currentName = parent->second;
    }

    return currentName;
}

D3DXMATRIX ComputeFrameCombinedMatrix(const std::string& frameName,
                                      const CustomXParseContext& context)
{
    D3DXMATRIX combined;
    D3DXMatrixIdentity(&combined);

    std::vector<std::string> chain;
    std::string currentName = frameName;
    while (!currentName.empty())
    {
        chain.push_back(currentName);
        const auto parent = context.frameParents.find(currentName);
        if (parent == context.frameParents.end())
        {
            break;
        }
        currentName = parent->second;
    }

    for (std::size_t i = 0; i < chain.size(); ++i)
    {
        const auto localMatrix = context.frameLocalMatrices.find(chain[i]);
        if (localMatrix != context.frameLocalMatrices.end())
        {
            combined *= localMatrix->second;
        }
    }

    return combined;
}

bool TryGetSkinWeightOffsetMatrix(const CustomXMeshData& meshData,
                                  const std::string& boneName,
                                  D3DXMATRIX& offsetMatrix)
{
    for (const CustomXSkinWeightsData& weightsData : meshData.skinWeights)
    {
        if (weightsData.boneName == boneName)
        {
            offsetMatrix = weightsData.offsetMatrix;
            return true;
        }
    }

    return false;
}

bool TryComputeFrameOffsetMatrix(const std::string& frameName,
                                 const CustomXParseContext& context,
                                 D3DXMATRIX& offsetMatrix)
{
    if (context.frameLocalMatrices.count(frameName) == 0)
    {
        return false;
    }

    const D3DXMATRIX combined = ComputeFrameCombinedMatrix(frameName, context);
    if (D3DXMatrixInverse(&offsetMatrix, nullptr, &combined) == nullptr)
    {
        return false;
    }

    return true;
}

void NormalizeMergedSkinWeights(CustomXSkinWeightsData& weightsData)
{
    std::map<DWORD, float> mergedWeights;
    for (std::size_t i = 0; i < weightsData.vertexIndices.size(); ++i)
    {
        if (weightsData.weights[i] <= 0.000001f)
        {
            continue;
        }

        mergedWeights[weightsData.vertexIndices[i]] += weightsData.weights[i];
    }

    weightsData.vertexIndices.clear();
    weightsData.weights.clear();
    for (const auto& mergedWeight : mergedWeights)
    {
        if (mergedWeight.second <= 0.000001f)
        {
            continue;
        }

        weightsData.vertexIndices.push_back(mergedWeight.first);
        weightsData.weights.push_back(mergedWeight.second);
    }
}

void CollapsePhysicsSkinWeights(CustomXMeshData& meshData,
                                const CustomXParseContext& context)
{
    if (meshData.skinWeights.empty())
    {
        return;
    }

    std::map<std::string, CustomXSkinWeightsData> mergedSkinWeights;
    DWORD collapsedBoneCount = 0;

    for (const CustomXSkinWeightsData& weightsData : meshData.skinWeights)
    {
        const std::string targetBoneName = FindSkinWeightCollapseTarget(weightsData.boneName, context);

        D3DXMATRIX targetOffsetMatrix;
        bool hasTargetOffsetMatrix = TryGetSkinWeightOffsetMatrix(meshData, targetBoneName, targetOffsetMatrix);
        if (!hasTargetOffsetMatrix)
        {
            hasTargetOffsetMatrix = TryComputeFrameOffsetMatrix(targetBoneName, context, targetOffsetMatrix);
        }
        if (!hasTargetOffsetMatrix)
        {
            targetOffsetMatrix = weightsData.offsetMatrix;
        }

        CustomXSkinWeightsData& mergedWeightsData = mergedSkinWeights[targetBoneName];
        if (mergedWeightsData.boneName.empty())
        {
            mergedWeightsData.boneName = targetBoneName;
            mergedWeightsData.offsetMatrix = targetOffsetMatrix;
        }

        if (targetBoneName != weightsData.boneName)
        {
            ++collapsedBoneCount;
        }

        for (std::size_t weightIndex = 0; weightIndex < weightsData.vertexIndices.size(); ++weightIndex)
        {
            if (weightsData.weights[weightIndex] <= 0.000001f)
            {
                continue;
            }

            mergedWeightsData.vertexIndices.push_back(weightsData.vertexIndices[weightIndex]);
            mergedWeightsData.weights.push_back(weightsData.weights[weightIndex]);
        }
    }

    if (collapsedBoneCount == 0)
    {
        return;
    }

    meshData.skinWeights.clear();
    for (auto& skinWeight : mergedSkinWeights)
    {
        NormalizeMergedSkinWeights(skinWeight.second);
        if (!skinWeight.second.vertexIndices.empty())
        {
            meshData.skinWeights.push_back(skinWeight.second);
        }
    }

}

bool SplitCustomXMeshDataByBoneLimit(const CustomXMeshData& sourceMeshData,
                                     const DWORD maxBonesPerPart,
                                     std::vector<CustomXMeshData>& outMeshParts)
{
    if (sourceMeshData.positions.empty() || sourceMeshData.faces.empty() ||
        sourceMeshData.skinWeights.empty())
    {
        return false;
    }

    std::vector<std::vector<DWORD>> vertexToBones(sourceMeshData.positions.size());
    for (DWORD boneIndex = 0; boneIndex < static_cast<DWORD>(sourceMeshData.skinWeights.size()); ++boneIndex)
    {
        const CustomXSkinWeightsData& sw = sourceMeshData.skinWeights[boneIndex];
        for (std::size_t weightIndex = 0; weightIndex < sw.vertexIndices.size(); ++weightIndex)
        {
            if (sw.weights[weightIndex] <= 0.000001f)
            {
                continue;
            }

            const DWORD vertexIndex = sw.vertexIndices[weightIndex];
            if (vertexIndex < sourceMeshData.positions.size())
            {
                vertexToBones[vertexIndex].push_back(boneIndex);
            }
        }
    }

    struct TriangleInfo
    {
        DWORD v0;
        DWORD v1;
        DWORD v2;
        DWORD materialIndex;
    };

    std::vector<TriangleInfo> triangles;
    for (std::size_t faceIndex = 0; faceIndex < sourceMeshData.faces.size(); ++faceIndex)
    {
        const std::vector<DWORD>& face = sourceMeshData.faces[faceIndex];
        if (face.size() < 3)
        {
            continue;
        }

        DWORD materialIndex = 0;
        if (faceIndex < sourceMeshData.faceMaterialIndices.size())
        {
            materialIndex = sourceMeshData.faceMaterialIndices[faceIndex];
        }

        for (std::size_t i = 1; i + 1 < face.size(); ++i)
        {
            TriangleInfo tri;
            tri.v0 = face[0];
            tri.v1 = face[i];
            tri.v2 = face[i + 1];
            tri.materialIndex = materialIndex;
            triangles.push_back(tri);
        }
    }

    std::vector<std::vector<TriangleInfo>> partTrianglesList;
    {
        std::vector<TriangleInfo> currentPartTriangles;
        std::set<DWORD> currentPartBones;
        std::size_t triangleIndex = 0;

        while (triangleIndex < triangles.size())
        {
            const TriangleInfo& tri = triangles[triangleIndex];
            std::set<DWORD> triBones;
            for (DWORD vertexIndex : {tri.v0, tri.v1, tri.v2})
            {
                for (DWORD boneIndex : vertexToBones[vertexIndex])
                {
                    triBones.insert(boneIndex);
                }
            }

            if (triBones.size() > maxBonesPerPart)
            {
                return false;
            }

            std::set<DWORD> mergedBones = currentPartBones;
            for (DWORD boneIndex : triBones)
            {
                mergedBones.insert(boneIndex);
            }

            if (mergedBones.size() <= maxBonesPerPart && !currentPartTriangles.empty())
            {
                currentPartBones = mergedBones;
                currentPartTriangles.push_back(tri);
                ++triangleIndex;
                continue;
            }

            if (!currentPartTriangles.empty())
            {
                partTrianglesList.push_back(currentPartTriangles);
                currentPartTriangles.clear();
                currentPartBones.clear();
            }

            currentPartBones = triBones;
            currentPartTriangles.push_back(tri);
            ++triangleIndex;
        }

        if (!currentPartTriangles.empty())
        {
            partTrianglesList.push_back(currentPartTriangles);
        }
    }

    for (std::size_t partIndex = 0; partIndex < partTrianglesList.size(); ++partIndex)
    {
        const std::vector<TriangleInfo>& partTriangles = partTrianglesList[partIndex];

        std::map<DWORD, DWORD> vertexRemap;
        std::vector<D3DXVECTOR3> partPositions;
        std::vector<D3DXVECTOR2> partTexCoords;

        for (const TriangleInfo& tri : partTriangles)
        {
            for (DWORD vertexIndex : {tri.v0, tri.v1, tri.v2})
            {
                if (vertexRemap.count(vertexIndex) == 0)
                {
                    DWORD localIndex = static_cast<DWORD>(partPositions.size());
                    vertexRemap[vertexIndex] = localIndex;
                    partPositions.push_back(sourceMeshData.positions[vertexIndex]);

                    if (vertexIndex < sourceMeshData.texCoords.size())
                    {
                        partTexCoords.push_back(sourceMeshData.texCoords[vertexIndex]);
                    }
                    else
                    {
                        partTexCoords.push_back(D3DXVECTOR2(0.0f, 0.0f));
                    }
                }
            }
        }

        std::set<DWORD> partBones;
        for (const TriangleInfo& tri : partTriangles)
        {
            for (DWORD vertexIndex : {tri.v0, tri.v1, tri.v2})
            {
                for (DWORD boneIndex : vertexToBones[vertexIndex])
                {
                    partBones.insert(boneIndex);
                }
            }
        }

        CustomXMeshData partMeshData;
        partMeshData.positions = partPositions;
        partMeshData.texCoords = partTexCoords;

        std::map<DWORD, DWORD> oldMaterialToLocalMaterial;
        for (const TriangleInfo& tri : partTriangles)
        {
            const DWORD oldMaterialIndex = tri.materialIndex;
            if (oldMaterialToLocalMaterial.count(oldMaterialIndex) == 0)
            {
                const DWORD localMaterialIndex =
                    static_cast<DWORD>(partMeshData.materials.size());
                oldMaterialToLocalMaterial[oldMaterialIndex] = localMaterialIndex;

                if (oldMaterialIndex < sourceMeshData.materials.size())
                {
                    partMeshData.materials.push_back(sourceMeshData.materials[oldMaterialIndex]);
                }
                else
                {
                    CustomXMaterialData fallbackMaterial;
                    ZeroMemory(&fallbackMaterial.material, sizeof(fallbackMaterial.material));
                    fallbackMaterial.material.Diffuse.r = 1.0f;
                    fallbackMaterial.material.Diffuse.g = 1.0f;
                    fallbackMaterial.material.Diffuse.b = 1.0f;
                    fallbackMaterial.material.Diffuse.a = 1.0f;
                    fallbackMaterial.material.Ambient = fallbackMaterial.material.Diffuse;
                    partMeshData.materials.push_back(fallbackMaterial);
                }
            }
        }

        for (const TriangleInfo& tri : partTriangles)
        {
            std::vector<DWORD> face;
            face.push_back(vertexRemap[tri.v0]);
            face.push_back(vertexRemap[tri.v1]);
            face.push_back(vertexRemap[tri.v2]);
            partMeshData.faces.push_back(face);
            partMeshData.faceMaterialIndices.push_back(oldMaterialToLocalMaterial[tri.materialIndex]);
        }

        std::map<DWORD, DWORD> oldBoneToLocalBone;
        DWORD localBoneIndex = 0;
        for (DWORD oldBoneIndex : partBones)
        {
            oldBoneToLocalBone[oldBoneIndex] = localBoneIndex;
            ++localBoneIndex;
        }

        for (DWORD oldBoneIndex : partBones)
        {
            const CustomXSkinWeightsData& sw = sourceMeshData.skinWeights[oldBoneIndex];
            CustomXSkinWeightsData localSw;
            localSw.boneName = sw.boneName;
            localSw.offsetMatrix = sw.offsetMatrix;

            for (std::size_t weightIndex = 0; weightIndex < sw.vertexIndices.size(); ++weightIndex)
            {
                if (sw.weights[weightIndex] <= 0.000001f)
                {
                    continue;
                }

                DWORD oldVertexIndex = sw.vertexIndices[weightIndex];
                if (vertexRemap.count(oldVertexIndex) > 0)
                {
                    localSw.vertexIndices.push_back(vertexRemap[oldVertexIndex]);
                    localSw.weights.push_back(sw.weights[weightIndex]);
                }
            }

            if (!localSw.vertexIndices.empty())
            {
                partMeshData.skinWeights.push_back(localSw);
            }
        }

        if (partMeshData.skinWeights.size() > maxBonesPerPart)
        {
            return false;
            return false;
        }

        outMeshParts.push_back(partMeshData);
    }

    return true;
}

bool ParseCustomXMesh(XTextTokenizer& tokenizer,
                      SkinAnimMeshFrame* frame,
                      SkinAnimMeshAlloc* allocator,
                      CustomXParseContext* context)
{
    std::string meshName = "CustomXMesh";
    std::string parsedMeshName;
    if (!ReadCustomXNameAndOpenBrace(tokenizer, parsedMeshName))
    {
        return false;
    }

    if (!parsedMeshName.empty())
    {
        meshName = parsedMeshName;
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

    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "}")
        {
            if (allocator != nullptr)
            {
                if (context != nullptr)
                {
                    CollapsePhysicsSkinWeights(meshData, *context);
                }

                if (meshData.skinWeights.size() <= MAX_SKININFO_BONES_PER_PART)
                {
                    LPD3DXMESHCONTAINER meshContainer = nullptr;
                    if (!CreateCustomXMeshContainer(meshName, meshData, allocator, &meshContainer))
                    {
                        return false;
                    }

                    AppendCustomXMeshContainer(frame, meshContainer);
                }
                else
                {
                    std::vector<CustomXMeshData> meshParts;
                    if (!SplitCustomXMeshDataByBoneLimit(meshData,
                                                         MAX_SKININFO_BONES_PER_PART,
                                                         meshParts))
                    {
                        return false;
                    }

                    for (std::size_t partIndex = 0; partIndex < meshParts.size(); ++partIndex)
                    {
                        LPD3DXMESHCONTAINER meshContainer = nullptr;
                        const std::string partMeshName =
                            meshName + "_part_" + std::to_string(partIndex);

                        if (!CreateCustomXMeshContainer(partMeshName,
                                                        meshParts[partIndex],
                                                        allocator,
                                                        &meshContainer))
                        {
                            return false;
                        }

                        AppendCustomXMeshContainer(frame, meshContainer);
                    }
                }
            }
            return true;
        }

        if (token == "MeshNormals")
        {
            if (!ParseCustomXMeshNormals(tokenizer, meshData))
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
            if (!ParseCustomXMeshMaterialList(tokenizer, meshData, context))
            {
                return false;
            }
            continue;
        }

        if (token == "SkinWeights")
        {
            if (!ParseCustomXSkinWeights(tokenizer, meshData, context))
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

bool ParseCustomXFrameBody(XTextTokenizer& tokenizer,
                           SkinAnimMeshFrame* frame,
                           SkinAnimMeshAlloc* allocator,
                           CustomXLoadPurpose loadPurpose,
                           CustomXParseContext* context);

SkinAnimMeshFrame* ParseCustomXFrame(XTextTokenizer& tokenizer,
                                     SkinAnimMeshAlloc* allocator,
                                     CustomXLoadPurpose loadPurpose,
                                     CustomXParseContext* context,
                                     const std::string& parentFrameName)
{
    std::string frameName;
    if (!ReadCustomXNameAndOpenBrace(tokenizer, frameName))
    {
        return nullptr;
    }

    SkinAnimMeshFrame* frame = CreateCustomXFrame(frameName);
    if (context != nullptr)
    {
        context->frameParents[frameName] = parentFrameName;
        context->frameLocalMatrices[frameName] = frame->TransformationMatrix;
    }

    if (!ParseCustomXFrameBody(tokenizer, frame, allocator, loadPurpose, context))
    {
        SAFE_DELETE_ARRAY(frame->Name);
        SAFE_DELETE(frame);
        return nullptr;
    }

    return frame;
}

bool ParseCustomXFrameBody(XTextTokenizer& tokenizer,
                           SkinAnimMeshFrame* frame,
                           SkinAnimMeshAlloc* allocator,
                           CustomXLoadPurpose loadPurpose,
                           CustomXParseContext* context)
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
            std::string parentFrameName;
            if (frame->Name != nullptr)
            {
                parentFrameName = frame->Name;
            }

            SkinAnimMeshFrame* child = ParseCustomXFrame(tokenizer,
                                                         allocator,
                                                         loadPurpose,
                                                         context,
                                                         parentFrameName);
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
            if (context != nullptr && frame->Name != nullptr)
            {
                context->frameLocalMatrices[frame->Name] = frame->TransformationMatrix;
            }
            continue;
        }

        if (token == "Mesh")
        {
            if (loadPurpose == CustomXLoadPurpose::AnimationOnly)
            {
                std::string meshToken;
                if (!tokenizer.ReadToken(meshToken))
                {
                    return false;
                }

                if (meshToken != "{")
                {
                    if (!ReadExpectedXToken(tokenizer, "{"))
                    {
                        return false;
                    }
                }

                if (!tokenizer.SkipObjectBodyFast())
                {
                    return false;
                }
                continue;
            }

            if (!ParseCustomXMesh(tokenizer, frame, allocator, context))
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

bool ParseCustomXAnimTicksPerSecond(XTextTokenizer& tokenizer, double& ticksPerSecond)
{
    if (!ReadExpectedXToken(tokenizer, "{"))
    {
        return false;
    }

    DWORD ticks = 0;
    if (!ReadXUIntToken(tokenizer, ticks))
    {
        return false;
    }

    if (ticks > 0)
    {
        ticksPerSecond = static_cast<double>(ticks);
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
                                                 LPD3DXANIMATIONCONTROLLER* outController,
                                                 const CustomXLoadOptions& options)
{
    if (outController == nullptr)
    {
        return E_POINTER;
    }

    *outController = nullptr;

    if (animationSets.empty() || frameRoot == nullptr)
    {
        CUSTOM_X_LOADER_LOG(L"BuildCtrl: skipping, animSets=" +
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

    CUSTOM_X_LOADER_LOG(L"BuildCtrl: uniqueAnimatedFrames=" +
                                std::to_wstring(animatedFrameNames.size()));

    if (animatedFrameNames.empty())
    {
        CUSTOM_X_LOADER_LOG(L"BuildCtrl: no animated frame names, skipping.");
        return S_OK;
    }

    const UINT maxOutputs = static_cast<UINT>(animatedFrameNames.size());
    const UINT maxSets = static_cast<UINT>(animationSets.size());
    const UINT maxTracks = 1;
    const UINT maxEvents = 16;

    LPD3DXANIMATIONCONTROLLER controller = nullptr;
    HRESULT hr = D3DXCreateAnimationController(maxOutputs, maxSets, maxTracks, maxEvents, &controller);
    CUSTOM_X_LOADER_LOG(L"BuildCtrl: D3DXCreateAnimationController HR=" + FormatHRESULT(hr) +
                                L" ptr=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(controller)) +
                                L" maxOutputs=" + std::to_wstring(maxOutputs) +
                                L" maxSets=" + std::to_wstring(maxSets) +
                                L" maxEvents=" + std::to_wstring(maxEvents));
    if (FAILED(hr) || controller == nullptr)
    {
        SAFE_RELEASE(controller);
        return E_FAIL;
    }

    CUSTOM_X_LOADER_LOG(L"BuildCtrl: controller created OK. Registering " +
                                std::to_wstring(maxOutputs) + L" outputs...");

    int registeredOutputs = 0;
    int failedOutputs = 0;
    for (const auto& name : animatedFrameNames)
    {
        LPD3DXFRAME rawFrame = D3DXFrameFind(frameRoot, name.c_str());
        if (rawFrame == nullptr)
        {
            ++failedOutputs;
            CUSTOM_X_LOADER_LOG(L"BuildCtrl: frame NOT FOUND for output '" +
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
            CUSTOM_X_LOADER_LOG(L"BuildCtrl: RegisterAnimationOutput FAILED for '" +
                                        AnsiTextToWideText(name) +
                                        L"'. HR=" + FormatHRESULT(hr));
            continue;
        }

        ++registeredOutputs;
    }

    CUSTOM_X_LOADER_LOG(L"BuildCtrl: output registration done. OK=" +
                                std::to_wstring(registeredOutputs) +
                                L" FAIL=" + std::to_wstring(failedOutputs));

    if (registeredOutputs == 0)
    {
        CUSTOM_X_LOADER_LOG(L"BuildCtrl: zero outputs registered, aborting.");
        SAFE_RELEASE(controller);
        return E_FAIL;
    }

    DWORD animSetIndex = 0;
    for (const auto& animSet : animationSets)
    {
        const DWORD numAnimations = static_cast<DWORD>(animSet.animations.size());
        CUSTOM_X_LOADER_LOG(L"BuildCtrl: creating animSet '" +
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
            CUSTOM_X_LOADER_LOG(L"BuildCtrl: D3DXCreateKeyframedAnimationSet FAILED. HR=" +
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
                        if (options.transposeAnimationMatrixKeys)
                        {
                            for (int row = 0; row < 3; ++row)
                            {
                                for (int col = row + 1; col < 3; ++col)
                                {
                                    const float temp = mat(row, col);
                                    mat(row, col) = mat(col, row);
                                    mat(col, row) = temp;
                                }
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

            std::sort(scaleKeys.begin(),
                      scaleKeys.end(),
                      [](const D3DXKEY_VECTOR3& a, const D3DXKEY_VECTOR3& b)
                      {
                          return a.Time < b.Time;
                      });
            std::sort(rotKeys.begin(),
                      rotKeys.end(),
                      [](const D3DXKEY_QUATERNION& a, const D3DXKEY_QUATERNION& b)
                      {
                          return a.Time < b.Time;
                      });
            std::sort(posKeys.begin(),
                      posKeys.end(),
                      [](const D3DXKEY_VECTOR3& a, const D3DXKEY_VECTOR3& b)
                      {
                          return a.Time < b.Time;
                      });

            for (std::size_t k = 0; k < rotKeys.size(); ++k)
            {
                D3DXQuaternionNormalize(&rotKeys[k].Value, &rotKeys[k].Value);
                if (k > 0)
                {
                    const FLOAT dot = D3DXQuaternionDot(&rotKeys[k - 1].Value, &rotKeys[k].Value);
                    if (dot < 0.0f)
                    {
                        rotKeys[k].Value.w = -rotKeys[k].Value.w;
                        rotKeys[k].Value.x = -rotKeys[k].Value.x;
                        rotKeys[k].Value.y = -rotKeys[k].Value.y;
                        rotKeys[k].Value.z = -rotKeys[k].Value.z;
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
                CUSTOM_X_LOADER_LOG(L"BuildCtrl: RegisterAnimationSRTKeys FAILED for '" +
                                            AnsiTextToWideText(anim.frameName) +
                                            L"' HR=" + FormatHRESULT(hr));
                anySrtKeyFailed = true;
            }
        }

        if (anySrtKeyFailed)
        {
            CUSTOM_X_LOADER_LOG(L"BuildCtrl: one or more RegisterAnimationSRTKeys failed, aborting animSet '" +
                                        AnsiTextToWideText(animSet.name) + L"'");
            SAFE_RELEASE(d3dxAnimSet);
            SAFE_RELEASE(controller);
            return E_FAIL;
        }

        hr = controller->RegisterAnimationSet(d3dxAnimSet);
        CUSTOM_X_LOADER_LOG(L"BuildCtrl: RegisterAnimationSet '" +
                                    AnsiTextToWideText(animSet.name) +
                                    L"' HR=" + FormatHRESULT(hr));
        if (FAILED(hr))
        {
            CUSTOM_X_LOADER_LOG(L"BuildCtrl: RegisterAnimationSet FAILED, aborting.");
            SAFE_RELEASE(d3dxAnimSet);
            SAFE_RELEASE(controller);
            return hr;
        }

        if (animSetIndex == 0)
        {
            hr = controller->SetTrackAnimationSet(0, d3dxAnimSet);
            CUSTOM_X_LOADER_LOG(L"BuildCtrl: SetTrackAnimationSet(0) HR=" + FormatHRESULT(hr));

            controller->SetTrackEnable(0, TRUE);
            controller->SetTrackWeight(0, 1.0f);
            controller->SetTrackSpeed(0, 1.0f);
            controller->SetTrackPosition(0, 0.0);

            CUSTOM_X_LOADER_LOG(L"BuildCtrl: Track 0 initialized with animSet '" +
                                        AnsiTextToWideText(animSet.name) + L"'");
        }

        SAFE_RELEASE(d3dxAnimSet);
        ++animSetIndex;
    }

    CUSTOM_X_LOADER_LOG(L"BuildCtrl: SUCCESS. controller=" +
                                std::to_wstring(reinterpret_cast<std::uintptr_t>(controller)) +
                                L" animSets=" + std::to_wstring(animSetIndex));
    *outController = controller;
    return S_OK;
}

HRESULT LoadCustomXFrameHierarchyFromText(const std::string& fileText,
                                          SkinAnimMeshAlloc* allocator,
                                          LPD3DXFRAME* frameRoot,
                                          std::vector<CustomXAnimationSet>* outAnimationSets,
                                          CustomXLoadPurpose loadPurpose,
                                          const CustomXLoadOptions& options)
{
    if (frameRoot == nullptr)
    {
        CUSTOM_X_LOADER_LOG(L"Custom parser failed: frameRoot output pointer is null.");
        return E_POINTER;
    }

    *frameRoot = nullptr;
    if (outAnimationSets != nullptr)
    {
        outAnimationSets->clear();
    }

    CUSTOM_X_LOADER_LOG(L"Custom parser start. Bytes=" + std::to_wstring(fileText.size()));

    XTextTokenizer tokenizer(fileText);
    CustomXParseContext parseContext;
    parseContext.options = options;
    std::string token;
    bool frameFound = false;
    bool syntheticRootCreated = false;
    double globalTicksPerSecond = 4800.0;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        if (token == "Frame")
        {
            CUSTOM_X_LOADER_LOG(L"Custom parser found top-level Frame.");
            SkinAnimMeshFrame* frame = ParseCustomXFrame(tokenizer,
                                                         allocator,
                                                         loadPurpose,
                                                         &parseContext,
                                                         std::string());
            if (frame == nullptr)
            {
                CUSTOM_X_LOADER_LOG(L"Custom parser failed while parsing top-level Frame.");
                return E_FAIL;
            }

            std::wstring frameName = L"(null)";
            if (frame->Name != nullptr)
            {
                frameName = AnsiTextToWideText(frame->Name);
            }
            CUSTOM_X_LOADER_LOG(L"Custom parser loaded top-level Frame: " + frameName);
            if (*frameRoot == nullptr)
            {
                *frameRoot = frame;
            }
            else
            {
                if (!syntheticRootCreated)
                {
                    SkinAnimMeshFrame* syntheticRoot = CreateCustomXFrame("");
                    AppendCustomXChildFrame(syntheticRoot,
                                            reinterpret_cast<SkinAnimMeshFrame*>(*frameRoot));
                    *frameRoot = syntheticRoot;
                    syntheticRootCreated = true;
                }

                AppendCustomXChildFrame(reinterpret_cast<SkinAnimMeshFrame*>(*frameRoot), frame);
            }
            frameFound = true;
            continue;
        }

        if (token == "AnimTicksPerSecond")
        {
            ParseCustomXAnimTicksPerSecond(tokenizer, globalTicksPerSecond);
            continue;
        }

        if (token == "Material")
        {
            std::string materialName;
            CustomXMaterialData materialData;
            if (!ParseCustomXNamedMaterial(tokenizer, materialName, materialData))
            {
                CUSTOM_X_LOADER_LOG(L"Custom parser failed while parsing top-level Material.");
                return E_FAIL;
            }

            if (!materialName.empty())
            {
                parseContext.namedMaterials[materialName] = materialData;
            }
            continue;
        }

        if (token == "AnimationSet" && outAnimationSets != nullptr)
        {
            CustomXAnimationSet animSet;
            animSet.ticksPerSecond = globalTicksPerSecond;
            if (ParseCustomXAnimationSet(tokenizer, animSet))
            {
                CUSTOM_X_LOADER_LOG(L"Custom parser loaded AnimationSet: " +
                                            AnsiTextToWideText(animSet.name));
                outAnimationSets->push_back(animSet);
            }
            else
            {
                CUSTOM_X_LOADER_LOG(L"Custom parser failed while parsing AnimationSet.");
            }
            continue;
        }

        if (token == "template")
        {
            if (!SkipCustomXObject(tokenizer))
            {
                CUSTOM_X_LOADER_LOG(L"Custom parser failed while skipping template.");
                return E_FAIL;
            }
            continue;
        }
    }

    if (!frameFound)
    {
        CUSTOM_X_LOADER_LOG(L"Custom parser failed: no Frame token was found.");
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

double GetCustomXMatrixMaxAbsElement(const D3DXMATRIX& matrix)
{
    double maxAbsValue = 0.0;
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            const double absValue = std::fabs(static_cast<double>(matrix(row, column)));
            if (absValue > maxAbsValue)
            {
                maxAbsValue = absValue;
            }
        }
    }

    return maxAbsValue;
}

double GetCustomXMatrixIdentityError(const D3DXMATRIX& matrix)
{
    double maxError = 0.0;
    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            double expected = 0.0;
            if (row == column)
            {
                expected = 1.0;
            }

            const double error = std::fabs(static_cast<double>(matrix(row, column)) - expected);
            if (error > maxError)
            {
                maxError = error;
            }
        }
    }

    return maxError;
}

void UpdateCustomXDiagnosticFrameMatrices(const LPD3DXFRAME frameBase,
                                          const LPD3DXMATRIX parentMatrix,
                                          std::map<std::string, D3DXMATRIX>& frameMatrices,
                                          double& maxAbsFrameCombined,
                                          const bool parentLocalOrder)
{
    if (frameBase == nullptr)
    {
        return;
    }

    const SkinAnimMeshFrame* frame = reinterpret_cast<const SkinAnimMeshFrame*>(frameBase);
    D3DXMATRIX combinedMatrix;
    if (parentMatrix != nullptr)
    {
        if (parentLocalOrder)
        {
            combinedMatrix = (*parentMatrix) * frame->TransformationMatrix;
        }
        else
        {
            combinedMatrix = frame->TransformationMatrix * (*parentMatrix);
        }
    }
    else
    {
        combinedMatrix = frame->TransformationMatrix;
    }

    if (frame->Name != nullptr)
    {
        frameMatrices[frame->Name] = combinedMatrix;
    }

    const double absValue = GetCustomXMatrixMaxAbsElement(combinedMatrix);
    if (absValue > maxAbsFrameCombined)
    {
        maxAbsFrameCombined = absValue;
    }

    if (frame->pFrameSibling != nullptr)
    {
        UpdateCustomXDiagnosticFrameMatrices(frame->pFrameSibling,
                                             parentMatrix,
                                             frameMatrices,
                                             maxAbsFrameCombined,
                                             parentLocalOrder);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        UpdateCustomXDiagnosticFrameMatrices(frame->pFrameFirstChild,
                                             &combinedMatrix,
                                             frameMatrices,
                                             maxAbsFrameCombined,
                                             parentLocalOrder);
    }
}

void DiagnoseCustomXMeshContainers(const LPD3DXFRAME frameBase,
                                   const std::map<std::string, D3DXMATRIX>& frameMatrices,
                                   const std::map<std::string, D3DXMATRIX>& parentLocalFrameMatrices,
                                   CustomXSkinningDiagnosticResult& result)
{
    if (frameBase == nullptr)
    {
        return;
    }

    const LPD3DXMESHCONTAINER containerBase = frameBase->pMeshContainer;
    LPD3DXMESHCONTAINER currentContainer = containerBase;
    while (currentContainer != nullptr)
    {
        ++result.meshContainerCount;
        const SkinAnimMeshContainer* container = reinterpret_cast<const SkinAnimMeshContainer*>(currentContainer);
        if (container->m_paletteSize > result.maxPaletteSize)
        {
            result.maxPaletteSize = container->m_paletteSize;
        }
        if (container->m_influenceCount > result.maxInfluenceCount)
        {
            result.maxInfluenceCount = container->m_influenceCount;
        }
        if (container->m_boneCount > result.maxBoneCount)
        {
            result.maxBoneCount = container->m_boneCount;
        }

        if (container->pSkinInfo != nullptr)
        {
            const DWORD boneCount = container->pSkinInfo->GetNumBones();
            for (DWORD boneIndex = 0; boneIndex < boneCount; ++boneIndex)
            {
                const D3DXMATRIX* offsetMatrix = container->pSkinInfo->GetBoneOffsetMatrix(boneIndex);
                if (boneIndex < container->m_boneOffsetMatrices.size())
                {
                    offsetMatrix = &container->m_boneOffsetMatrices[boneIndex];
                }

                if (offsetMatrix == nullptr)
                {
                    continue;
                }

                const double offsetAbsValue = GetCustomXMatrixMaxAbsElement(*offsetMatrix);
                if (offsetAbsValue > result.maxAbsBoneOffset)
                {
                    result.maxAbsBoneOffset = offsetAbsValue;
                }

                const char* boneName = container->pSkinInfo->GetBoneName(boneIndex);
                if (boneIndex < container->m_boneNames.size() && !container->m_boneNames.at(boneIndex).empty())
                {
                    boneName = container->m_boneNames.at(boneIndex).c_str();
                }

                if (boneName == nullptr)
                {
                    continue;
                }

                const auto frameMatrix = frameMatrices.find(boneName);
                if (frameMatrix == frameMatrices.end())
                {
                    continue;
                }

                const D3DXMATRIX bindPoseMatrix = (*offsetMatrix) * frameMatrix->second;
                const double bindPoseError = GetCustomXMatrixIdentityError(bindPoseMatrix);
                if (bindPoseError > result.maxBindPoseError)
                {
                    result.maxBindPoseError = bindPoseError;
                    result.maxBindPoseErrorBoneName = AnsiTextToWideText(boneName);
                    std::wstring detail = L"Diagnostic max bind pose matrices for ";
                    detail += result.maxBindPoseErrorBoneName;
                    detail += L"\nOffset=";
                    for (int row = 0; row < 4; ++row)
                    {
                        for (int column = 0; column < 4; ++column)
                        {
                            detail += std::to_wstring((*offsetMatrix)(row, column));
                            detail += L",";
                        }
                    }
                    detail += L"\nCombined=";
                    for (int row = 0; row < 4; ++row)
                    {
                        for (int column = 0; column < 4; ++column)
                        {
                            detail += std::to_wstring(frameMatrix->second(row, column));
                            detail += L",";
                        }
                    }
                    detail += L"\nBind=";
                    for (int row = 0; row < 4; ++row)
                    {
                        for (int column = 0; column < 4; ++column)
                        {
                            detail += std::to_wstring(bindPoseMatrix(row, column));
                            detail += L",";
                        }
                    }
                    result.message = detail;
                }

                const D3DXMATRIX combinedOffsetMatrix = frameMatrix->second * (*offsetMatrix);
                const double combinedOffsetError = GetCustomXMatrixIdentityError(combinedOffsetMatrix);
                if (combinedOffsetError > result.maxBindPoseErrorCombinedOffset)
                {
                    result.maxBindPoseErrorCombinedOffset = combinedOffsetError;
                }

                D3DXMATRIX transposedOffsetMatrix;
                D3DXMatrixTranspose(&transposedOffsetMatrix, offsetMatrix);
                const D3DXMATRIX transposedBindPoseMatrix = transposedOffsetMatrix * frameMatrix->second;
                const double transposedBindPoseError = GetCustomXMatrixIdentityError(transposedBindPoseMatrix);
                if (transposedBindPoseError > result.maxBindPoseErrorTransposedOffset)
                {
                    result.maxBindPoseErrorTransposedOffset = transposedBindPoseError;
                }

                const auto parentLocalFrameMatrix = parentLocalFrameMatrices.find(boneName);
                if (parentLocalFrameMatrix != parentLocalFrameMatrices.end())
                {
                    const D3DXMATRIX parentLocalBindPoseMatrix = (*offsetMatrix) * parentLocalFrameMatrix->second;
                    const double parentLocalBindPoseError =
                        GetCustomXMatrixIdentityError(parentLocalBindPoseMatrix);
                    if (parentLocalBindPoseError > result.maxBindPoseErrorParentLocalFrame)
                    {
                        result.maxBindPoseErrorParentLocalFrame = parentLocalBindPoseError;
                    }
                }
            }
        }

        currentContainer = currentContainer->pNextMeshContainer;
    }

    if (frameBase->pFrameSibling != nullptr)
    {
        DiagnoseCustomXMeshContainers(frameBase->pFrameSibling,
                                      frameMatrices,
                                      parentLocalFrameMatrices,
                                      result);
    }

    if (frameBase->pFrameFirstChild != nullptr)
    {
        DiagnoseCustomXMeshContainers(frameBase->pFrameFirstChild,
                                      frameMatrices,
                                      parentLocalFrameMatrices,
                                      result);
    }
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

CustomXSkinningDiagnosticResult DiagnoseCustomXSkinningForTest(const std::wstring& filePath,
                                                               const CustomXLoadOptions& options)
{
    CustomXSkinningDiagnosticResult result;

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
    result.hr = LoadCustomXFrameHierarchyFromText(fileText,
                                                  &allocator,
                                                  &frameRoot,
                                                  nullptr,
                                                  CustomXLoadPurpose::MeshAndAnimation,
                                                  options);
    if (FAILED(result.hr) || frameRoot == nullptr)
    {
        result.message = L"Parser failed. Bytes=" + std::to_wstring(fileText.size());
        DestroyCustomXFrameHierarchyWithAllocator(frameRoot, allocator);
        return result;
    }

    result.frameCount = CountCustomXFrames(frameRoot);
    std::map<std::string, D3DXMATRIX> frameMatrices;
    UpdateCustomXDiagnosticFrameMatrices(frameRoot,
                                         nullptr,
                                         frameMatrices,
                                         result.maxAbsFrameCombined,
                                         false);
    double unusedMaxAbsParentLocalFrameCombined = 0.0;
    std::map<std::string, D3DXMATRIX> parentLocalFrameMatrices;
    UpdateCustomXDiagnosticFrameMatrices(frameRoot,
                                         nullptr,
                                         parentLocalFrameMatrices,
                                         unusedMaxAbsParentLocalFrameCombined,
                                         true);
    DiagnoseCustomXMeshContainers(frameRoot, frameMatrices, parentLocalFrameMatrices, result);
    if (result.message.empty())
    {
        result.message = L"Diagnostic completed.";
    }

    DestroyCustomXFrameHierarchyWithAllocator(frameRoot, allocator);
    return result;
}

} // namespace NSRender
