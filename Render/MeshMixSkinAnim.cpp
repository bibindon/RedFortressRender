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
#include <string>

#include "Camera.h"
#include "Common.h"
#include "CustomXLoader.h"
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

} // anonymous namespace

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

    const double deltaTime = Common::ANIMATION_SPEED / D3DX64_ANIMATION_TIME_SCALE;
    clip.currentTime += deltaTime;

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

    clip.controller->SetTrackPosition(0, clip.currentTime);
    clip.controller->AdvanceTime(0.0, nullptr);
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
                                   &clip.controller,
                                   CustomXLoadPurpose::AnimationOnly);
    if (FAILED(hr) || clip.frameRoot == nullptr || clip.controller == nullptr)
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
                                           LPD3DXANIMATIONCONTROLLER* animationController,
                                           CustomXLoadPurpose loadPurpose)
{
    if (m_loadMode == MeshMixSkinAnimLoadMode::Custom)
    {
        WriteMeshMixSkinAnimLoadLog(L"LoadMeshHierarchy route=Custom Path=" + filePath);
        return LoadMeshHierarchyWithCustomLoader(filePath,
                                                 allocator,
                                                 frameRoot,
                                                 animationController,
                                                 loadPurpose);
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
                                                         LPD3DXANIMATIONCONTROLLER* animationController,
                                                         CustomXLoadPurpose loadPurpose)
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
    const HRESULT hr = LoadCustomXFrameHierarchyFromText(fileText, &allocator, frameRoot, &animationSets, loadPurpose);
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

    DWORD MAX_MATRICES = 16;
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
