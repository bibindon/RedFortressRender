#include "MeshMixSkinAnim.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>

#include "Camera.h"
#include "Common.h"
#include "Light.h"
#include "Util.h"

namespace NSRender
{
namespace
{
constexpr float X_MATERIAL_SPECULAR_INTENSITY_SCALE = 0.5f;
constexpr double D3DX64_ANIMATION_TIME_SCALE = 160.0;

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
    return clampedPower;
}

float ConvertXMaterialPowerToSpecularIntensity(const float materialPower)
{
    const float clampedPower = (std::max)(0.0f, (std::min)(materialPower, 255.0f));
    return (clampedPower / 255.0f) * X_MATERIAL_SPECULAR_INTENSITY_SCALE;
}

float PointLightShapeToShaderValue(const PointLightShape shape)
{
    return static_cast<float>(static_cast<int>(shape));
}

bool& GetSharedMirrorClipEnabled()
{
    static bool sharedMirrorClipEnabled = false;
    return sharedMirrorClipEnabled;
}

D3DXVECTOR4& GetSharedMirrorClipPlane()
{
    static D3DXVECTOR4 sharedMirrorClipPlane(0.0f, 1.0f, 0.0f, 0.0f);
    return sharedMirrorClipPlane;
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

std::wstring TrimText(const std::wstring& text)
{
    const std::wstring whitespace = L" \t\r\n";
    const std::size_t first = text.find_first_not_of(whitespace);
    if (first == std::wstring::npos)
    {
        return L"";
    }

    const std::size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

std::wstring UnquoteText(const std::wstring& text)
{
    const std::wstring trimmed = TrimText(text);
    if (trimmed.size() >= 2 && trimmed.front() == L'"' && trimmed.back() == L'"')
    {
        return trimmed.substr(1, trimmed.size() - 2);
    }

    return trimmed;
}

std::vector<std::wstring> SplitCsvLineText(const std::wstring& line)
{
    std::vector<std::wstring> fields;
    std::wstring currentField;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        const wchar_t ch = line[i];
        if (ch == L'"')
        {
            if (inQuotes && (i + 1) < line.size() && line[i + 1] == L'"')
            {
                currentField += L'"';
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
            continue;
        }

        if (ch == L',' && !inQuotes)
        {
            fields.push_back(currentField);
            currentField.clear();
            continue;
        }

        currentField += ch;
    }

    fields.push_back(currentField);
    return fields;
}

std::wstring GetResolvedPath(const std::wstring& path)
{
    if (PathIsRelative(path.c_str()))
    {
        return Util::GetExeDir() + path;
    }

    return path;
}

bool FileExists(const std::wstring& filePath)
{
    const DWORD attributes = GetFileAttributesW(filePath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }

    return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring GetCurrentDirectoryPath()
{
    wchar_t currentDirectory[MAX_PATH] { };
    const DWORD length = GetCurrentDirectoryW(_countof(currentDirectory), currentDirectory);
    if (length == 0 || length >= _countof(currentDirectory))
    {
        return L"";
    }

    std::wstring result(currentDirectory);
    if (!result.empty() && result.back() != L'\\' && result.back() != L'/')
    {
        result += L"\\";
    }

    return result;
}

std::wstring ResolveRuntimeFilePath(const std::wstring& fileName)
{
    const std::wstring exePath = Util::GetExeDir() + fileName;
    if (FileExists(exePath))
    {
        return exePath;
    }

    const std::wstring currentDirectory = GetCurrentDirectoryPath();
    if (!currentDirectory.empty())
    {
        const std::wstring currentDirectoryPath = currentDirectory + fileName;
        if (FileExists(currentDirectoryPath))
        {
            return currentDirectoryPath;
        }
    }

    return exePath;
}

std::wstring GetDirectoryPath(const std::wstring& path)
{
    const std::size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
    {
        return L"";
    }

    return path.substr(0, pos);
}

std::wstring GetPathWithoutExtension(const std::wstring& path)
{
    const std::size_t slashPos = path.find_last_of(L"\\/");
    const std::size_t dotPos = path.find_last_of(L'.');
    if (dotPos == std::wstring::npos ||
        (slashPos != std::wstring::npos && dotPos < slashPos))
    {
        return path;
    }

    return path.substr(0, dotPos);
}

std::wstring Utf8BytesToWideString(const std::string& bytes)
{
    if (bytes.empty())
    {
        return L"";
    }

    const char* data = bytes.data();
    int size = static_cast<int>(bytes.size());
    if (size >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xef &&
        static_cast<unsigned char>(data[1]) == 0xbb &&
        static_cast<unsigned char>(data[2]) == 0xbf)
    {
        data += 3;
        size -= 3;
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, data, size, nullptr, 0);
    if (required <= 0)
    {
        return L"";
    }

    std::wstring result(required, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data, size, &result[0], required);
    return result;
}

double GetAnimationControllerDuration(LPD3DXANIMATIONCONTROLLER controller)
{
    if (controller == nullptr || controller->GetMaxNumAnimationSets() == 0)
    {
        return 1.0;
    }

    LPD3DXANIMATIONSET animationSet = nullptr;
    if (FAILED(controller->GetAnimationSet(0, &animationSet)) || animationSet == nullptr)
    {
        return 1.0;
    }

    const double duration = (std::max)(animationSet->GetPeriod(), 0.0001);
    SAFE_RELEASE(animationSet);
    return duration;
}

void WriteMeshMixSkinAnimLoadLog(const std::wstring& message)
{
    const std::wstring line = L"[MeshMixSkinAnim] " + message + L"\r\n";
    OutputDebugStringW(line.c_str());

    std::wofstream file(Util::GetExeDir() + L"MeshMixSkinAnimLoad.log", std::ios::app);
    if (file)
    {
        file << line;
    }
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

struct CustomXAnimationKey
{
    DWORD keyType = 0;
    DWORD valueCount = 0;
    std::vector<double> times;
    std::vector<float> values;
};

struct CustomXAnimation
{
    std::string frameName;
    std::vector<CustomXAnimationKey> keys;
};

struct CustomXAnimationSet
{
    std::string name;
    double ticksPerSecond = 4800.0;
    std::vector<CustomXAnimation> animations;
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
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        char* endPtr = nullptr;
        value = std::strtof(token.c_str(), &endPtr);
        return endPtr != token.c_str();
    }

    return false;
}

bool ReadXUIntToken(XTextTokenizer& tokenizer, DWORD& value)
{
    std::string token;
    while (tokenizer.ReadToken(token))
    {
        if (IsXTextSeparatorToken(token))
        {
            continue;
        }

        char* endPtr = nullptr;
        const unsigned long parsed = std::strtoul(token.c_str(), &endPtr, 10);
        if (endPtr == token.c_str())
        {
            return false;
        }

        value = static_cast<DWORD>(parsed);
        return true;
    }

    return false;
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

    const DWORD fvf = D3DFVF_XYZ | D3DFVF_TEX1;
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

    std::vector<DWORD> adjacency(triangleCount * 3, 0xffffffff);
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

bool ParseCustomXFrameBody(XTextTokenizer& tokenizer, SkinAnimMeshFrame* frame, SkinAnimMeshAlloc* allocator);

SkinAnimMeshFrame* ParseCustomXFrame(XTextTokenizer& tokenizer, SkinAnimMeshAlloc* allocator)
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
    if (!ParseCustomXFrameBody(tokenizer, frame, allocator))
    {
        SAFE_DELETE_ARRAY(frame->Name);
        SAFE_DELETE(frame);
        return nullptr;
    }

    return frame;
}

bool ParseCustomXFrameBody(XTextTokenizer& tokenizer, SkinAnimMeshFrame* frame, SkinAnimMeshAlloc* allocator)
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
            SkinAnimMeshFrame* child = ParseCustomXFrame(tokenizer, allocator);
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
    std::string token;
    if (!tokenizer.ReadToken(token))
    {
        return false;
    }

    if (token == "{")
    {
        anim.frameName.clear();
    }
    else
    {
        anim.frameName = token;
        if (!ReadExpectedXToken(tokenizer, "{"))
        {
            return false;
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
            return true;
        }

        if (token == "AnimationKey")
        {
            CustomXAnimationKey key;
            if (!ParseCustomXAnimationKey(tokenizer, key))
            {
                return false;
            }
            anim.keys.push_back(key);
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
            CustomXAnimation anim;
            if (!ParseCustomXAnimation(tokenizer, anim))
            {
                return false;
            }
            animSet.animations.push_back(anim);
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

    const UINT maxOutputs = static_cast<UINT>(animatedFrameNames.size());
    if (maxOutputs == 0)
    {
        return S_OK;
    }

    const UINT maxSets = static_cast<UINT>(animationSets.size());
    const UINT maxTracks = maxSets;

    LPD3DXANIMATIONCONTROLLER controller = nullptr;
    HRESULT hr = D3DXCreateAnimationController(maxOutputs, maxSets, maxTracks, 0, &controller);
    if (FAILED(hr) || controller == nullptr)
    {
        return FAILED(hr) ? hr : E_FAIL;
    }

    std::map<std::string, SkinAnimMeshFrame*> frameMap;
    for (const auto& name : animatedFrameNames)
    {
        LPD3DXFRAME rawFrame = D3DXFrameFind(frameRoot, name.c_str());
        if (rawFrame == nullptr)
        {
            continue;
        }

        SkinAnimMeshFrame* skinFrame = reinterpret_cast<SkinAnimMeshFrame*>(rawFrame);
        hr = controller->RegisterAnimationOutput(name.c_str(),
                                                  &skinFrame->TransformationMatrix,
                                                  &skinFrame->m_animationScale,
                                                  &skinFrame->m_animationRotation,
                                                  &skinFrame->m_animationPosition);
        if (SUCCEEDED(hr))
        {
            frameMap[name] = skinFrame;
        }
    }

    for (const auto& animSet : animationSets)
    {
        const DWORD numAnimations = static_cast<DWORD>(animSet.animations.size());

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
            continue;
        }

        for (DWORD animIndex = 0; animIndex < numAnimations; ++animIndex)
        {
            const CustomXAnimation& anim = animSet.animations[animIndex];

            std::vector<D3DXKEY_QUATERNION> rotationKeys;
            std::vector<D3DXKEY_VECTOR3> scaleKeys;
            std::vector<D3DXKEY_VECTOR3> positionKeys;
            std::vector<D3DXKEY_VECTOR3> matrixKeysTempForScale;
            std::vector<D3DXKEY_VECTOR3> matrixKeysTempForPosition;
            std::vector<D3DXKEY_QUATERNION> matrixKeysTempForRotation;

            bool hasValidData = false;

            for (const auto& key : anim.keys)
            {
                for (std::size_t k = 0; k < key.times.size(); ++k)
                {
                    const double t = key.times[k];
                    const float* vals = &key.values[k * key.valueCount];

                    switch (key.keyType)
                    {
                        case 0:
                        {
                            D3DXKEY_QUATERNION qKey;
                            qKey.Time = static_cast<float>(t);
                            qKey.Value.w = vals[0];
                            qKey.Value.x = vals[1];
                            qKey.Value.y = vals[2];
                            qKey.Value.z = vals[3];
                            rotationKeys.push_back(qKey);
                            hasValidData = true;
                            break;
                        }
                        case 1:
                        {
                            D3DXKEY_VECTOR3 vKey;
                            vKey.Time = static_cast<float>(t);
                            vKey.Value.x = vals[0];
                            vKey.Value.y = vals[1];
                            vKey.Value.z = vals[2];
                            scaleKeys.push_back(vKey);
                            hasValidData = true;
                            break;
                        }
                        case 2:
                        {
                            D3DXKEY_VECTOR3 vKey;
                            vKey.Time = static_cast<float>(t);
                            vKey.Value.x = vals[0];
                            vKey.Value.y = vals[1];
                            vKey.Value.z = vals[2];
                            positionKeys.push_back(vKey);
                            hasValidData = true;
                            break;
                        }
                        case 4:
                        {
                            D3DXMATRIX mat;
                            for (int row = 0; row < 4; ++row)
                            {
                                for (int col = 0; col < 4; ++col)
                                {
                                    mat(row, col) = vals[row * 4 + col];
                                }
                            }

                            D3DXVECTOR3 scale;
                            D3DXQUATERNION rotation;
                            D3DXVECTOR3 position;
                            D3DXMatrixDecompose(&scale, &rotation, &position, &mat);

                            D3DXKEY_VECTOR3 sKey;
                            sKey.Time = static_cast<float>(t);
                            sKey.Value.x = scale.x;
                            sKey.Value.y = scale.y;
                            sKey.Value.z = scale.z;
                            matrixKeysTempForScale.push_back(sKey);

                            D3DXKEY_QUATERNION rKey;
                            rKey.Time = static_cast<float>(t);
                            rKey.Value.w = rotation.w;
                            rKey.Value.x = rotation.x;
                            rKey.Value.y = rotation.y;
                            rKey.Value.z = rotation.z;
                            matrixKeysTempForRotation.push_back(rKey);

                            D3DXKEY_VECTOR3 pKey;
                            pKey.Time = static_cast<float>(t);
                            pKey.Value.x = position.x;
                            pKey.Value.y = position.y;
                            pKey.Value.z = position.z;
                            matrixKeysTempForPosition.push_back(pKey);

                            hasValidData = true;
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            if (!hasValidData)
            {
                continue;
            }

            DWORD registeredAnimIndex = 0;
            if (!rotationKeys.empty() && !matrixKeysTempForRotation.empty())
            {
                d3dxAnimSet->RegisterAnimationSRTKeys(anim.frameName.c_str(),
                                                       static_cast<UINT>(matrixKeysTempForScale.size()),
                                                       static_cast<UINT>(matrixKeysTempForRotation.size()),
                                                       static_cast<UINT>(matrixKeysTempForPosition.size()),
                                                       &matrixKeysTempForScale[0],
                                                       &matrixKeysTempForRotation[0],
                                                       &matrixKeysTempForPosition[0],
                                                       &registeredAnimIndex);
            }
            else
            {
                d3dxAnimSet->RegisterAnimationSRTKeys(anim.frameName.c_str(),
                                                       static_cast<UINT>(scaleKeys.size()),
                                                       static_cast<UINT>(rotationKeys.size()),
                                                       static_cast<UINT>(positionKeys.size()),
                                                       scaleKeys.empty() ? nullptr : &scaleKeys[0],
                                                       rotationKeys.empty() ? nullptr : &rotationKeys[0],
                                                       positionKeys.empty() ? nullptr : &positionKeys[0],
                                                       &registeredAnimIndex);
            }
        }

        hr = controller->RegisterAnimationSet(d3dxAnimSet);
        SAFE_RELEASE(d3dxAnimSet);
    }

    *outController = controller;
    return S_OK;
}

HRESULT LoadCustomXFrameHierarchyFromText(const std::string& fileText,
                                          SkinAnimMeshAlloc* allocator,
                                          LPD3DXFRAME* frameRoot,
                                          std::vector<CustomXAnimationSet>* outAnimationSets = nullptr)
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
            SkinAnimMeshFrame* frame = ParseCustomXFrame(tokenizer, allocator);
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

void MeshMixSkinAnim::SetSharedMirrorClipPlane(const bool enabled, const D3DXVECTOR4& plane)
{
    GetSharedMirrorClipEnabled() = enabled;
    GetSharedMirrorClipPlane() = plane;
}

MeshMixSkinAnim::MeshMixSkinAnim(const std::wstring& filename,
                                 const D3DXVECTOR3& pos,
                                 const D3DXVECTOR3& rotate,
                                 const float scale,
                                 const stMeshParam& param,
                                 const AnimSetMap& animSetMap,
                                 const MeshMixSkinAnimLoadMode loadMode)
    : m_meshName(filename)
    , m_animationMeshName(filename)
    , m_allocator(filename)
    , m_animationAllocator(filename)
    , m_pos(pos)
    , m_rotate(rotate)
    , m_scale(scale)
    , m_loadMode(loadMode)
    , m_param(param)
    , m_animSetMap(animSetMap)
{
}

MeshMixSkinAnim::MeshMixSkinAnim(const std::wstring& meshFilename,
                                 const std::wstring& animationFilename,
                                 const D3DXVECTOR3& pos,
                                 const D3DXVECTOR3& rotate,
                                 const float scale,
                                 const stMeshParam& param,
                                 const AnimSetMap& animSetMap,
                                 const MeshMixSkinAnimLoadMode loadMode)
    : m_meshName(meshFilename)
    , m_animationMeshName(animationFilename)
    , m_allocator(meshFilename)
    , m_animationAllocator(animationFilename)
    , m_pos(pos)
    , m_rotate(rotate)
    , m_scale(scale)
    , m_loadMode(loadMode)
    , m_param(param)
    , m_animSetMap(animSetMap)
{
    m_useExternalAnimation = true;
}

MeshMixSkinAnim::~MeshMixSkinAnim()
{
    SAFE_RELEASE(m_D3DEffect);
    m_animController.Finalize();
    ReleaseAnimationClips();

    if (m_frameRoot != nullptr)
    {
        ReleaseMeshAllocatorRecursive(m_frameRoot, m_allocator);
        m_frameRoot = nullptr;
    }

    if (m_animationFrameRoot != nullptr)
    {
        ReleaseMeshAllocatorRecursive(m_animationFrameRoot, m_animationAllocator);
        m_animationFrameRoot = nullptr;
    }
}

void MeshMixSkinAnim::Initialize()
{
    HRESULT hr = E_FAIL;
    std::wstring tempPath = ResolveRuntimeFilePath(SHADER_FILENAME);

    hr = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                  tempPath.c_str(),
                                  nullptr,
                                  nullptr,
                                  0,
                                  nullptr,
                                  &m_D3DEffect,
                                  nullptr);
    if (FAILED(hr))
    {
        throw std::exception("Failed to create an effect file.");
    }

    LPD3DXANIMATIONCONTROLLER tempAnimController = nullptr;

    if (PathIsRelative(m_meshName.c_str()))
    {
        tempPath = Util::GetExeDir() + m_meshName;
    }
    else
    {
        tempPath = m_meshName;
    }

    hr = LoadMeshHierarchy(tempPath,
                           m_allocator,
                           &m_frameRoot,
                           &tempAnimController);
    WriteMeshMixSkinAnimLoadLog(L"Primary mesh load result. Path=" + tempPath +
                                L" HR=" + FormatHRESULT(hr) +
                                L" FrameRoot=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(m_frameRoot)) +
                                L" AnimController=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(tempAnimController)));
    if (FAILED(hr) || m_frameRoot == nullptr)
    {
        SAFE_RELEASE(tempAnimController);
        throw std::exception("Failed to load skin animation mesh.");
    }

    const bool loadedAnimationCsv = LoadAnimationCsv();

    if (loadedAnimationCsv)
    {
        SAFE_RELEASE(tempAnimController);
        m_useExternalAnimation = true;
    }
    else if (m_useExternalAnimation)
    {
        SAFE_RELEASE(tempAnimController);

        if (PathIsRelative(m_animationMeshName.c_str()))
        {
            tempPath = Util::GetExeDir() + m_animationMeshName;
        }
        else
        {
            tempPath = m_animationMeshName;
        }

        hr = LoadMeshHierarchy(tempPath,
                               m_animationAllocator,
                               &m_animationFrameRoot,
                               &tempAnimController);
        WriteMeshMixSkinAnimLoadLog(L"Split animation load result. Path=" + tempPath +
                                    L" HR=" + FormatHRESULT(hr) +
                                    L" FrameRoot=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(m_animationFrameRoot)) +
                                    L" AnimController=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(tempAnimController)));
        if (FAILED(hr) || m_animationFrameRoot == nullptr ||
            (tempAnimController == nullptr && m_loadMode != MeshMixSkinAnimLoadMode::Custom))
        {
            SAFE_RELEASE(tempAnimController);
            throw std::exception("Failed to load split animation mesh.");
        }

        ReleaseMeshContainersRecursive(m_animationFrameRoot, m_animationAllocator);
    }
    else if (tempAnimController == nullptr && m_loadMode != MeshMixSkinAnimLoadMode::Custom)
    {
        SAFE_RELEASE(tempAnimController);
        throw std::exception("Failed to load animation controller.");
    }

    if (m_animationClips.empty() && tempAnimController != nullptr)
    {
        m_animController.Init(tempAnimController, m_animSetMap);
        m_hasAnimationController = true;
    }
    else
    {
        SAFE_RELEASE(tempAnimController);
    }
    AllocateAllBoneMatrix(m_frameRoot);

    Common::AddDeviceLostResource(this);
    m_bLoaded = true;
}

void MeshMixSkinAnim::UpdateAnimation()
{
    if (!m_bLoaded || !m_enabled)
    {
        return;
    }

    if (!m_animationClips.empty())
    {
        UpdateActiveAnimationClip();
    }
    else
    {
        if (m_hasAnimationController)
        {
            m_animController.Update();
        }
    }

    if (!m_animationClips.empty())
    {
        const AnimationClip& clip = m_animationClips.at(m_activeAnimationClipIndex);
        ApplyAnimationFrameTransformsToMeshHierarchy(m_frameRoot, clip.frameRoot);
    }
    else if (m_useExternalAnimation)
    {
        ApplyAnimationFrameTransformsToMeshHierarchy(m_frameRoot, m_animationFrameRoot);
    }

    D3DXMATRIX worldMatrix = BuildWorldMatrix();
    UpdateFrameMatrix(m_frameRoot, &worldMatrix);
}

void MeshMixSkinAnim::Render()
{
    if (!m_bLoaded || !m_enabled)
    {
        return;
    }

    const D3DXVECTOR4 lightDir = Light::GetLightDir();
    const D3DXVECTOR4 lightColor = D3DXVECTOR4(Light::GetLightColor());
    const D3DXVECTOR4 ambientColor = D3DXVECTOR4(Light::GetAmbientColor());
    m_D3DEffect->SetVector("g_lightDir", &lightDir);
    m_D3DEffect->SetVector("g_lightColor", &lightColor);
    m_D3DEffect->SetVector("g_ambient", &ambientColor);
    m_D3DEffect->SetFloat("g_fSunLightIntensity", Light::GetBrightness());
    m_D3DEffect->SetFloat("g_fAmbientIntensity", Light::GetAmbientBrightness());

    const D3DXVECTOR4 cameraPos = D3DXVECTOR4(Camera::GetEyePos(), 1.0f);
    m_D3DEffect->SetVector("g_cameraPos", &cameraPos);
    BOOL useSaturateShadow = FALSE;
    if (m_param.saturateShadow)
    {
        useSaturateShadow = TRUE;
    }
    m_D3DEffect->SetBool("g_bSaturateShadow", useSaturateShadow);
    m_D3DEffect->SetBool("g_treatTextureAsWhite", m_param.treatTextureAsWhite ? TRUE : FALSE);
    m_D3DEffect->SetBool("g_alphaClipEnabled", m_alphaClipEnabled ? TRUE : FALSE);
    m_D3DEffect->SetBool("g_fresnelEnable", m_param.fresnel ? TRUE : FALSE);
    m_D3DEffect->SetFloat("g_fresnelIntensity", m_param.fresnelIntensity);
    m_D3DEffect->SetBool("g_mirrorClipEnable", GetSharedMirrorClipEnabled() ? TRUE : FALSE);
    const D3DXVECTOR4 mirrorClipPlane = GetSharedMirrorClipPlane();
    m_D3DEffect->SetVector("g_mirrorClipPlane", &mirrorClipPlane);
    m_D3DEffect->SetFloat("g_fSaturateShadowIntensity", m_param.saturateShadowIntensity);
    m_D3DEffect->SetFloat("g_fShadowDarkness", m_param.shadowDarkness);
    m_D3DEffect->SetFloat("g_specularIntensity", m_param.specularIntensity);

    if (m_param.pointLight)
    {
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

        m_D3DEffect->SetVectorArray("g_pointLightPos", pos, 16);
        m_D3DEffect->SetFloatArray("g_pointLightBrightness", brightness, 16);
        m_D3DEffect->SetFloatArray("g_pointLightShape", shape, 16);
        m_D3DEffect->SetFloatArray("g_pointLightLineLength", lineLength, 16);
        m_D3DEffect->SetFloatArray("g_pointLightSquareWidth", squareWidth, 16);
        m_D3DEffect->SetFloatArray("g_pointLightSquareHeight", squareHeight, 16);
        m_D3DEffect->SetVectorArray("g_pointLightRotation", rotation, 16);
        m_D3DEffect->SetVectorArray("g_pointLightColor", color, 16);
    }

    D3DXMATRIX viewProjectionMatrix = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_D3DEffect->SetMatrix("g_matViewProj", &viewProjectionMatrix);
    m_D3DEffect->SetTechnique(m_alphaClipEnabled ? "TechniqueAlphaClip" : "Technique1");
    RenderFrame(m_frameRoot);
}

D3DXMATRIX MeshMixSkinAnim::BuildWorldMatrix() const
{
    D3DXMATRIX worldMatrix;
    D3DXMatrixIdentity(&worldMatrix);
    {
        D3DXMATRIX mat;
        D3DXMatrixTranslation(&mat, -m_centerPos.x, -m_centerPos.y, -m_centerPos.z);
        worldMatrix *= mat;

        D3DXMatrixScaling(&mat, m_scale, m_scale, m_scale);
        worldMatrix *= mat;

        D3DXMatrixRotationYawPitchRoll(&mat, m_rotate.y, m_rotate.x, m_rotate.z);
        worldMatrix *= mat;

        D3DXMatrixTranslation(&mat, m_pos.x, m_pos.y, m_pos.z);
        worldMatrix *= mat;
    }

    return worldMatrix;
}

void MeshMixSkinAnim::ApplyAnimationFrameTransformsToMeshHierarchy(const LPD3DXFRAME meshFrameBase,
                                                                   const LPD3DXFRAME animationFrameRoot)
{
    if (meshFrameBase == nullptr || animationFrameRoot == nullptr)
    {
        return;
    }

    auto meshFrame = reinterpret_cast<SkinAnimMeshFrame*>(meshFrameBase);
    if (meshFrame->Name != nullptr)
    {
        LPD3DXFRAME animationFrameBase = D3DXFrameFind(animationFrameRoot, meshFrame->Name);
        if (animationFrameBase != nullptr)
        {
            auto animationFrame = reinterpret_cast<SkinAnimMeshFrame*>(animationFrameBase);
            meshFrame->TransformationMatrix = animationFrame->TransformationMatrix;
        }
    }

    if (meshFrame->pFrameSibling != nullptr)
    {
        ApplyAnimationFrameTransformsToMeshHierarchy(meshFrame->pFrameSibling, animationFrameRoot);
    }

    if (meshFrame->pFrameFirstChild != nullptr)
    {
        ApplyAnimationFrameTransformsToMeshHierarchy(meshFrame->pFrameFirstChild, animationFrameRoot);
    }
}

void MeshMixSkinAnim::UpdateActiveAnimationClip()
{
    if (m_activeAnimationClipIndex < 0 ||
        m_activeAnimationClipIndex >= static_cast<int>(m_animationClips.size()))
    {
        return;
    }

    AnimationClip& clip = m_animationClips.at(m_activeAnimationClipIndex);
    if (clip.controller == nullptr)
    {
        return;
    }

    clip.currentTime += Common::ANIMATION_SPEED / D3DX64_ANIMATION_TIME_SCALE;
    if (clip.currentTime >= clip.duration)
    {
        if (clip.stopWhenEnd)
        {
            clip.currentTime = clip.duration;
        }
        else
        {
            clip.currentTime = std::fmod(clip.currentTime, clip.duration);
        }
    }

    clip.controller->SetTrackPosition(0, 0.0);
    clip.controller->AdvanceTime(clip.currentTime, nullptr);
}

bool MeshMixSkinAnim::LoadAnimationCsv()
{
    const std::wstring meshPath = GetResolvedPath(m_meshName);
    const std::wstring csvPath = GetPathWithoutExtension(meshPath) + L".csv";
    if (!PathFileExists(csvPath.c_str()))
    {
        return false;
    }

    std::ifstream file(csvPath, std::ios::binary);
    if (!file)
    {
        return false;
    }

    const std::string bytes((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    const std::wstring csvText = Utf8BytesToWideString(bytes);
    const std::wstring csvDirectory = GetDirectoryPath(csvPath);

    std::vector<AnimationInfo> animationInfoList;
    std::wstring line;
    for (std::size_t i = 0; i <= csvText.size(); ++i)
    {
        if (i < csvText.size() && csvText[i] != L'\n')
        {
            line += csvText[i];
            continue;
        }

        const std::wstring trimmedLine = TrimText(line);
        line.clear();
        if (trimmedLine.empty())
        {
            continue;
        }

        const std::vector<std::wstring> fields = SplitCsvLineText(trimmedLine);
        if (fields.size() < 4 || TrimText(fields[0]) != L"Anim")
        {
            continue;
        }

        AnimationInfo info;
        info.name = UnquoteText(fields[1]);
        std::wstring filePath = UnquoteText(fields[2]);
        if (PathIsRelative(filePath.c_str()))
        {
            filePath = csvDirectory + L"\\" + filePath;
        }
        info.filePath = filePath;
        info.mode = UnquoteText(fields[3]);
        info.isDefault = (info.mode == L"default");

        if (!info.name.empty() && PathFileExists(info.filePath.c_str()))
        {
            animationInfoList.push_back(info);
        }
    }

    if (animationInfoList.empty())
    {
        return false;
    }

    int defaultIndex = 0;
    for (int i = 0; i < static_cast<int>(animationInfoList.size()); ++i)
    {
        if (!LoadAnimationClip(animationInfoList.at(i)))
        {
            continue;
        }

        if (animationInfoList.at(i).isDefault)
        {
            defaultIndex = static_cast<int>(m_animationClips.size()) - 1;
        }
    }

    if (m_animationClips.empty())
    {
        m_animationInfoList.clear();
        return false;
    }

    m_animationInfoList.clear();
    for (const auto& clip : m_animationClips)
    {
        m_animationInfoList.push_back(clip.info);
    }

    m_activeAnimationClipIndex = defaultIndex;
    return true;
}

bool MeshMixSkinAnim::LoadAnimationClip(const AnimationInfo& info)
{
    AnimationClip clip;
    clip.info = info;
    clip.loop = (info.mode == L"loop" || info.mode == L"default");
    clip.stopWhenEnd = (info.mode == L"stopWhenEnd");
    clip.allocator = NEW SkinAnimMeshAlloc(info.filePath);

    HRESULT hr = LoadMeshHierarchy(info.filePath,
                                   *clip.allocator,
                                   &clip.frameRoot,
                                   &clip.controller);
    const bool needsAnimationController = m_loadMode != MeshMixSkinAnimLoadMode::Custom;
    if (FAILED(hr) || clip.frameRoot == nullptr ||
        (needsAnimationController && clip.controller == nullptr))
    {
        SAFE_RELEASE(clip.controller);
        if (clip.frameRoot != nullptr)
        {
            ReleaseMeshAllocatorRecursive(clip.frameRoot, *clip.allocator);
            clip.frameRoot = nullptr;
        }
        SAFE_DELETE(clip.allocator);
        return false;
    }

    ReleaseMeshContainersRecursive(clip.frameRoot, *clip.allocator);
    if (clip.controller != nullptr)
    {
        clip.duration = GetAnimationControllerDuration(clip.controller);
    }
    m_animationClips.push_back(clip);
    return true;
}

HRESULT MeshMixSkinAnim::LoadMeshHierarchy(const std::wstring& filePath,
                                           SkinAnimMeshAlloc& allocator,
                                           LPD3DXFRAME* frameRoot,
                                           LPD3DXANIMATIONCONTROLLER* animationController)
{
    if (m_loadMode == MeshMixSkinAnimLoadMode::Custom)
    {
        WriteMeshMixSkinAnimLoadLog(L"LoadMeshHierarchy route=Custom Path=" + filePath);
        return LoadMeshHierarchyWithCustomLoader(filePath,
                                                 allocator,
                                                 frameRoot,
                                                 animationController);
    }

    WriteMeshMixSkinAnimLoadLog(L"LoadMeshHierarchy route=DirectX Path=" + filePath);
    return LoadMeshHierarchyWithDirectX(filePath,
                                        allocator,
                                        frameRoot,
                                        animationController);
}

HRESULT MeshMixSkinAnim::LoadMeshHierarchyWithDirectX(const std::wstring& filePath,
                                                      SkinAnimMeshAlloc& allocator,
                                                      LPD3DXFRAME* frameRoot,
                                                      LPD3DXANIMATIONCONTROLLER* animationController)
{
    return D3DXLoadMeshHierarchyFromX(filePath.c_str(),
                                      D3DXMESH_MANAGED | D3DXMESH_32BIT,
                                      Common::D3DDevice(),
                                      &allocator,
                                      nullptr,
                                      frameRoot,
                                      animationController);
}

HRESULT MeshMixSkinAnim::LoadMeshHierarchyWithCustomLoader(const std::wstring& filePath,
                                                        SkinAnimMeshAlloc& allocator,
                                                        LPD3DXFRAME* frameRoot,
                                                        LPD3DXANIMATIONCONTROLLER* animationController)
{
    if (animationController != nullptr)
    {
        *animationController = nullptr;
    }

    if (frameRoot == nullptr)
    {
        WriteMeshMixSkinAnimLoadLog(L"Custom loader failed: frameRoot output pointer is null.");
        return E_POINTER;
    }

    *frameRoot = nullptr;

    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        WriteMeshMixSkinAnimLoadLog(L"Custom loader failed: file open failed. Path=" + filePath);
        return E_FAIL;
    }

    const std::string fileText((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());

    std::vector<CustomXAnimationSet> animationSets;
    const HRESULT hr = LoadCustomXFrameHierarchyFromText(fileText, &allocator, frameRoot, &animationSets);
    WriteMeshMixSkinAnimLoadLog(L"Custom loader result. Path=" + filePath +
                                L" HR=" + FormatHRESULT(hr) +
                                L" FrameRoot=" + std::to_wstring(reinterpret_cast<std::uintptr_t>(*frameRoot)) +
                                L" AnimationSets=" + std::to_wstring(animationSets.size()));

    if (SUCCEEDED(hr) && animationController != nullptr && !animationSets.empty())
    {
        LPD3DXANIMATIONCONTROLLER controller = nullptr;
        const HRESULT controllerHr = CreateAnimationControllerFromParsedData(animationSets,
                                                                              *frameRoot,
                                                                              &controller);
        if (SUCCEEDED(controllerHr) && controller != nullptr)
        {
            *animationController = controller;
            WriteMeshMixSkinAnimLoadLog(L"Custom loader built animation controller. Sets=" +
                                        std::to_wstring(animationSets.size()));
        }
        else
        {
            WriteMeshMixSkinAnimLoadLog(L"Custom loader animation controller build failed. HR=" +
                                        FormatHRESULT(controllerHr));
        }
    }

    return hr;
}

void MeshMixSkinAnim::ReleaseAnimationClips()
{
    for (auto& clip : m_animationClips)
    {
        SAFE_RELEASE(clip.controller);
        if (clip.frameRoot != nullptr && clip.allocator != nullptr)
        {
            ReleaseMeshAllocatorRecursive(clip.frameRoot, *clip.allocator);
            clip.frameRoot = nullptr;
        }
        SAFE_DELETE(clip.allocator);
    }

    m_animationClips.clear();
    m_animationInfoList.clear();
    m_activeAnimationClipIndex = -1;
}

void MeshMixSkinAnim::UpdateFrameMatrix(const LPD3DXFRAME frameBase, const LPD3DXMATRIX matParent)
{
    auto frame = reinterpret_cast<SkinAnimMeshFrame*>(frameBase);

    if (matParent != nullptr)
    {
        frame->m_combinedMatrix = frame->TransformationMatrix * (*matParent);
    }
    else
    {
        frame->m_combinedMatrix = frame->TransformationMatrix;
    }

    if (frame->pFrameSibling != nullptr)
    {
        UpdateFrameMatrix(frame->pFrameSibling, matParent);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        UpdateFrameMatrix(frame->pFrameFirstChild, &frame->m_combinedMatrix);
    }
}

void MeshMixSkinAnim::RenderFrame(const LPD3DXFRAME frame)
{
    LPD3DXMESHCONTAINER container = frame->pMeshContainer;
    while (container != nullptr)
    {
        RenderMeshContainer(container);
        container = container->pNextMeshContainer;
    }

    if (frame->pFrameSibling != nullptr)
    {
        RenderFrame(frame->pFrameSibling);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        RenderFrame(frame->pFrameFirstChild);
    }
}

void MeshMixSkinAnim::RenderToEffect(LPD3DXEFFECT effect)
{
    if (!m_bLoaded || !m_enabled || effect == nullptr)
    {
        return;
    }

    RenderFrameToEffect(m_frameRoot, effect);
}

void MeshMixSkinAnim::RenderFrameToEffect(const LPD3DXFRAME frame, LPD3DXEFFECT effect)
{
    LPD3DXMESHCONTAINER container = frame->pMeshContainer;
    while (container != nullptr)
    {
        RenderMeshContainerToEffect(container, effect);
        container = container->pNextMeshContainer;
    }

    if (frame->pFrameSibling != nullptr)
    {
        RenderFrameToEffect(frame->pFrameSibling, effect);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        RenderFrameToEffect(frame->pFrameFirstChild, effect);
    }
}

void MeshMixSkinAnim::RenderMeshContainerToEffect(const LPD3DXMESHCONTAINER containerBase, LPD3DXEFFECT effect)
{
    auto container = reinterpret_cast<SkinAnimMeshContainer*>(containerBase);
    auto boneCombination = reinterpret_cast<LPD3DXBONECOMBINATION>(container->m_boneBuffer->GetBufferPointer());
    const DWORD paletteSize = container->m_paletteSize;

    effect->SetInt("g_currentBoneIndex", container->m_influenceCount - 1);

    const D3DXHANDLE skinAlphaCutoutHandle = effect->GetParameterByName(nullptr, "g_useSkinAlphaCutout");
    const D3DXHANDLE skinAlphaTextureHandle = effect->GetParameterByName(nullptr, "g_texSkinAlpha");
    const D3DXHANDLE meshAlphaCutoutHandle = effect->GetParameterByName(nullptr, "g_useMeshAlphaCutout");
    const D3DXHANDLE meshAlphaTextureHandle = effect->GetParameterByName(nullptr, "g_texMeshAlpha");

    effect->Begin(nullptr, 0);
    effect->BeginPass(0);

    for (DWORD i = 0; i < container->m_boneCount; ++i)
    {
        for (DWORD k = 0; k < paletteSize; ++k)
        {
            const DWORD boneId = boneCombination[i].BoneId[k];
            if (boneId == UINT_MAX)
            {
                continue;
            }

            m_matWorldArray[k] = container->m_boneOffsetMatrices[boneId] *
                                  (*container->m_frameCombinedMatrix[boneId]);
        }

        effect->SetMatrixArray("g_matWorldArray", &m_matWorldArray[0], paletteSize);
        const DWORD materialIndex = boneCombination[i].AttribId;
        const D3DMATERIAL9& material = container->pMaterials[materialIndex].MatD3D;
        const bool hasTexture = materialIndex < container->m_textureList.size() &&
                                container->m_textureList[materialIndex] != nullptr;
        bool useAlphaCutout = m_alphaClipEnabled;
        if (m_ignoreTransparentMaterial && material.Diffuse.a <= 0.001f)
        {
            useAlphaCutout = false;
        }

        BOOL useAlphaCutoutValue = FALSE;
        if (useAlphaCutout)
        {
            useAlphaCutoutValue = TRUE;
        }

        LPDIRECT3DTEXTURE9 alphaTexture = nullptr;
        if (useAlphaCutout && hasTexture)
        {
            alphaTexture = container->m_textureList[materialIndex];
        }

        if (skinAlphaCutoutHandle != nullptr)
        {
            effect->SetBool(skinAlphaCutoutHandle, useAlphaCutoutValue);
        }
        if (skinAlphaTextureHandle != nullptr)
        {
            effect->SetTexture(skinAlphaTextureHandle, alphaTexture);
        }
        if (meshAlphaCutoutHandle != nullptr)
        {
            effect->SetBool(meshAlphaCutoutHandle, useAlphaCutoutValue);
        }
        if (meshAlphaTextureHandle != nullptr)
        {
            effect->SetTexture(meshAlphaTextureHandle, alphaTexture);
        }
        effect->CommitChanges();
        container->MeshData.pMesh->DrawSubset(i);
    }

    effect->EndPass();
    effect->End();
}

void MeshMixSkinAnim::RenderMeshContainer(const LPD3DXMESHCONTAINER containerBase)
{
    auto container = reinterpret_cast<SkinAnimMeshContainer*>(containerBase);
    auto boneCombination = reinterpret_cast<LPD3DXBONECOMBINATION>(container->m_boneBuffer->GetBufferPointer());
    const DWORD paletteSize = container->m_paletteSize;

    for (DWORD i = 0; i < container->m_boneCount; ++i)
    {
        for (DWORD k = 0; k < paletteSize; ++k)
        {
            const DWORD boneId = boneCombination[i].BoneId[k];
            if (boneId == UINT_MAX)
            {
                continue;
            }

            m_matWorldArray[k] = container->m_boneOffsetMatrices[boneId] *
                                 (*container->m_frameCombinedMatrix[boneId]);
        }

        m_D3DEffect->SetMatrixArray("g_matWorldArray", &m_matWorldArray[0], paletteSize);

        const DWORD materialIndex = boneCombination[i].AttribId;
        const D3DMATERIAL9& material = container->pMaterials[materialIndex].MatD3D;
        const bool hasTexture = materialIndex < container->m_textureList.size() &&
                                container->m_textureList[materialIndex] != nullptr;
        const bool disableZWrite = !m_alphaClipEnabled && !m_ignoreTransparentMaterial && hasTexture && material.Diffuse.a <= 0.001f;
        const float diffuseAlpha = (hasTexture && material.Diffuse.a <= 0.001f)
                                 ? 1.0f
                                 : material.Diffuse.a;
        D3DXVECTOR4 diffuse(material.Diffuse.r,
                            material.Diffuse.g,
                            material.Diffuse.b,
                            diffuseAlpha);
        m_D3DEffect->SetVector("g_diffuse", &diffuse);

        float specularIntensity = GetMaterialSpecularIntensity(material);
        if (m_param.specularIntensityOverrideEnabled)
        {
            specularIntensity = m_param.specularIntensity;
        }
        m_D3DEffect->SetFloat("g_specularIntensity", specularIntensity);

        float specularPower = ConvertXMaterialPowerToShaderPower(material.Power);
        if (m_param.specularEdgeOverrideEnabled)
        {
            specularPower = ConvertSpecularEdgeToShaderPower(m_param.specularEdge);
        }
        m_D3DEffect->SetFloat("g_specularPower", specularPower);

        if (hasTexture)
        {
            m_D3DEffect->SetTexture("g_texture", container->m_textureList[materialIndex]);
        }
        else
        {
            m_D3DEffect->SetTexture("g_texture", nullptr);
        }

        const bool useAlphaDepthPrePass = !m_alphaClipEnabled && !m_ignoreTransparentMaterial && hasTexture;
        if (useAlphaDepthPrePass)
        {
            m_D3DEffect->SetTechnique("TechniqueAlphaDepthPrePass");
            m_D3DEffect->Begin(nullptr, 0);
            if (FAILED(m_D3DEffect->BeginPass(0)))
            {
                m_D3DEffect->End();
                throw std::exception("Failed 'BeginPass' function.");
            }

            m_D3DEffect->CommitChanges();
            container->MeshData.pMesh->DrawSubset(i);
            m_D3DEffect->EndPass();
            m_D3DEffect->End();
        }

        DWORD oldZWriteEnable = TRUE;
        if (disableZWrite || useAlphaDepthPrePass)
        {
            Common::D3DDevice()->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
            Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        }

        m_D3DEffect->SetTechnique(m_alphaClipEnabled ? "TechniqueAlphaClip" : "Technique1");
        m_D3DEffect->Begin(nullptr, 0);
        if (FAILED(m_D3DEffect->BeginPass(0)))
        {
            if (disableZWrite || useAlphaDepthPrePass)
            {
                Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
            }
            m_D3DEffect->End();
            throw std::exception("Failed 'BeginPass' function.");
        }

        m_D3DEffect->CommitChanges();
        container->MeshData.pMesh->DrawSubset(i);
        m_D3DEffect->EndPass();
        m_D3DEffect->End();

        if (!m_param.pointLight)
        {
            if (disableZWrite || useAlphaDepthPrePass)
            {
                Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
            }
            continue;
        }

        m_D3DEffect->Begin(nullptr, 0);
        if (FAILED(m_D3DEffect->BeginPass(1)))
        {
            if (disableZWrite || useAlphaDepthPrePass)
            {
                Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
            }
            m_D3DEffect->End();
            throw std::exception("Failed 'BeginPass' function.");
        }

        m_D3DEffect->CommitChanges();
        container->MeshData.pMesh->DrawSubset(i);
        m_D3DEffect->EndPass();
        m_D3DEffect->End();

        if (disableZWrite || useAlphaDepthPrePass)
        {
            Common::D3DDevice()->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
        }
    }
}

HRESULT MeshMixSkinAnim::AllocateBoneMatrix(LPD3DXMESHCONTAINER containerBase)
{
    SkinAnimMeshFrame *frame = nullptr;

    auto container = reinterpret_cast<SkinAnimMeshContainer*>(containerBase);

    DWORD boneCount = container->pSkinInfo->GetNumBones();
    container->m_frameCombinedMatrix.resize(boneCount);

    DWORD MAX_MATRICES = 8;
    if (boneCount > MAX_MATRICES)
    {
        m_matWorldArray.resize(MAX_MATRICES);
    }
    else
    {
        m_matWorldArray.resize(boneCount);
    }

    m_D3DEffect->SetInt("g_currentBoneIndex", container->m_influenceCount - 1);

    for (DWORD i = 0; i < boneCount; ++i)
    {
        LPD3DXFRAME p = D3DXFrameFind(m_frameRoot,
                                      container->pSkinInfo->GetBoneName(i));

        frame = reinterpret_cast<SkinAnimMeshFrame*>(p);
        if (frame == nullptr)
        {
            return E_FAIL;
        }

        LPD3DXMATRIX pMat = &frame->m_combinedMatrix;
        container->m_frameCombinedMatrix.at(i) = pMat;
    }

    return S_OK;
}

HRESULT MeshMixSkinAnim::AllocateAllBoneMatrix(LPD3DXFRAME frame)
{
    if (frame->pMeshContainer != nullptr && FAILED(AllocateBoneMatrix(frame->pMeshContainer)))
    {
        return E_FAIL;
    }

    if (frame->pFrameSibling != nullptr && FAILED(AllocateAllBoneMatrix(frame->pFrameSibling)))
    {
        return E_FAIL;
    }

    if (frame->pFrameFirstChild != nullptr && FAILED(AllocateAllBoneMatrix(frame->pFrameFirstChild)))
    {
        return E_FAIL;
    }

    return S_OK;
}

void MeshMixSkinAnim::ReleaseMeshAllocator(const LPD3DXFRAME frame)
{
    ReleaseMeshAllocatorRecursive(frame, m_allocator);
}

void MeshMixSkinAnim::ReleaseMeshContainersRecursive(const LPD3DXFRAME frame,
                                                     SkinAnimMeshAlloc& allocator)
{
    if (frame == nullptr)
    {
        return;
    }

    LPD3DXFRAME mutableFrame = frame;
    LPD3DXMESHCONTAINER container = mutableFrame->pMeshContainer;
    mutableFrame->pMeshContainer = nullptr;

    while (container != nullptr)
    {
        LPD3DXMESHCONTAINER nextContainer = container->pNextMeshContainer;
        container->pNextMeshContainer = nullptr;
        allocator.DestroyMeshContainer(container);
        container = nextContainer;
    }

    if (frame->pFrameSibling != nullptr)
    {
        ReleaseMeshContainersRecursive(frame->pFrameSibling, allocator);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        ReleaseMeshContainersRecursive(frame->pFrameFirstChild, allocator);
    }
}

void MeshMixSkinAnim::ReleaseMeshAllocatorRecursive(const LPD3DXFRAME frame, SkinAnimMeshAlloc& allocator)
{
    if (frame == nullptr)
    {
        return;
    }

    LPD3DXFRAME mutableFrame = frame;
    LPD3DXMESHCONTAINER container = mutableFrame->pMeshContainer;
    mutableFrame->pMeshContainer = nullptr;

    while (container != nullptr)
    {
        LPD3DXMESHCONTAINER nextContainer = container->pNextMeshContainer;
        container->pNextMeshContainer = nullptr;
        allocator.DestroyMeshContainer(container);
        container = nextContainer;
    }

    if (frame->pFrameSibling != nullptr)
    {
        ReleaseMeshAllocatorRecursive(frame->pFrameSibling, allocator);
    }

    if (frame->pFrameFirstChild != nullptr)
    {
        ReleaseMeshAllocatorRecursive(frame->pFrameFirstChild, allocator);
    }

    allocator.DestroyFrame(frame);
}

void MeshMixSkinAnim::SetPos(const D3DXVECTOR3& pos)
{
    m_pos = pos;
}

void MeshMixSkinAnim::SetSaturateShadow(const bool enabled)
{
    m_param.saturateShadow = enabled;
}

void MeshMixSkinAnim::SetSaturateShadowIntensity(const float intensity)
{
    m_param.saturateShadowIntensity = intensity;
}

void MeshMixSkinAnim::SetShadowDarkness(const float darkness)
{
    m_param.shadowDarkness = darkness;
}

void MeshMixSkinAnim::SetSpecularIntensity(const float intensity)
{
    m_param.specularIntensity = intensity;
}

void MeshMixSkinAnim::SetSpecularEdge(const float edge)
{
    m_param.specularEdge = edge;
}

void MeshMixSkinAnim::SetFresnelIntensity(const float intensity)
{
    m_param.fresnelIntensity = (std::max)(0.0f, intensity);
}

void MeshMixSkinAnim::SetSpecularIntensityOverrideEnabled(const bool enabled)
{
    m_param.specularIntensityOverrideEnabled = enabled;
}

void MeshMixSkinAnim::SetSpecularEdgeOverrideEnabled(const bool enabled)
{
    m_param.specularEdgeOverrideEnabled = enabled;
}

void MeshMixSkinAnim::SetTreatTextureAsWhite(const bool enabled)
{
    m_param.treatTextureAsWhite = enabled;
}

void MeshMixSkinAnim::SetAlphaClipEnabled(const bool enabled)
{
    m_alphaClipEnabled = enabled;
}

void MeshMixSkinAnim::SetIgnoreTransparentMaterial(const bool enabled)
{
    m_ignoreTransparentMaterial = enabled;
}

void MeshMixSkinAnim::SetRotY(const float rotY)
{
    m_rotate.y = rotY;
}

D3DXVECTOR3 MeshMixSkinAnim::GetRot() const
{
    return m_rotate;
}

D3DXVECTOR3 MeshMixSkinAnim::GetPos() const
{
    return m_pos;
}

float MeshMixSkinAnim::GetScale() const
{
    return m_scale;
}

bool MeshMixSkinAnim::IsEnabled() const
{
    return m_enabled;
}

void MeshMixSkinAnim::SetEnabled(const bool enabled)
{
    m_enabled = enabled;
}

std::wstring MeshMixSkinAnim::GetMeshName() const
{
    return m_meshName;
}

const std::vector<MeshMixSkinAnim::AnimationInfo>& MeshMixSkinAnim::GetAnimationInfoList() const
{
    return m_animationInfoList;
}

bool MeshMixSkinAnim::PlayAnimation(const std::wstring& name)
{
    for (int i = 0; i < static_cast<int>(m_animationClips.size()); ++i)
    {
        if (m_animationClips.at(i).info.name != name)
        {
            continue;
        }

        m_activeAnimationClipIndex = i;
        m_animationClips.at(i).currentTime = 0.0;
        if (m_animationClips.at(i).controller != nullptr)
        {
            m_animationClips.at(i).controller->SetTrackPosition(0, 0.0);
            m_animationClips.at(i).controller->AdvanceTime(0.0, nullptr);
        }
        return true;
    }

    return false;
}

void MeshMixSkinAnim::OnDeviceLost()
{
    const HRESULT hr = m_D3DEffect->OnLostDevice();
    assert(hr == S_OK);
}

void MeshMixSkinAnim::OnDeviceReset()
{
    const HRESULT hr = m_D3DEffect->OnResetDevice();
    assert(hr == S_OK);
}

}
