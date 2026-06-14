#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "winmm.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif

#include "Render.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <mmsystem.h>
#include <string>
#include <tchar.h>
#include <algorithm>
#include <cassert>
#include <crtdbg.h>
#include <cwctype>
#include <vector>
#include <Shlwapi.h>

#include "Common.h"
#pragma comment(lib, "Shlwapi.lib")

#include "MeshOld.h"
#include "AnimMesh.h"
#include "SkinAnimMesh.h"
#include "MeshMixSkinAnim.h"

#include "Camera.h"
#include "Light.h"

#include "Font.h"
#include "FontEx.h"
#include "LoadingScreen.h"
#include <chrono>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cwctype>
#include <utility>

namespace NSRender
{
namespace
{
constexpr std::chrono::duration<double> kTargetFrameDuration(1.0 / 60.0);

float ClampNormalizedTextPosition(const float value)
{
    return (std::max)(0.0f, (std::min)(value, 1.0f));
}

int NormalizedTextXToBaseX(const float x)
{
    return static_cast<int>(ClampNormalizedTextPosition(x) * static_cast<float>(Common::BASE_W));
}

int NormalizedTextYToBaseY(const float y)
{
    return static_cast<int>(ClampNormalizedTextPosition(y) * static_cast<float>(Common::BASE_H));
}

class CameraShakeFrameScope
{
public:
    CameraShakeFrameScope()
    {
        Camera::BeginShakeFrame();
    }

    ~CameraShakeFrameScope()
    {
        Camera::EndShakeFrame();
    }
};

bool TryParseBoolSetting(const std::wstring& value, bool& result)
{
    std::wstring normalized;
    normalized.reserve(value.size());
    for (wchar_t ch : value)
    {
        normalized.push_back(static_cast<wchar_t>(std::towlower(ch)));
    }

    if (normalized == L"1" || normalized == L"true" || normalized == L"on" || normalized == L"yes")
    {
        result = true;
        return true;
    }

    if (normalized == L"0" || normalized == L"false" || normalized == L"off" || normalized == L"no")
    {
        result = false;
        return true;
    }

    return false;
}
}

std::wstring Render::Trim(const std::wstring& text)
{
    const auto first = std::find_if_not(text.begin(), text.end(), [](wchar_t ch)
    {
        return std::iswspace(ch) != 0;
    });

    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](wchar_t ch)
    {
        return std::iswspace(ch) != 0;
    }).base();

    if (first >= last)
    {
        return L"";
    }

    return std::wstring(first, last);
}

int Render::NormalizeGaussianSampleSize(const int sampleSize)
{
    int normalized = (std::max)(1, (std::min)(sampleSize, 101));

    if ((normalized % 2) == 0)
    {
        --normalized;
    }

    return (std::max)(1, normalized);
}

int Render::NormalizeFontGaussianSampleSize(const int sampleSize)
{
    return (std::max)(1, (std::min)(sampleSize, 21));
}

int NormalizeFXAAQuality(const int quality)
{
    return (std::max)(1, (std::min)(quality, 8));
}

float HaltonSequence(unsigned int index, const unsigned int base)
{
    float result = 0.0f;
    float fraction = 1.0f / static_cast<float>(base);
    while (index > 0)
    {
        result += fraction * static_cast<float>(index % base);
        index /= base;
        fraction /= static_cast<float>(base);
    }
    return result;
}

int NormalizeMotionBlurCameraQuality(const int quality)
{
    return (std::max)(1, (std::min)(quality, 8));
}

float NormalizeMotionBlurCameraMaxBlurPixels(const float maxBlurPixels)
{
    return (std::max)(1.0f, (std::min)(maxBlurPixels, 64.0f));
}

int NormalizeMotionBlurCameraSampleCount(const int sampleCount)
{
    return (std::max)(2, (std::min)(sampleCount, 21));
}

float ClampUnitSetting(const float value)
{
    return (std::max)(0.0f, (std::min)(value, 1.0f));
}

std::wstring TrimCsvField(const std::wstring& text)
{
    const auto first = std::find_if_not(text.begin(), text.end(), [](wchar_t ch)
    {
        return std::iswspace(ch) != 0;
    });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](wchar_t ch)
    {
        return std::iswspace(ch) != 0;
    }).base();
    if (first >= last)
    {
        return L"";
    }
    return std::wstring(first, last);
}

std::wstring UnquoteCsvField(const std::wstring& text)
{
    if (text.size() >= 2 && text.front() == L'"' && text.back() == L'"')
    {
        return text.substr(1, text.size() - 2);
    }
    return text;
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

bool IsAbsoluteCsvPath(const std::wstring& path)
{
    if (path.size() >= 2 && path[1] == L':')
    {
        return true;
    }
    if (path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\')
    {
        return true;
    }
    return !path.empty() && (path[0] == L'\\' || path[0] == L'/');
}

std::wstring GetCsvParentDirectoryPath(const std::wstring& filePath)
{
    wchar_t fullPath[MAX_PATH] { };
    const DWORD length = GetFullPathNameW(filePath.c_str(), static_cast<DWORD>(_countof(fullPath)), fullPath, nullptr);
    if (length == 0 || length >= _countof(fullPath))
    {
        return L"";
    }
    std::wstring directoryPath = fullPath;
    const std::size_t slashPos = directoryPath.find_last_of(L"\\/");
    if (slashPos == std::wstring::npos)
    {
        return L"";
    }
    directoryPath.erase(slashPos);
    return directoryPath;
}

bool ResolveCsvFilePath(const std::wstring& csvDirectoryPath,
                        const std::wstring& sourcePath,
                        std::wstring& resolvedPath)
{
    std::wstring candidatePath = UnquoteCsvField(TrimCsvField(sourcePath));
    if (candidatePath.empty())
    {
        return false;
    }
    if (!IsAbsoluteCsvPath(candidatePath) && !csvDirectoryPath.empty())
    {
        candidatePath = csvDirectoryPath + L"\\" + candidatePath;
    }
    wchar_t fullPath[MAX_PATH] { };
    const DWORD length = GetFullPathNameW(candidatePath.c_str(), static_cast<DWORD>(_countof(fullPath)), fullPath, nullptr);
    if (length == 0 || length >= _countof(fullPath))
    {
        return false;
    }
    const DWORD attributes = GetFileAttributesW(fullPath);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        return false;
    }
    resolvedPath = fullPath;
    return true;
}

bool Render::LoadXFileListFromCsv(const std::wstring& csvPath,
                                  const float scale,
                                  int* loadedCount,
                                  int* skippedCount)
{
    if (loadedCount != nullptr)
    {
        *loadedCount = 0;
    }
    if (skippedCount != nullptr)
    {
        *skippedCount = 0;
    }
    if (csvPath.empty())
    {
        return false;
    }

    std::wifstream file(csvPath);
    if (!file)
    {
        return false;
    }

    const std::wstring csvDirectoryPath = GetCsvParentDirectoryPath(csvPath);
    int localLoadedCount = 0;
    int localSkippedCount = 0;
    std::wstring line;
    while (std::getline(file, line))
    {
        const std::wstring trimmedLine = TrimCsvField(line);
        if (trimmedLine.empty() || trimmedLine.front() == L'#')
        {
            continue;
        }

        const std::vector<std::wstring> fields = SplitCsvLineText(trimmedLine);
        if (fields.size() < 9)
        {
            ++localSkippedCount;
            continue;
        }

        std::wstring loadType = L"normal";
        if (fields.size() >= 10)
        {
            loadType = TrimCsvField(fields[9]);
            if (loadType != L"normal" && loadType != L"instancing" && loadType != L"skinanim")
            {
                loadType = L"normal";
            }
        }

        try
        {
            std::wstring resolvedPath;
            if (!ResolveCsvFilePath(csvDirectoryPath, fields[1], resolvedPath))
            {
                ++localSkippedCount;
                continue;
            }

            const D3DXVECTOR3 pos(std::stof(TrimCsvField(fields[2])),
                                  std::stof(TrimCsvField(fields[3])),
                                  std::stof(TrimCsvField(fields[4])));
            const D3DXVECTOR3 rot(D3DXToRadian(std::stof(TrimCsvField(fields[5]))),
                                  D3DXToRadian(std::stof(TrimCsvField(fields[6]))),
                                  D3DXToRadian(std::stof(TrimCsvField(fields[7]))));
            const float modelScale = std::stof(TrimCsvField(fields[8]));

            int renderId = -1;
            if (loadType == L"instancing")
            {
                renderId = AddMeshInstansing(resolvedPath, pos, rot, modelScale);
            }
            else if (loadType == L"skinanim")
            {
                AnimSetMap emptyAnimSetMap;
                renderId = AddMeshMixSkinAnim(resolvedPath, pos, rot, modelScale, emptyAnimSetMap);
            }
            else
            {
                renderId = AddMeshMix(resolvedPath, pos, rot, modelScale, 1.0f);
            }

            if (renderId < 0)
            {
                ++localSkippedCount;
                continue;
            }

            const int csvId = std::stoi(TrimCsvField(fields[0]));
            m_csvIdToRenderId[csvId] = renderId;

            ++localLoadedCount;
        }
        catch (...)
        {
            ++localSkippedCount;
        }
    }

    if (loadedCount != nullptr)
    {
        *loadedCount = localLoadedCount;
    }
    if (skippedCount != nullptr)
    {
        *skippedCount = localSkippedCount;
    }
    return localLoadedCount > 0;
}

bool Render::IsAllMeshLoaded() const
{
    for (const auto& mesh : m_meshMixList)
    {
        if (!mesh.IsLoaded())
        {
            return false;
        }
    }

    for (const auto* mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr && !mesh->IsLoaded())
        {
            return false;
        }
    }

    return true;
}

bool Render::LoadXFileListMoveFromCsv(const std::wstring& csvPath,
                                       int* loadedCount,
                                       int* skippedCount)
{
    if (loadedCount != nullptr)
    {
        *loadedCount = 0;
    }
    if (skippedCount != nullptr)
    {
        *skippedCount = 0;
    }
    if (csvPath.empty())
    {
        return false;
    }

    std::wifstream file(csvPath);
    if (!file)
    {
        return false;
    }

    int localLoadedCount = 0;
    int localSkippedCount = 0;
    std::wstring line;
    while (std::getline(file, line))
    {
        const std::wstring trimmedLine = TrimCsvField(line);
        if (trimmedLine.empty() || trimmedLine.front() == L'#')
        {
            continue;
        }

        const std::vector<std::wstring> fields = SplitCsvLineText(trimmedLine);
        if (fields.size() < 17)
        {
            ++localSkippedCount;
            continue;
        }

        try
        {
            const int renderId = std::stoi(TrimCsvField(fields[1]));
            const auto found = m_csvIdToRenderId.find(renderId);
            if (found == m_csvIdToRenderId.end())
            {
                ++localSkippedCount;
                continue;
            }

            MovingPlatform platform;
            platform.renderId = found->second;
            platform.csvId = renderId;
            platform.startPos = D3DXVECTOR3(std::stof(TrimCsvField(fields[10])),
                                            std::stof(TrimCsvField(fields[11])),
                                            std::stof(TrimCsvField(fields[12])));
            platform.endPos = D3DXVECTOR3(std::stof(TrimCsvField(fields[13])),
                                          std::stof(TrimCsvField(fields[14])),
                                          std::stof(TrimCsvField(fields[15])));
            platform.duration = std::stof(TrimCsvField(fields[16]));
            platform.elapsed = 0.0f;

            m_movingPlatforms.push_back(platform);
            ++localLoadedCount;
        }
        catch (...)
        {
            ++localSkippedCount;
        }
    }

    if (loadedCount != nullptr)
    {
        *loadedCount = localLoadedCount;
    }
    if (skippedCount != nullptr)
    {
        *skippedCount = localSkippedCount;
    }
    return localLoadedCount > 0;
}

void Render::UpdateMovingPlatforms(const float deltaSeconds)
{
    for (auto& platform : m_movingPlatforms)
    {
        platform.elapsed += deltaSeconds;
        if (platform.duration <= 0.0f)
        {
            continue;
        }

        const float t = platform.elapsed / platform.duration;
        const float pingPong = (std::sin(t * D3DX_PI * 2.0f) + 1.0f) * 0.5f;
        const D3DXVECTOR3 pos = platform.startPos + (platform.endPos - platform.startPos) * pingPong;

        SetMeshMixPos(platform.renderId, pos);
    }
}

void Render::ResetMovingPlatforms()
{
    for (auto& platform : m_movingPlatforms)
    {
        platform.elapsed = 0.0f;
    }
}

void Render::RegisterCsvIdMapping(const int csvId, const int renderId)
{
    m_csvIdToRenderId[csvId] = renderId;
}

const std::vector<Render::MovingPlatform>& Render::GetMovingPlatforms() const
{
    return m_movingPlatforms;
}

void Render::LoadSettingsCsv(const std::wstring& settingsCsvPath)
{
    m_settings.clear();

    if (settingsCsvPath.empty())
    {
        return;
    }

    std::wifstream file(settingsCsvPath);
    if (!file)
    {
        return;
    }

    std::wstring line;
    while (std::getline(file, line))
    {
        const std::size_t commentPos = line.find(L'#');
        if (commentPos != std::wstring::npos)
        {
            line = line.substr(0, commentPos);
        }

        const std::size_t commaPos = line.find(L',');
        if (commaPos == std::wstring::npos)
        {
            continue;
        }

        const std::wstring key = Trim(line.substr(0, commaPos));
        const std::wstring value = Trim(line.substr(commaPos + 1));

        if (!key.empty())
        {
            m_settings[key] = value;
        }
    }
}

void Render::ReloadSettingsCsv(const std::wstring& settingsCsvPath)
{
    // CSV の再読み込みも初期化時と同じ setter 経路を通して反映する。
    LoadSettingsCsv(settingsCsvPath);
    ApplySettings();
}

void Render::ApplySettings()
{
    const auto cameraNear = m_settings.find(L"CameraNear");
    const auto cameraFar = m_settings.find(L"CameraFar");
    const auto cameraHorizontalFov = m_settings.find(L"CameraHorizontalFov");
    auto tryReadFloatSetting = [this](const wchar_t* key, float& outValue)
    {
        const auto setting = m_settings.find(key);
        if (setting == m_settings.end())
        {
            return false;
        }

        try
        {
            outValue = std::stof(setting->second);
            return true;
        }
        catch (...)
        {
        }

        return false;
    };
    if (cameraHorizontalFov != m_settings.end())
    {
        try
        {
            SetCameraHorizontalFovDegrees(std::stof(cameraHorizontalFov->second));
        }
        catch (...)
        {
        }
    }

    if (cameraNear != m_settings.end() || cameraFar != m_settings.end())
    {
        float nearPlane = Camera::GetNear();
        float farPlane = Camera::GetFar();
        try
        {
            if (cameraNear != m_settings.end())
            {
                nearPlane = std::stof(cameraNear->second);
            }
            if (cameraFar != m_settings.end())
            {
                farPlane = std::stof(cameraFar->second);
            }
            SetCameraClipPlanes(nearPlane, farPlane);
        }
        catch (...)
        {
        }
    }

    D3DXVECTOR3 cameraPos = Camera::GetEyePos();
    D3DXVECTOR3 lookAtPos = Camera::GetLookAtPos();
    bool hasCameraPosSetting = false;
    bool hasLookAtPosSetting = false;
    hasCameraPosSetting = tryReadFloatSetting(L"CameraPosX", cameraPos.x) || hasCameraPosSetting;
    hasCameraPosSetting = tryReadFloatSetting(L"CameraPosY", cameraPos.y) || hasCameraPosSetting;
    hasCameraPosSetting = tryReadFloatSetting(L"CameraPosZ", cameraPos.z) || hasCameraPosSetting;
    hasLookAtPosSetting = tryReadFloatSetting(L"CameraLookAtX", lookAtPos.x) || hasLookAtPosSetting;
    hasLookAtPosSetting = tryReadFloatSetting(L"CameraLookAtY", lookAtPos.y) || hasLookAtPosSetting;
    hasLookAtPosSetting = tryReadFloatSetting(L"CameraLookAtZ", lookAtPos.z) || hasLookAtPosSetting;
    if (hasCameraPosSetting || hasLookAtPosSetting)
    {
        SetCamera(cameraPos, lookAtPos);
    }

    const auto gbufferNear = m_settings.find(L"GBufferNear");
    const auto gbufferFar = m_settings.find(L"GBufferFar");
    if (gbufferNear != m_settings.end() || gbufferFar != m_settings.end())
    {
        float nearPlane = 0.1f;
        float farPlane = 30'000.0f;
        try
        {
            if (gbufferNear != m_settings.end())
            {
                nearPlane = std::stof(gbufferNear->second);
            }
            if (gbufferFar != m_settings.end())
            {
                farPlane = std::stof(gbufferFar->second);
            }
            SetGBufferClipPlanes(nearPlane, farPlane);
        }
        catch (...)
        {
        }
    }

    const auto gbufferEnable = m_settings.find(L"GBufferEnable");
    if (gbufferEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(gbufferEnable->second, enabled))
        {
            SetGBufferEnable(enabled);
        }
    }

    const auto depthBufferShadowEnable = m_settings.find(L"DepthBufferShadowEnable");
    if (depthBufferShadowEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(depthBufferShadowEnable->second, enabled))
        {
            SetPostEffectDepthBufferShadow(enabled);
        }
    }

    const auto zShadowTexSize = m_settings.find(L"ZShadowTexSize");
    if (zShadowTexSize != m_settings.end())
    {
        if (zShadowTexSize->second == L"1/2")
        {
            SetPostEffectDepthBufferShadowTexSizeDivisor(2);
        }
        else if (zShadowTexSize->second == L"1/4")
        {
            SetPostEffectDepthBufferShadowTexSizeDivisor(4);
        }
        else if (zShadowTexSize->second == L"1/8")
        {
            SetPostEffectDepthBufferShadowTexSizeDivisor(8);
        }
        else if (zShadowTexSize->second == L"1/16")
        {
            SetPostEffectDepthBufferShadowTexSizeDivisor(16);
        }
        else
        {
            SetPostEffectDepthBufferShadowTexSizeDivisor(1);
        }
    }

    const auto ssaoEnable = m_settings.find(L"SSAOEnable");
    if (ssaoEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssaoEnable->second, enabled))
        {
            SetPostEffectSSAO(enabled);
        }
    }

    const auto ssaoBlurEnable = m_settings.find(L"SSAOBlurEnable");
    if (ssaoBlurEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssaoBlurEnable->second, enabled))
        {
            SetPostEffectSSAOBlur(enabled);
        }
    }

    const auto ssaoSeparableBlurEnable = m_settings.find(L"SSAOSeparableBlurEnable");
    if (ssaoSeparableBlurEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(ssaoSeparableBlurEnable->second, enabled))
        {
            SetPostEffectSSAOSeparableBlur(enabled);
        }
    }

    const auto ssaoDepthScaledSampleDistanceEnable = m_settings.find(L"SSAODepthScaledSampleDistanceEnable");
    if (ssaoDepthScaledSampleDistanceEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(ssaoDepthScaledSampleDistanceEnable->second, enabled))
        {
            SetPostEffectSSAODepthScaledSampleDistance(enabled);
        }
    }

    const auto ssaoRandomSamplingDirectionEnable = m_settings.find(L"SSAORandomSamplingDirectionEnable");
    if (ssaoRandomSamplingDirectionEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssaoRandomSamplingDirectionEnable->second, enabled))
        {
            SetPostEffectSSAORandomSamplingDirection(enabled);
        }
    }

    const auto ssaoTexSize = m_settings.find(L"SSAOTexSize");
    if (ssaoTexSize != m_settings.end())
    {
        if (ssaoTexSize->second == L"1/2")
        {
            SetPostEffectSSAOTexSizeDivisor(2);
        }
        else if (ssaoTexSize->second == L"1/4")
        {
            SetPostEffectSSAOTexSizeDivisor(4);
        }
        else
        {
            SetPostEffectSSAOTexSizeDivisor(1);
        }
    }

    const auto ssaoCompositeGaussian3x3Enable =
        m_settings.find(L"SSAOCompositeGaussian3x3Enable");
    if (ssaoCompositeGaussian3x3Enable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(ssaoCompositeGaussian3x3Enable->second, enabled))
        {
            SetPostEffectSSAOCompositeGaussian3x3(enabled);
        }
        else
        {
            SetPostEffectSSAOCompositeGaussian3x3(false);
        }
    }

    const auto ssaoMaxDarknessClampEnable =
        m_settings.find(L"SSAOMaxDarknessClampEnable");
    if (ssaoMaxDarknessClampEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssaoMaxDarknessClampEnable->second, enabled))
        {
            SetPostEffectSSAOMaxDarknessClamp(enabled);
        }
    }

    const auto ssgiEnable = m_settings.find(L"SSGIEnable");
    if (ssgiEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(ssgiEnable->second, enabled))
        {
            SetPostEffectSSGI(enabled);
        }
    }

    const auto ssgiBlurEnable = m_settings.find(L"SSGIBlurEnable");
    if (ssgiBlurEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssgiBlurEnable->second, enabled))
        {
            SetPostEffectSSGIBlur(enabled);
        }
    }

    const auto ssgiSeparableBlurEnable = m_settings.find(L"SSGISeparableBlurEnable");
    if (ssgiSeparableBlurEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssgiSeparableBlurEnable->second, enabled))
        {
            SetPostEffectSSGISeparableBlur(enabled);
        }
    }

    const auto ssgiDepthScaledSampleDistanceEnable =
        m_settings.find(L"SSGIDepthScaledSampleDistanceEnable");
    if (ssgiDepthScaledSampleDistanceEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(ssgiDepthScaledSampleDistanceEnable->second, enabled))
        {
            SetPostEffectSSGIDepthScaledSampleDistance(enabled);
        }
    }

    const auto ssgiUseThickness = m_settings.find(L"SSGIUseThickness");
    if (ssgiUseThickness != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(ssgiUseThickness->second, enabled))
        {
            SetPostEffectSSGIUseThickness(enabled);
        }
    }

    const auto ssgiTexSize = m_settings.find(L"SSGITexSize");
    if (ssgiTexSize != m_settings.end())
    {
        if (ssgiTexSize->second == L"1/2")
        {
            SetPostEffectSSGITexSizeDivisor(2);
        }
        else if (ssgiTexSize->second == L"1/4")
        {
            SetPostEffectSSGITexSizeDivisor(4);
        }
        else
        {
            SetPostEffectSSGITexSizeDivisor(1);
        }
    }

    const auto fogEnable = m_settings.find(L"FogEnable");
    if (fogEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(fogEnable->second, enabled))
        {
            SetPostEffectFog(enabled);
        }
    }

    const auto saturateEnable = m_settings.find(L"SaturateEnable");
    if (saturateEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(saturateEnable->second, enabled))
        {
            SetPostEffectSaturateEnable(enabled);
        }
    }

    const auto saturateLevel = m_settings.find(L"SaturateLevel");
    if (saturateLevel != m_settings.end())
    {
        try
        {
            SetPostEffectSaturate(std::stof(saturateLevel->second));
        }
        catch (...)
        {
        }
    }

    const auto gaussianEnable = m_settings.find(L"GaussianEnable");
    if (gaussianEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(gaussianEnable->second, enabled))
        {
            SetPostEffectGaussianFilter(enabled);
        }
    }

    const auto gaussianSampleSize = m_settings.find(L"GaussianSampleSize");
    if (gaussianSampleSize != m_settings.end())
    {
        try
        {
            SetPostEffectGaussianSampleSize(std::stoi(gaussianSampleSize->second));
        }
        catch (...)
        {
            SetPostEffectGaussianSampleSize(m_gaussianSampleSize);
        }
    }
    else
    {
        SetPostEffectGaussianSampleSize(m_gaussianSampleSize);
    }

    const auto gaussianStrength = m_settings.find(L"GaussianStrength");
    if (gaussianStrength != m_settings.end())
    {
        try
        {
            SetPostEffectGaussianStrength(std::stof(gaussianStrength->second));
        }
        catch (...)
        {
            SetPostEffectGaussianStrength(m_gaussianStrength);
        }
    }
    else
    {
        SetPostEffectGaussianStrength(m_gaussianStrength);
    }

    const auto maskedGaussianEnable = m_settings.find(L"MaskedGaussianEnable");
    if (maskedGaussianEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(maskedGaussianEnable->second, enabled))
        {
            SetPostEffectMaskedGaussianFilter(enabled);
        }
    }

    const auto maskedGaussianMaskPath = m_settings.find(L"MaskedGaussianMaskPath");
    if (maskedGaussianMaskPath != m_settings.end())
    {
        SetPostEffectMaskedGaussianMaskPath(maskedGaussianMaskPath->second);
    }

    const auto fxaaEnable = m_settings.find(L"FXAAEnable");
    if (fxaaEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(fxaaEnable->second, enabled))
        {
            SetPostEffectFXAA(enabled);
        }
    }

    const auto fxaaQuality = m_settings.find(L"FXAAQuality");
    if (fxaaQuality != m_settings.end())
    {
        try
        {
            SetPostEffectFXAAQuality(std::stoi(fxaaQuality->second));
        }
        catch (...)
        {
            SetPostEffectFXAAQuality(m_fxaaQuality);
        }
    }
    else
    {
        SetPostEffectFXAAQuality(m_fxaaQuality);
    }

    const auto taaEnable = m_settings.find(L"TAAEnable");
    if (taaEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(taaEnable->second, enabled))
        {
            SetPostEffectTAA(enabled);
        }
    }

    const auto taaHistoryWeight = m_settings.find(L"TAAHistoryWeight");
    if (taaHistoryWeight != m_settings.end())
    {
        try
        {
            SetPostEffectTAAHistoryWeight(std::stof(taaHistoryWeight->second));
        }
        catch (...)
        {
            SetPostEffectTAAHistoryWeight(m_taaHistoryWeight);
        }
    }
    else
    {
        SetPostEffectTAAHistoryWeight(m_taaHistoryWeight);
    }

    const auto postEffectAAEnable = m_settings.find(L"PostEffectAAEnable");
    if (postEffectAAEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(postEffectAAEnable->second, enabled))
        {
            SetPostEffectAA(enabled);
        }
    }

    const auto motionBlurCameraEnable = m_settings.find(L"MotionBlurCameraEnable");
    if (motionBlurCameraEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(motionBlurCameraEnable->second, enabled))
        {
            SetPostEffectMotionBlurCamera(enabled);
        }
    }

    const auto motionBlurCameraQuality = m_settings.find(L"MotionBlurCameraQuality");
    if (motionBlurCameraQuality != m_settings.end())
    {
        try
        {
            SetPostEffectMotionBlurCameraQuality(std::stoi(motionBlurCameraQuality->second));
        }
        catch (...)
        {
            SetPostEffectMotionBlurCameraQuality(m_motionBlurCameraQuality);
        }
    }
    else
    {
        SetPostEffectMotionBlurCameraQuality(m_motionBlurCameraQuality);
    }

    const auto motionBlurCameraMaxBlurPixels = m_settings.find(L"MotionBlurCameraMaxBlurPixels");
    if (motionBlurCameraMaxBlurPixels != m_settings.end())
    {
        try
        {
            SetPostEffectMotionBlurCameraMaxBlurPixels(std::stof(motionBlurCameraMaxBlurPixels->second));
        }
        catch (...)
        {
            SetPostEffectMotionBlurCameraMaxBlurPixels(m_motionBlurCameraMaxBlurPixels);
        }
    }

    const auto motionBlurCameraSampleCount = m_settings.find(L"MotionBlurCameraSampleCount");
    if (motionBlurCameraSampleCount != m_settings.end())
    {
        try
        {
            SetPostEffectMotionBlurCameraSampleCount(std::stoi(motionBlurCameraSampleCount->second));
        }
        catch (...)
        {
            SetPostEffectMotionBlurCameraSampleCount(m_motionBlurCameraSampleCount);
        }
    }

    const auto sunLightIntensity = m_settings.find(L"SunLightIntensity");
    if (sunLightIntensity != m_settings.end())
    {
        try
        {
            SetLightBrightness((std::max)(0.0f, std::stof(sunLightIntensity->second)));
        }
        catch (...)
        {
        }
    }

    D3DXCOLOR sunLightColor = GetLightColor();
    bool sunLightColorChanged = false;
    const auto sunLightColorR = m_settings.find(L"SunLightColorR");
    if (sunLightColorR != m_settings.end())
    {
        try
        {
            sunLightColor.r = ClampUnitSetting(std::stof(sunLightColorR->second));
            sunLightColorChanged = true;
        }
        catch (...)
        {
        }
    }
    const auto sunLightColorG = m_settings.find(L"SunLightColorG");
    if (sunLightColorG != m_settings.end())
    {
        try
        {
            sunLightColor.g = ClampUnitSetting(std::stof(sunLightColorG->second));
            sunLightColorChanged = true;
        }
        catch (...)
        {
        }
    }
    const auto sunLightColorB = m_settings.find(L"SunLightColorB");
    if (sunLightColorB != m_settings.end())
    {
        try
        {
            sunLightColor.b = ClampUnitSetting(std::stof(sunLightColorB->second));
            sunLightColorChanged = true;
        }
        catch (...)
        {
        }
    }
    if (sunLightColorChanged)
    {
        SetLightColor(sunLightColor);
    }

    const auto ambientLightIntensity = m_settings.find(L"AmbientLightIntensity");
    if (ambientLightIntensity != m_settings.end())
    {
        try
        {
            SetAmbientLightBrightness((std::max)(0.0f, std::stof(ambientLightIntensity->second)));
        }
        catch (...)
        {
        }
    }

    D3DXCOLOR ambientLightColor = GetAmbientLightColor();
    bool ambientLightColorChanged = false;
    const auto ambientLightColorR = m_settings.find(L"AmbientLightColorR");
    if (ambientLightColorR != m_settings.end())
    {
        try
        {
            ambientLightColor.r = ClampUnitSetting(std::stof(ambientLightColorR->second));
            ambientLightColorChanged = true;
        }
        catch (...)
        {
        }
    }
    const auto ambientLightColorG = m_settings.find(L"AmbientLightColorG");
    if (ambientLightColorG != m_settings.end())
    {
        try
        {
            ambientLightColor.g = ClampUnitSetting(std::stof(ambientLightColorG->second));
            ambientLightColorChanged = true;
        }
        catch (...)
        {
        }
    }
    const auto ambientLightColorB = m_settings.find(L"AmbientLightColorB");
    if (ambientLightColorB != m_settings.end())
    {
        try
        {
            ambientLightColor.b = ClampUnitSetting(std::stof(ambientLightColorB->second));
            ambientLightColorChanged = true;
        }
        catch (...)
        {
        }
    }
    if (ambientLightColorChanged)
    {
        SetAmbientLightColor(ambientLightColor);
    }

    const auto sssEnable = m_settings.find(L"SSSEnable");
    if (sssEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(sssEnable->second, enabled))
        {
            SetMeshMixSSS(enabled);
        }
    }

    const auto phongTreatTextureAsWhite = m_settings.find(L"PhongTreatTextureAsWhite");
    if (phongTreatTextureAsWhite != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(phongTreatTextureAsWhite->second, enabled))
        {
            SetPhongTreatTextureAsWhite(enabled);
        }
    }

    const auto sssIntensity = m_settings.find(L"SSSIntensity");
    if (sssIntensity != m_settings.end())
    {
        try
        {
            SetMeshMixSSSIntensity(std::stof(sssIntensity->second));
        }
        catch (...)
        {
            SetMeshMixSSSIntensity(m_meshMixSSSIntensity);
        }
    }

    float sssColorR = static_cast<float>((m_meshMixSSSColor >> 16) & 0xff) / 255.0f;
    float sssColorG = static_cast<float>((m_meshMixSSSColor >> 8) & 0xff) / 255.0f;
    float sssColorB = static_cast<float>(m_meshMixSSSColor & 0xff) / 255.0f;
    bool sssColorChanged = false;

    const auto sssColorRSetting = m_settings.find(L"SSSColorR");
    if (sssColorRSetting != m_settings.end())
    {
        try
        {
            sssColorR = ClampUnitSetting(std::stof(sssColorRSetting->second));
            sssColorChanged = true;
        }
        catch (...)
        {
        }
    }

    const auto sssColorGSetting = m_settings.find(L"SSSColorG");
    if (sssColorGSetting != m_settings.end())
    {
        try
        {
            sssColorG = ClampUnitSetting(std::stof(sssColorGSetting->second));
            sssColorChanged = true;
        }
        catch (...)
        {
        }
    }

    const auto sssColorBSetting = m_settings.find(L"SSSColorB");
    if (sssColorBSetting != m_settings.end())
    {
        try
        {
            sssColorB = ClampUnitSetting(std::stof(sssColorBSetting->second));
            sssColorChanged = true;
        }
        catch (...)
        {
        }
    }

    if (sssColorChanged)
    {
        const DWORD r = static_cast<DWORD>(sssColorR * 255.0f + 0.5f);
        const DWORD g = static_cast<DWORD>(sssColorG * 255.0f + 0.5f);
        const DWORD b = static_cast<DWORD>(sssColorB * 255.0f + 0.5f);
        SetMeshMixSSSColor((r << 16) | (g << 8) | b);
    }

    const auto specularIntensity = m_settings.find(L"SpecularIntensity");
    if (specularIntensity != m_settings.end())
    {
        try
        {
            SetMeshMixSpecularIntensity(std::stof(specularIntensity->second));
        }
        catch (...)
        {
        }
    }

    const auto specularIntensityOverride = m_settings.find(L"SpecularIntensityOverride");
    if (specularIntensityOverride != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(specularIntensityOverride->second, enabled))
        {
            SetMeshMixSpecularIntensityOverrideEnabled(enabled);
        }
    }

    const auto fresnelIntensity = m_settings.find(L"FresnelIntensity");
    if (fresnelIntensity != m_settings.end())
    {
        try
        {
            SetMeshMixFresnelIntensity(std::stof(fresnelIntensity->second));
        }
        catch (...)
        {
        }
    }

    const auto halfLambertShadowSaturation = m_settings.find(L"HalfLambertShadowSaturation");
    if (halfLambertShadowSaturation != m_settings.end())
    {
        try
        {
            const float value = (std::max)(0.0f, std::stof(halfLambertShadowSaturation->second));
            SetMeshMixSaturateShadow(value > 0.0f);
            SetMeshMixSaturateShadowIntensity(value);
        }
        catch (...)
        {
        }
    }

    const auto fogIntensity = m_settings.find(L"FogIntensity");
    if (fogIntensity != m_settings.end())
    {
        try
        {
            SetPostEffectFogIntensity(std::stof(fogIntensity->second));
        }
        catch (...)
        {
            SetPostEffectFogIntensity(2.0f);
        }
    }
    else
    {
        SetPostEffectFogIntensity(2.0f);
    }

    const auto fogHeightEnable = m_settings.find(L"FogHeightEnable");
    if (fogHeightEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(fogHeightEnable->second, enabled))
        {
            SetPostEffectFogHeightEnable(enabled);
        }
    }

    const auto fogHeightIntensity = m_settings.find(L"FogHeightIntensity");
    if (fogHeightIntensity != m_settings.end())
    {
        try
        {
            SetPostEffectFogHeightIntensity(std::stof(fogHeightIntensity->second));
        }
        catch (...)
        {
            SetPostEffectFogHeightIntensity(0.3f);
        }
    }
    else
    {
        SetPostEffectFogHeightIntensity(0.3f);
    }

    const auto fogHeightStart = m_settings.find(L"FogHeightStart");
    if (fogHeightStart != m_settings.end())
    {
        try
        {
            SetPostEffectFogHeightStart(std::stof(fogHeightStart->second));
        }
        catch (...)
        {
            SetPostEffectFogHeightStart(0.0f);
        }
    }
    else
    {
        SetPostEffectFogHeightStart(0.0f);
    }

    const auto fogHeightMax = m_settings.find(L"FogHeightMax");
    if (fogHeightMax != m_settings.end())
    {
        try
        {
            SetPostEffectHeightFogMax(std::stof(fogHeightMax->second));
        }
        catch (...)
        {
            SetPostEffectHeightFogMax(-5.0f);
        }
    }
    else
    {
        SetPostEffectHeightFogMax(-5.0f);
    }

    const auto fogHeightDistanceStart = m_settings.find(L"FogHeightDistanceStart");
    if (fogHeightDistanceStart != m_settings.end())
    {
        try
        {
            SetPostEffectHeightFogDistanceStart(std::stof(fogHeightDistanceStart->second));
        }
        catch (...)
        {
            SetPostEffectHeightFogDistanceStart(0.0f);
        }
    }
    else
    {
        SetPostEffectHeightFogDistanceStart(0.0f);
    }

    const auto fogHeightDistanceMax = m_settings.find(L"FogHeightDistanceMax");
    if (fogHeightDistanceMax != m_settings.end())
    {
        try
        {
            SetPostEffectHeightFogDistanceMax(std::stof(fogHeightDistanceMax->second));
        }
        catch (...)
        {
            SetPostEffectHeightFogDistanceMax(20.0f);
        }
    }
    else
    {
        SetPostEffectHeightFogDistanceMax(20.0f);
    }

    const auto shadowIntensity = m_settings.find(L"ShadowIntensity");
    if (shadowIntensity != m_settings.end())
    {
        try
        {
            SetPostEffectDepthBufferShadowIntensity(std::stof(shadowIntensity->second));
        }
        catch (...)
        {
            SetPostEffectDepthBufferShadowIntensity(0.5f);
        }
    }
    else
    {
        SetPostEffectDepthBufferShadowIntensity(0.5f);
    }

    const auto ssaoShadowStrength = m_settings.find(L"SSAOShadowStrength");
    if (ssaoShadowStrength != m_settings.end())
    {
        try
        {
            SetPostEffectSSAOShadowStrength(std::stof(ssaoShadowStrength->second));
        }
        catch (...)
        {
            SetPostEffectSSAOShadowStrength(1.0f);
        }
    }
    else
    {
        SetPostEffectSSAOShadowStrength(1.0f);
    }

    const auto ssaoShadowSaturationBoost = m_settings.find(L"SSAOShadowSaturationBoost");
    if (ssaoShadowSaturationBoost != m_settings.end())
    {
        try
        {
            SetPostEffectSSAOSaturationBoost(std::stof(ssaoShadowSaturationBoost->second));
        }
        catch (...)
        {
            SetPostEffectSSAOSaturationBoost(0.30f);
        }
    }
    else
    {
        SetPostEffectSSAOSaturationBoost(0.30f);
    }

    const auto ssaoSampleCount = m_settings.find(L"SSAOSampleCount");
    if (ssaoSampleCount != m_settings.end())
    {
        try
        {
            SetPostEffectSSAOSampleCount(std::stoi(ssaoSampleCount->second));
        }
        catch (...)
        {
            SetPostEffectSSAOSampleCount(16);
        }
    }
    else
    {
        SetPostEffectSSAOSampleCount(16);
    }

    const auto shadowSaturationBoost = m_settings.find(L"ShadowSaturationBoost");
    if (shadowSaturationBoost != m_settings.end())
    {
        try
        {
            SetPostEffectDepthBufferShadowSaturationBoost(std::stof(shadowSaturationBoost->second));
        }
        catch (...)
        {
            SetPostEffectDepthBufferShadowSaturationBoost(0.35f);
        }
    }
    else
    {
        SetPostEffectDepthBufferShadowSaturationBoost(0.35f);
    }

    const auto shadowCoverage = m_settings.find(L"ShadowCoverage");
    if (shadowCoverage != m_settings.end())
    {
        try
        {
            SetPostEffectDepthBufferShadowCoverage(std::stof(shadowCoverage->second));
        }
        catch (...)
        {
            SetPostEffectDepthBufferShadowCoverage(0.5f);
        }
    }
    else
    {
        SetPostEffectDepthBufferShadowCoverage(0.5f);
    }

    const auto shadowPcfTapCount = m_settings.find(L"ShadowPcfTapCount");
    if (shadowPcfTapCount != m_settings.end())
    {
        try
        {
            SetPostEffectDepthBufferShadowPcfTapCount(std::stoi(shadowPcfTapCount->second));
        }
        catch (...)
        {
            SetPostEffectDepthBufferShadowPcfTapCount(11);
        }
    }
    else
    {
        SetPostEffectDepthBufferShadowPcfTapCount(11);
    }

    const auto shadowCompositeTapCount = m_settings.find(L"ShadowCompositeTapCount");
    if (shadowCompositeTapCount != m_settings.end())
    {
        try
        {
            SetPostEffectDepthBufferShadowCompositeTapCount(std::stoi(shadowCompositeTapCount->second));
        }
        catch (...)
        {
            SetPostEffectDepthBufferShadowCompositeTapCount(11);
        }
    }
    else
    {
        SetPostEffectDepthBufferShadowCompositeTapCount(11);
    }

    const auto ssaoSampleRadius = m_settings.find(L"SSAOSampleRadius");
    if (ssaoSampleRadius != m_settings.end())
    {
        try
        {
            SetPostEffectSSAOSampleRadius(std::stof(ssaoSampleRadius->second));
        }
        catch (...)
        {
            SetPostEffectSSAOSampleRadius(4.0f);
        }
    }
    else
    {
        SetPostEffectSSAOSampleRadius(4.0f);
    }

    const auto ssgiSampleRadius = m_settings.find(L"SSGISampleRadius");
    if (ssgiSampleRadius != m_settings.end())
    {
        try
        {
            SetPostEffectSSGISampleRadius(std::stof(ssgiSampleRadius->second));
        }
        catch (...)
        {
            SetPostEffectSSGISampleRadius(1.0f);
        }
    }
    else
    {
        SetPostEffectSSGISampleRadius(1.0f);
    }

    const auto ssaoBlurKernelSize = m_settings.find(L"SSAOBlurKernelSize");
    if (ssaoBlurKernelSize != m_settings.end())
    {
        try
        {
            SetPostEffectSSAOBlurKernelSize(std::stoi(ssaoBlurKernelSize->second));
        }
        catch (...)
        {
            SetPostEffectSSAOBlurKernelSize(21);
        }
    }
    else
    {
        SetPostEffectSSAOBlurKernelSize(21);
    }

    const auto ssgiBlurKernelSize = m_settings.find(L"SSGIBlurKernelSize");
    if (ssgiBlurKernelSize != m_settings.end())
    {
        try
        {
            SetPostEffectSSGIBlurKernelSize(std::stoi(ssgiBlurKernelSize->second));
        }
        catch (...)
        {
            SetPostEffectSSGIBlurKernelSize(21);
        }
    }
    else
    {
        SetPostEffectSSGIBlurKernelSize(21);
    }

    const auto ssgiSampleCount = m_settings.find(L"SSGISampleCount");
    if (ssgiSampleCount != m_settings.end())
    {
        try
        {
            SetPostEffectSSGISampleCount(std::stoi(ssgiSampleCount->second));
        }
        catch (...)
        {
            SetPostEffectSSGISampleCount(16);
        }
    }
    else
    {
        SetPostEffectSSGISampleCount(16);
    }

    const auto ssgiIndirectLightStrength = m_settings.find(L"SSGIIndirectLightStrength");
    if (ssgiIndirectLightStrength != m_settings.end())
    {
        try
        {
            SetPostEffectSSGIIndirectLightStrength(std::stof(ssgiIndirectLightStrength->second));
        }
        catch (...)
        {
            SetPostEffectSSGIIndirectLightStrength(1.0f);
        }
    }
    else
    {
        SetPostEffectSSGIIndirectLightStrength(1.0f);
    }

    const auto ssgiIndirectLightMaxContribution =
        m_settings.find(L"SSGIIndirectLightMaxContribution");
    if (ssgiIndirectLightMaxContribution != m_settings.end())
    {
        try
        {
            SetPostEffectSSGIIndirectLightMaxContribution(std::stof(ssgiIndirectLightMaxContribution->second));
        }
        catch (...)
        {
            SetPostEffectSSGIIndirectLightMaxContribution(1.0f);
        }
    }
    else
    {
        SetPostEffectSSGIIndirectLightMaxContribution(1.0f);
    }

    const auto bloomEnable = m_settings.find(L"BloomEnable");
    if (bloomEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(bloomEnable->second, enabled))
        {
            SetPostEffectBloom(enabled);
        }
    }

    const auto bloomThreshold = m_settings.find(L"BloomThreshold");
    if (bloomThreshold != m_settings.end())
    {
        try
        {
            SetPostEffectBloomThreshold(std::stof(bloomThreshold->second));
        }
        catch (...)
        {
            SetPostEffectBloomThreshold(2.5f);
        }
    }
    else
    {
        SetPostEffectBloomThreshold(2.5f);
    }

    const auto bloomWeightSum = m_settings.find(L"BloomWeightSum");
    if (bloomWeightSum != m_settings.end())
    {
        try
        {
            SetPostEffectBloomWeightSum(std::stof(bloomWeightSum->second));
        }
        catch (...)
        {
            SetPostEffectBloomWeightSum(1.0f);
        }
    }
    else
    {
        SetPostEffectBloomWeightSum(1.0f);
    }

    const auto haloEnable = m_settings.find(L"HaloEnable");
    if (haloEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(haloEnable->second, enabled))
        {
            SetPostEffectHalo(enabled);
        }
    }

    const auto haloThreshold = m_settings.find(L"HaloThreshold");
    if (haloThreshold != m_settings.end())
    {
        try
        {
            SetPostEffectHaloThreshold(std::stof(haloThreshold->second));
        }
        catch (...)
        {
            SetPostEffectHaloThreshold(2.5f);
        }
    }
    else
    {
        SetPostEffectHaloThreshold(2.5f);
    }

    const auto depthOfFieldMode = m_settings.find(L"DepthOfFieldMode");
    if (depthOfFieldMode != m_settings.end())
    {
        try
        {
            const int modeValue = std::stoi(depthOfFieldMode->second);
            if (modeValue <= 0)
            {
                SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
            }
            else if (modeValue == 1)
            {
                SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Enabled);
            }
            else
            {
                SetPostEffectDepthOfFieldMode(DepthOfFieldMode::AutoNear);
            }
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
        }
    }
    else
    {
        const auto depthOfFieldEnable = m_settings.find(L"DepthOfFieldEnable");
        if (depthOfFieldEnable != m_settings.end())
        {
            bool enabled = false;
            if (TryParseBoolSetting(depthOfFieldEnable->second, enabled))
            {
                SetPostEffectDepthOfField(enabled);
            }
        }
    }

    const auto depthOfFieldFocalDistance = m_settings.find(L"DepthOfFieldFocalDistance");
    if (depthOfFieldFocalDistance != m_settings.end())
    {
        try
        {
            SetPostEffectDepthOfFieldFocalDistance(std::stof(depthOfFieldFocalDistance->second));
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldFocalDistance(8.0f);
        }
    }
    else
    {
        SetPostEffectDepthOfFieldFocalDistance(8.0f);
    }

    const auto depthOfFieldStartNear = m_settings.find(L"DepthOfFieldStartNear");
    if (depthOfFieldStartNear != m_settings.end())
    {
        try
        {
            SetPostEffectDepthOfFieldStartNear(std::stof(depthOfFieldStartNear->second));
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldStartNear(0.0f);
        }
    }
    else
    {
        SetPostEffectDepthOfFieldStartNear(0.0f);
    }

    const auto depthOfFieldMaxBlurDistance = m_settings.find(L"DepthOfFieldMaxBlurDistance");
    if (depthOfFieldMaxBlurDistance != m_settings.end())
    {
        try
        {
            SetPostEffectDepthOfFieldMaxBlurDistance(std::stof(depthOfFieldMaxBlurDistance->second));
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldMaxBlurDistance(16.0f);
        }
    }
    else
    {
        SetPostEffectDepthOfFieldMaxBlurDistance(16.0f);
    }

    const auto depthOfFieldAutoActivationDistance = m_settings.find(L"DepthOfFieldAutoActivationDistance");
    if (depthOfFieldAutoActivationDistance != m_settings.end())
    {
        try
        {
            SetPostEffectDepthOfFieldAutoActivationDistance(std::stof(depthOfFieldAutoActivationDistance->second));
        }
        catch (...)
        {
            SetPostEffectDepthOfFieldAutoActivationDistance(10.0f);
        }
    }
    else
    {
        SetPostEffectDepthOfFieldAutoActivationDistance(10.0f);
    }

    const auto starBurstEnable = m_settings.find(L"StarBurstEnable");
    if (starBurstEnable != m_settings.end())
    {
        bool enabled = true;
        if (TryParseBoolSetting(starBurstEnable->second, enabled))
        {
            SetPostEffectStarBurst(enabled);
        }
    }

    const auto starBurstThreshold = m_settings.find(L"StarBurstThreshold");
    if (starBurstThreshold != m_settings.end())
    {
        try
        {
            SetPostEffectStarBurstThreshold(std::stof(starBurstThreshold->second));
        }
        catch (...)
        {
            SetPostEffectStarBurstThreshold(2.8f);
        }
    }
    else
    {
        SetPostEffectStarBurstThreshold(2.8f);
    }

    const auto starBurstDistanceFade = m_settings.find(L"StarBurstDistanceFade");
    if (starBurstDistanceFade != m_settings.end())
    {
        try
        {
            SetPostEffectStarBurstDistanceFade(std::stof(starBurstDistanceFade->second));
        }
        catch (...)
        {
            SetPostEffectStarBurstDistanceFade(0.0f);
        }
    }
    else
    {
        SetPostEffectStarBurstDistanceFade(0.0f);
    }

    const auto godRayEnable = m_settings.find(L"GodRayEnable");
    if (godRayEnable != m_settings.end())
    {
        bool enabled = false;
        if (TryParseBoolSetting(godRayEnable->second, enabled))
        {
            SetPostEffectGodRay(enabled);
        }
    }

    {
        D3DXVECTOR3 godRayLightPos = GetPostEffectGodRayLightPos();
        bool godRayLightPosChanged = false;
        const auto godRayLightPosX = m_settings.find(L"GodRayLightPosX");
        if (godRayLightPosX != m_settings.end())
        {
            try { godRayLightPos.x = std::stof(godRayLightPosX->second); godRayLightPosChanged = true; }
            catch (...) { }
        }
        const auto godRayLightPosY = m_settings.find(L"GodRayLightPosY");
        if (godRayLightPosY != m_settings.end())
        {
            try { godRayLightPos.y = std::stof(godRayLightPosY->second); godRayLightPosChanged = true; }
            catch (...) { }
        }
        const auto godRayLightPosZ = m_settings.find(L"GodRayLightPosZ");
        if (godRayLightPosZ != m_settings.end())
        {
            try { godRayLightPos.z = std::stof(godRayLightPosZ->second); godRayLightPosChanged = true; }
            catch (...) { }
        }
        if (godRayLightPosChanged)
        {
            SetPostEffectGodRayLightPos(godRayLightPos);
        }
    }

    {
        D3DXVECTOR3 godRayLightColor = GetPostEffectGodRayLightColor();
        bool godRayLightColorChanged = false;
        const auto godRayLightColorR = m_settings.find(L"GodRayLightColorR");
        if (godRayLightColorR != m_settings.end())
        {
            try { godRayLightColor.x = ClampUnitSetting(std::stof(godRayLightColorR->second)); godRayLightColorChanged = true; }
            catch (...) { }
        }
        const auto godRayLightColorG = m_settings.find(L"GodRayLightColorG");
        if (godRayLightColorG != m_settings.end())
        {
            try { godRayLightColor.y = ClampUnitSetting(std::stof(godRayLightColorG->second)); godRayLightColorChanged = true; }
            catch (...) { }
        }
        const auto godRayLightColorB = m_settings.find(L"GodRayLightColorB");
        if (godRayLightColorB != m_settings.end())
        {
            try { godRayLightColor.z = ClampUnitSetting(std::stof(godRayLightColorB->second)); godRayLightColorChanged = true; }
            catch (...) { }
        }
        if (godRayLightColorChanged)
        {
            SetPostEffectGodRayLightColor(godRayLightColor);
        }
    }

    const auto godRayIntensity = m_settings.find(L"GodRayIntensity");
    if (godRayIntensity != m_settings.end())
    {
        try
        {
            SetPostEffectGodRayIntensity(std::stof(godRayIntensity->second));
        }
        catch (...)
        {
        }
    }

    const auto godRayVirtualProximityStrength = m_settings.find(L"GodRayVirtualProximityStrength");
    if (godRayVirtualProximityStrength != m_settings.end())
    {
        try
        {
            SetPostEffectGodRayVirtualProximityStrength(std::stof(godRayVirtualProximityStrength->second));
        }
        catch (...)
        {
        }
    }

    const auto renderingQuality = m_settings.find(L"RenderQuality");
    if (renderingQuality != m_settings.end())
    {
        SetRenderQuality(renderingQuality->second);
    }
}

void Render::Initialize(HWND hWnd, const std::wstring& settingsCsvPath)
{
    HRESULT hResult = E_FAIL;

    m_hWnd = hWnd;

    LoadSettingsCsv(settingsCsvPath);

    m_windowManager.Initialize(hWnd);

    m_sprite.Initialize();
    m_particleSystem.Initialize();

    CreateTexture();

    ApplySettings();

    // 画面転送
    m_postEffectEnd.Initialize();

    const MMRESULT timerResolutionResult = timeBeginPeriod(1);
    m_hasRequestedTimerResolution = (timerResolutionResult == TIMERR_NOERROR);

    m_hasLastFrameTime = false;
    m_hasLastFramePacingTime = false;

    Common::AddDeviceLostResource(this);
}

void Render::Finalize()
{
    m_settingsDialog.Finalize();

    MeshMixManager::SetSharedThicknessTexture(NULL);

    m_postEffectZShadow.Finalize();
    m_postEffectSSGI.Finalize();
    m_postEffectSSAO.Finalize();
    m_postEffectFog.Finalize();
    m_postEffectHeightFog.Finalize();
    m_postEffectSaturate.Finalize();
    m_postEffectDepthOfField.Finalize();
    m_PostEffectBloom.Finalize();
    m_postEffectHalo.Finalize();
    m_postEffectStarBurst.Finalize();
    m_postEffectGodRay.Finalize();
    m_postEffectGauss.Finalize();
    m_postEffectMaskedGauss.Finalize();
    m_postEffectAA.Finalize();
    m_postEffectMotionBlurCamera.Finalize();
    m_postEffectFXAA.Finalize();
    m_postEffectTAA.Finalize();
    m_postEffectEnd.Finalize();
    m_particleSystem.Finalize();

    if (m_hasRequestedTimerResolution)
    {
        timeEndPeriod(1);
        m_hasRequestedTimerResolution = false;
    }

    m_hasLastFrameTime = false;
    m_hasLastFramePacingTime = false;

    for (auto& mesh : m_meshList)
    {
        mesh.Finalize();
    }
    m_meshList.clear();
    m_meshEnabledList.clear();

    for (auto& mesh : m_meshSmoothList)
    {
        mesh.Finalize();
    }
    m_meshSmoothList.clear();

    for (auto& mesh : m_meshSSSLikeList)
    {
        mesh.Finalize();
    }
    m_meshSSSLikeList.clear();

    for (auto& mesh : m_meshSSSList)
    {
        mesh.Finalize();
    }
    m_meshSSSList.clear();
    m_meshSSSEnabledList.clear();

    for (auto& mesh : m_meshPointLightList)
    {
        mesh.Finalize();
    }
    m_meshPointLightList.clear();
    m_meshPointLightEnabledList.clear();

    for (auto& mesh : m_meshNormalMapList)
    {
        mesh.Finalize();
    }
    m_meshNormalMapList.clear();
    m_meshNormalMapEnabledList.clear();

    for (auto& mesh : m_meshPOMList)
    {
        mesh.Finalize();
    }
    m_meshPOMList.clear();
    m_meshPOMEnabledList.clear();

    for (auto& mesh : m_animMeshList)
    {
        SAFE_DELETE(mesh);
    }
    m_animMeshList.clear();

    for (auto& mesh : m_skinAnimMeshList)
    {
        SAFE_DELETE(mesh);
    }
    m_skinAnimMeshList.clear();

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        SAFE_DELETE(mesh);
    }
    m_meshMixSkinAnimList.clear();

    for (auto& mesh : m_meshInstancingMap)
    {
        SAFE_DELETE(mesh.second);
    }
    m_meshInstancingMap.clear();

    for (auto& mesh : m_meshMixList)
    {
        mesh.Finalize();
    }
    m_meshMixList.clear();

    for (auto& mesh : m_meshPBRList)
    {
        mesh.Finalize();
    }
    m_meshPBRList.clear();

    for (auto& font : m_fontList)
    {
        SAFE_DELETE(font);
    }
    m_fontList.clear();

    for (auto& font : m_fontExList)
    {
        if (font != nullptr)
        {
            font->Finalize();
        }
        SAFE_DELETE(font);
    }
    m_fontExList.clear();

    if (m_loadingScreenTitleFontRegistered)
    {
        RemoveFontResourceExW(m_loadingScreenTitleFontPath.c_str(), FR_PRIVATE, NULL);
        m_loadingScreenTitleFontRegistered = false;
    }

    m_sprite.Finalize();
    m_GBuffer.Finalize();
    SAFE_RELEASE(m_pRenderTarget1);
    SAFE_RELEASE(m_pRenderTarget2);
    SAFE_RELEASE(m_pLightEffectSourceTexture);
    SAFE_RELEASE(m_pMirrorRenderTarget);

    Common::RemoveDeviceLostResource(this);

    LPDIRECT3DDEVICE9 d3dDevice = Common::D3DDevice();
    SAFE_RELEASE(d3dDevice);
    Common::SetD3DDevice(NULL);

    m_windowManager.Finalize();
    Common::Finalize();
}

void Render::ShowSettingsDialog(const bool activateDialog)
{
    m_settingsDialog.Show(m_hWnd, this, activateDialog);
}

void Render::ToggleSettingsDialog()
{
    m_settingsDialog.Toggle(m_hWnd, this);
}

void Render::Draw()
{
    HRESULT hResult = E_FAIL;

    if (!m_windowManager.EnsureDeviceReady())
    {
        return;
    }

    if (m_bShowFPS)
    {
        float fps = CalcFPS();
        ShowFPS(fps);
    }

    const float frameDeltaSeconds = CalcFrameDeltaSeconds();
    m_particleSystem.Update(frameDeltaSeconds);
    m_loadingScreen.Update(frameDeltaSeconds);
    UpdateMovingPlatforms(frameDeltaSeconds);
    UpdateFade(frameDeltaSeconds);
    UpdateSkinAnimationState();
    CameraShakeFrameScope cameraShakeFrameScope;
    ApplyTAAProjectionJitter();

    //---------------------------------------------------------------
    // ポストエフェクトと一部のメッシュ描画のために深度画像と
    // ワールド座標画像を先に作成
    //---------------------------------------------------------------
    LPDIRECT3DTEXTURE9 pTexTempZ = NULL;
    LPDIRECT3DTEXTURE9 pTexTempCameraZ = NULL;
    LPDIRECT3DTEXTURE9 pTexTempPos = NULL;
    LPDIRECT3DTEXTURE9 pTexTempNoral = NULL;
    LPDIRECT3DTEXTURE9 pTexTempThickness = NULL;
    LPDIRECT3DTEXTURE9 pTexTempBackDepth = NULL;
    if (m_gBufferEnabled)
    {
        EnsureGBufferInitialized();
        m_GBuffer.Draw(m_meshMixList,
                       m_meshMixSkinAnimList,
                       m_meshInstancingMap,
                       &m_particleSystem,
                       &pTexTempZ,
                       &pTexTempCameraZ,
                       &pTexTempPos,
                       &pTexTempNoral,
                       &pTexTempThickness,
                       &pTexTempBackDepth);
        MeshMixManager::SetSharedThicknessTexture(pTexTempThickness);
    }
    else
    {
        MeshMixManager::SetSharedThicknessTexture(NULL);
    }

    int activeMirrorMeshIndex = FindActiveMirrorMeshIndex();
    D3DXMATRIX mirrorViewProj;
    D3DXMatrixIdentity(&mirrorViewProj);
    MeshMixManager::SetSharedMirrorTexture(NULL);
    MeshMixManager::SetSharedMirrorViewProj(mirrorViewProj);
    if (activeMirrorMeshIndex >= 0 && RenderMirrorTexture(activeMirrorMeshIndex))
    {
        MeshMixManager::SetSharedMirrorTexture(m_pMirrorRenderTarget);
    }
    else
    {
        activeMirrorMeshIndex = -1;
    }

    DrawPass1(true, activeMirrorMeshIndex);

    //---------------------------------------------------------------
    // ポストエフェクト
    // 共通の HDR 作業バッファ 2 枚を ping-pong して使う。
    //---------------------------------------------------------------

    LPDIRECT3DTEXTURE9 pTempTexture = m_pRenderTarget1;
    LPDIRECT3DTEXTURE9 pWorkTexture = m_pRenderTarget2;

    if (m_gBufferEnabled && m_postEffectZShadowEnabled)
    {
        EnsurePostEffectZShadowInitialized();
        m_postEffectZShadow.Draw(pTempTexture,
                                 pWorkTexture,
                                 pTexTempZ,
                                 pTexTempNoral,
                                 m_meshMixList,
                                 m_meshMixSkinAnimList,
                                 m_meshInstancingMap);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectSSGIEnabled)
    {
        EnsurePostEffectSSGIInitialized();
        m_postEffectSSGI.Draw(pTempTexture,
                              pWorkTexture,
                              pTexTempZ,
                              pTexTempPos,
                              pTexTempNoral,
                              pTexTempThickness);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectSSAOEnabled)
    {
        EnsurePostEffectSSAOInitialized();
        m_postEffectSSAO.Draw(pTempTexture,
                               pWorkTexture,
                               pTexTempZ,
                               pTexTempPos,
                               pTexTempNoral,
                               pTexTempThickness);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectFogZEnabled)
    {
        EnsurePostEffectFogInitialized();
        m_postEffectFog.Draw(pTempTexture,
                             pWorkTexture,
                             pTexTempCameraZ,
                             pTexTempPos,
                             m_postEffectFogZEnabled,
                             false);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectFogHeightEnabled)
    {
        EnsurePostEffectHeightFogInitialized();
        m_postEffectHeightFog.Draw(pTempTexture, pWorkTexture, pTexTempCameraZ);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectSaturateEnabled)
    {
        EnsurePostEffectSaturateInitialized();
        m_postEffectSaturate.Draw(pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectDepthOfFieldMode == DepthOfFieldMode::Enabled)
    {
        EnsurePostEffectDepthOfFieldInitialized();
        m_postEffectDepthOfField.SetBlend(1.0f);
        m_postEffectDepthOfField.Draw(pTempTexture, pTexTempPos, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }
    else if (m_gBufferEnabled && m_postEffectDepthOfFieldMode == DepthOfFieldMode::AutoNear)
    {
        EnsurePostEffectDepthOfFieldInitialized();
        m_postEffectDepthOfField.UpdateAutoBlend(pTexTempPos);
        if (m_postEffectDepthOfField.GetBlend() > 0.001f)
        {
            m_postEffectDepthOfField.Draw(pTempTexture, pTexTempPos, pWorkTexture);
            SwapPostEffectBuffers(pTempTexture, pWorkTexture);
        }
    }

    const bool lightEffectEnabled = m_gBufferEnabled &&
        (m_postEffectBloomEnabled || m_postEffectHaloEnabled || m_postEffectStarBurstEnabled);
    LPDIRECT3DTEXTURE9 pLightEffectSource = pTempTexture;
    if (lightEffectEnabled)
    {
        CopyTexture(pTempTexture, m_pLightEffectSourceTexture);
        pLightEffectSource = m_pLightEffectSourceTexture;
    }

    if (m_gBufferEnabled && m_postEffectBloomEnabled)
    {
        EnsurePostEffectBloomInitialized();
        m_PostEffectBloom.Draw(pLightEffectSource, pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectHaloEnabled)
    {
        EnsurePostEffectHaloInitialized();
        m_postEffectHalo.Draw(pLightEffectSource, pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectStarBurstEnabled)
    {
        EnsurePostEffectStarBurstInitialized();
        m_postEffectStarBurst.Draw(pLightEffectSource, pTempTexture, pTexTempCameraZ, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectGodRayEnabled)
    {
        EnsurePostEffectGodRayInitialized();
        m_postEffectGodRay.Draw(pTempTexture, pTexTempCameraZ, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectGaussEnabled)
    {
        EnsurePostEffectGaussInitialized();
        m_postEffectGauss.Draw(pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectMaskedGaussEnabled)
    {
        EnsurePostEffectMaskedGaussInitialized();
        m_postEffectMaskedGauss.Draw(pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectAAEnabled)
    {
        EnsurePostEffectAAInitialized();
        m_postEffectAA.Draw(pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectMotionBlurCameraEnabled)
    {
        EnsurePostEffectMotionBlurCameraInitialized();
        bool motionBlurApplied = false;
        m_postEffectMotionBlurCamera.Draw(pTempTexture, pTexTempZ, pWorkTexture, motionBlurApplied);
        if (motionBlurApplied)
        {
            SwapPostEffectBuffers(pTempTexture, pWorkTexture);
        }
    }
    else
    {
        m_postEffectMotionBlurCamera.UpdateFrameMatrices();
    }

    if (m_gBufferEnabled && m_postEffectFXAAEnabled)
    {
        EnsurePostEffectFXAAInitialized();
        m_postEffectFXAA.Draw(pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    if (m_gBufferEnabled && m_postEffectTAAEnabled)
    {
        EnsurePostEffectTAAInitialized();
        m_postEffectTAA.Draw(pTempTexture, pWorkTexture);
        SwapPostEffectBuffers(pTempTexture, pWorkTexture);
    }

    // g_pRenderTargetの内容を画面に転送
    if (m_gBufferEnabled)
    {
        switch (m_debugGBufferView)
        {
        case DebugGBufferView::WorldPos:
            m_postEffectEnd.Draw(pTexTempPos);
            break;
        case DebugGBufferView::Normal:
            m_postEffectEnd.Draw(pTexTempNoral);
            break;
        case DebugGBufferView::Depth:
            m_postEffectEnd.DrawSingleChannel(pTexTempZ);
            break;
        case DebugGBufferView::Thickness:
            m_postEffectEnd.DrawSingleChannel(pTexTempThickness);
            break;
        case DebugGBufferView::BackDepth:
            m_postEffectEnd.DrawSingleChannel(pTexTempBackDepth);
            break;
        default:
            m_postEffectEnd.Draw(pTempTexture);
            break;
        }
    }
    else
    {
        m_postEffectEnd.Draw(pTempTexture);
    }

    // 文字と画像は彩度フィルタの影響を受けないようにする
    Draw2D();

    if (m_postEffectZShadowEnabled)
    {
        if (false)
        {
            m_postEffectZShadow.DrawDebugLightDepthOverlay(10, 10, 256, 256);
        }
    }

    WaitForTargetFrameRate();
    hResult = Common::D3DDevice()->Present(NULL, NULL, NULL, NULL);
    ClearTAAProjectionJitter();
    if (hResult == D3DERR_DEVICELOST)
    {
        m_windowManager.NotifyDeviceLost();
        return;
    }
    assert(hResult == S_OK);

    m_windowManager.ChangeWindowMode();

}

HWND Render::GetWindowHandle() const
{
    return m_hWnd;
}

void Render::UpdateSkinAnimationState()
{
    for (auto& elem : m_skinAnimMeshList)
    {
        if (elem != nullptr)
        {
            elem->UpdateAnimation();
        }
    }

    for (auto& elem : m_meshMixSkinAnimList)
    {
        if (elem != nullptr)
        {
            elem->UpdateAnimation();
        }
    }
}

void Render::ApplyTAAProjectionJitter()
{
    if (!m_gBufferEnabled || !m_postEffectTAAEnabled || Common::ScreenW() <= 0 || Common::ScreenH() <= 0)
    {
        Camera::SetProjectionJitter(0.0f, 0.0f);
        return;
    }

    const unsigned int sampleIndex = (m_taaFrameIndex % 8) + 1;
    ++m_taaFrameIndex;

    const float jitterPixelX = HaltonSequence(sampleIndex, 2) - 0.5f;
    const float jitterPixelY = HaltonSequence(sampleIndex, 3) - 0.5f;
    const float jitterX = (jitterPixelX * 2.0f) / static_cast<float>(Common::ScreenW());
    const float jitterY = (jitterPixelY * -2.0f) / static_cast<float>(Common::ScreenH());
    Camera::SetProjectionJitter(jitterX, jitterY);
}

void Render::ClearTAAProjectionJitter()
{
    Camera::SetProjectionJitter(0.0f, 0.0f);
}

void Render::ChangeResolution(const int W, const int H)
{
    m_windowManager.ChangeResolution(W, H);
}

void Render::ChangeWindowMode(const eWindowMode eWindowMode_)
{
    m_windowManager.RequestWindowMode(eWindowMode_);
    m_windowManager.ChangeWindowMode();
}

int Render::AddMesh(const std::wstring& filePath,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rot,
                    const float scale,
                    const float radius,
                    const float uvTile)
{
    m_meshList.push_back(MeshOld(filePath, pos, rot, scale, radius, uvTile));
    m_meshEnabledList.push_back(true);
    m_meshList.rbegin()->Initialize();

    return (int)m_meshList.size() - 1;
}

bool Render::RemoveMesh(const int id)
{
    if (id < 0 ||
        id >= static_cast<int>(m_meshEnabledList.size()) ||
        id >= static_cast<int>(m_meshList.size()))
    {
        return false;
    }

    m_meshList.at(id).Finalize();
    m_meshEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshNoLighting(const std::wstring& filePath,
                              const D3DXVECTOR3& pos,
                              const D3DXVECTOR3& rot,
                              const float scale,
                              const float radius,
                              const float uvTile)
{
    m_meshList.push_back(MeshOld(L".\\MeshNoLighting.cso", filePath, pos, rot, scale, radius, uvTile));
    m_meshEnabledList.push_back(true);
    m_meshList.rbegin()->Initialize();

    return (int)m_meshList.size() - 1;
}

void Render::AddMeshSmooth(const std::wstring& filePath,
                                     const D3DXVECTOR3& pos,
                                     const D3DXVECTOR3& rot,
                                     const float scale,
                                     const float radius)
{
    MeshSmooth mesh;
    m_meshSmoothList.push_back(mesh);
    m_meshSmoothList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
}

void Render::AddMeshSSSLike(const std::wstring& filePath,
                                      const D3DXVECTOR3& pos,
                                      const D3DXVECTOR3& rot,
                                      const float scale,
                                      const float radius)
{
    MeshSSSLike mesh;
    m_meshSSSLikeList.push_back(mesh);
    m_meshSSSLikeList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
}

int Render::AddMeshSSS(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius)
{
    auto mesh = MeshSSS(filePath, pos, rot, scale, radius);
    m_meshSSSList.push_back(mesh);
    m_meshSSSEnabledList.push_back(true);
    m_meshSSSList.rbegin()->Initialize();
    return static_cast<int>(m_meshSSSList.size()) - 1;
}

bool Render::RemoveMeshSSS(const int id)
{
    if (id < 0 ||
        id >= static_cast<int>(m_meshSSSEnabledList.size()) ||
        id >= static_cast<int>(m_meshSSSList.size()))
    {
        return false;
    }

    m_meshSSSList.at(id).Finalize();
    m_meshSSSEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshPointLight(const std::wstring& filePath,
                              const D3DXVECTOR3& pos,
                              const D3DXVECTOR3& rot,
                              const float scale,
                              const float radius)
{
    MeshPointLight mesh;
    m_meshPointLightList.push_back(mesh);
    m_meshPointLightEnabledList.push_back(true);
    m_meshPointLightList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
    return static_cast<int>(m_meshPointLightList.size()) - 1;
}

bool Render::RemoveMeshPointLight(const int id)
{
    if (id < 0 ||
        id >= static_cast<int>(m_meshPointLightEnabledList.size()) ||
        id >= static_cast<int>(m_meshPointLightList.size()))
    {
        return false;
    }

    m_meshPointLightList.at(id).Finalize();
    m_meshPointLightEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshNormalMapping(const std::wstring& filePath,
                                 const std::wstring& normalMap,
                                 const D3DXVECTOR3& pos,
                                 const D3DXVECTOR3& rot,
                                 const float scale,
                                 const float radius)
{
    MeshNormalMapping mesh;
    m_meshNormalMapList.push_back(mesh);
    m_meshNormalMapEnabledList.push_back(true);
    m_meshNormalMapList.rbegin()->Initialize(filePath, normalMap, pos, rot, scale, radius);
    return static_cast<int>(m_meshNormalMapList.size()) - 1;
}

bool Render::RemoveMeshNormalMapping(const int id)
{
    if (id < 0 ||
        id >= static_cast<int>(m_meshNormalMapEnabledList.size()) ||
        id >= static_cast<int>(m_meshNormalMapList.size()))
    {
        return false;
    }

    m_meshNormalMapList.at(id).Finalize();
    m_meshNormalMapEnabledList.at(id) = false;
    return true;
}

int Render::AddMeshPOM(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius)
{
    MeshPOM mesh;
    m_meshPOMList.push_back(mesh);
    m_meshPOMEnabledList.push_back(true);
    m_meshPOMList.rbegin()->Initialize(filePath, pos, rot, scale, radius);
    return static_cast<int>(m_meshPOMList.size()) - 1;
}

bool Render::RemoveMeshPOM(const int id)
{
    if (id < 0 ||
        id >= static_cast<int>(m_meshPOMEnabledList.size()) ||
        id >= static_cast<int>(m_meshPOMList.size()))
    {
        return false;
    }

    m_meshPOMList.at(id).Finalize();
    m_meshPOMEnabledList.at(id) = false;
    return true;
}

int Render::AddAnimMesh(const std::wstring& filePath,
                        const D3DXVECTOR3& pos,
                        const D3DXVECTOR3& rot,
                        const float scale,
                        const AnimSetMap& animSetMap)
{
    AnimMesh* animMesh = NEW AnimMesh(filePath, pos, rot, scale, animSetMap);
    m_animMeshList.push_back(animMesh);
    return static_cast<int>(m_animMeshList.size()) - 1;
}

bool Render::RemoveAnimMesh(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_animMeshList.size()) || m_animMeshList.at(id) == nullptr)
    {
        return false;
    }

    SAFE_DELETE(m_animMeshList.at(id));
    m_animMeshList.erase(m_animMeshList.begin() + static_cast<std::ptrdiff_t>(id));
    return true;
}

int Render::AddSkinAnimMesh(const std::wstring& filePath,
                            const D3DXVECTOR3& pos,
                            const D3DXVECTOR3& rot,
                            const float scale,
                            const AnimSetMap& animSetMap)
{
    SkinAnimMesh* mesh = NEW SkinAnimMesh(filePath, pos, rot, scale, animSetMap);
    m_skinAnimMeshList.push_back(mesh);
    return static_cast<int>(m_skinAnimMeshList.size()) - 1;
}

bool Render::RemoveSkinAnimMesh(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_skinAnimMeshList.size()) || m_skinAnimMeshList.at(id) == nullptr)
    {
        return false;
    }

    SAFE_DELETE(m_skinAnimMeshList.at(id));
    m_skinAnimMeshList.erase(m_skinAnimMeshList.begin() + static_cast<std::ptrdiff_t>(id));
    return true;
}

int Render::AddMeshInstansing(const std::wstring& filePath,
                              const D3DXVECTOR3& pos,
                              const D3DXVECTOR3& rot,
                              const float scale)
{
    if (m_meshInstancingMap.find(filePath) == m_meshInstancingMap.end())
    {
        MeshInstancing* mesh = NEW MeshInstancing();
        mesh->Initialize(filePath, true);
        mesh->SetHighQuality(m_meshInstancingHighQualityEnabled);

        m_meshInstancingMap[filePath] = mesh;
    }

    m_meshInstancingMap[filePath]->AddInstance(pos);
    return 0;
}

bool Render::RemoveMeshInstancing(const std::wstring& filePath)
{
    const auto found = m_meshInstancingMap.find(filePath);
    if (found == m_meshInstancingMap.end())
    {
        return false;
    }

    SAFE_DELETE(found->second);
    m_meshInstancingMap.erase(found);
    return true;
}

void Render::SetMeshInstancingHighQuality(const bool enabled)
{
    m_meshInstancingHighQualityEnabled = enabled;

    for (auto& mesh : m_meshInstancingMap)
    {
        if (mesh.second != nullptr)
        {
            mesh.second->SetHighQuality(enabled);
        }
    }
}

int Render::AddMeshMix(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius,
                       const bool useParallaxOcclusionMapping,
                       const bool useNormalMapping,
                       const bool async)
{
    auto param = GetMeshParamPreset(eMeshParamPreset::GRASS);
    param.smooth = false;
    param.parallaxOcclusionMapping = useParallaxOcclusionMapping;
    param.normalMapping = useNormalMapping;
    param.saturateShadow = m_meshMixSaturateShadowEnabled;
    param.saturateShadowIntensity = m_meshMixSaturateShadowIntensity;
    param.shadowDarkness = m_meshMixShadowDarkness;
    param.specularIntensity = m_meshMixSpecularIntensity;
    param.specularEdge = m_meshMixSpecularEdge;
    param.specularIntensityOverrideEnabled = m_meshMixSpecularIntensityOverrideEnabled;
    param.specularEdgeOverrideEnabled = m_meshMixSpecularEdgeOverrideEnabled;
    param.cubeMappingRate = m_meshMixCubeMappingRate;
    param.sss = m_meshMixSSSEnabled;
    param.sssIntensity = m_meshMixSSSIntensity;
    param.sssColor = m_meshMixSSSColor;
    auto mesh = MeshMixManager(filePath, pos, rot, scale, param);
    m_meshMixList.push_back(std::move(mesh));
    m_meshMixList.rbegin()->Initialize(async);

    return static_cast<int>(m_meshMixList.size()) - 1;
}

bool Render::RemoveMeshMix(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixList.size()))
    {
        return false;
    }

    m_meshMixList.at(id).Finalize();
    m_meshMixList.erase(m_meshMixList.begin() + static_cast<std::ptrdiff_t>(id));
    return true;
}

int Render::AddMeshPBR(const std::wstring& filePath,
                       const D3DXVECTOR3& pos,
                       const D3DXVECTOR3& rot,
                       const float scale,
                       const float radius,
                       const std::wstring& envMapPath,
                       const bool async)
{
    auto param = GetMeshPBRParamPreset(eMeshPBRParamPreset::GRASS);
    param.smooth = false;
    param.saturateShadow = m_meshMixSaturateShadowEnabled;
    param.saturateShadowIntensity = m_meshMixSaturateShadowIntensity;
    param.shadowDarkness = m_meshMixShadowDarkness;
    param.specularIntensity = m_meshMixSpecularIntensity;
    param.specularEdge = m_meshMixSpecularEdge;
    param.specularIntensityOverrideEnabled = m_meshMixSpecularIntensityOverrideEnabled;
    param.specularEdgeOverrideEnabled = m_meshMixSpecularEdgeOverrideEnabled;
    param.cubeMappingRate = m_meshMixCubeMappingRate;
    param.pbrRoughness = m_meshPBRRoughness;
    param.pbrMetallic = m_meshPBRMetallic;
    param.envReflectionIntensity = m_meshPBREnvReflectionIntensity;
    param.envMaxMipLevel = m_meshPBREnvMaxMipLevel;
    param.envDiffuseIntensity = m_meshPBREnvDiffuseIntensity;
    param.envDiffuseMipLevel = m_meshPBREnvDiffuseMipLevel;
    param.sss = m_meshMixSSSEnabled;
    param.sssIntensity = m_meshMixSSSIntensity;
    param.sssColor = m_meshMixSSSColor;
    param.envMapTexturePath = envMapPath;
    if (!envMapPath.empty())
    {
        param.cubeMapping = true;
    }
    (void)radius;

    auto mesh = MeshPBRManager(filePath, pos, rot, scale, param);
    m_meshPBRList.push_back(std::move(mesh));
    m_meshPBRList.rbegin()->Initialize(async);

    return static_cast<int>(m_meshPBRList.size()) - 1;
}

bool Render::RemoveMeshPBR(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshPBRList.size()))
    {
        return false;
    }

    m_meshPBRList.at(id).Finalize();
    m_meshPBRList.erase(m_meshPBRList.begin() + static_cast<std::ptrdiff_t>(id));
    return true;
}

void Render::SetMeshMixPos(const int id, const D3DXVECTOR3& pos)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixList.size()))
    {
        return;
    }

    m_meshMixList.at(id).SetPos(pos);
}

D3DXVECTOR3 Render::GetMeshMixPos(const int id) const
{
    if (id < 0 || id >= static_cast<int>(m_meshMixList.size()))
    {
        return D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    }

    return m_meshMixList.at(id).GetPos();
}

void Render::SetMeshMixSkinAnimPos(const int id, const D3DXVECTOR3& pos)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixSkinAnimList.size()))
    {
        return;
    }

    m_meshMixSkinAnimList.at(id)->SetPos(pos);
}

void Render::SetMeshMixSkinAnimRotY(const int id, const float rotY)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixSkinAnimList.size()))
    {
        return;
    }

    m_meshMixSkinAnimList.at(id)->SetRotY(rotY);
}

int Render::AddMeshMixSkinAnim(const std::wstring& filePath,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rot,
                               const float scale,
                               const AnimSetMap& animSetMap,
                               const float radius,
                               const bool useParallaxOcclusionMapping,
                               const bool useNormalMapping,
                               const MeshMixSkinAnimLoadMode loadMode)
{
    auto param = GetMeshParamPreset(eMeshParamPreset::GRASS);
    param.smooth = false;
    param.parallaxOcclusionMapping = useParallaxOcclusionMapping;
    param.normalMapping = useNormalMapping;
    param.saturateShadow = m_meshMixSaturateShadowEnabled;
    param.saturateShadowIntensity = m_meshMixSaturateShadowIntensity;
    param.shadowDarkness = m_meshMixShadowDarkness;
    param.specularIntensity = m_meshMixSpecularIntensity;
    param.specularEdge = m_meshMixSpecularEdge;
    param.fresnelIntensity = m_meshMixFresnelIntensity;
    param.specularIntensityOverrideEnabled = m_meshMixSpecularIntensityOverrideEnabled;
    param.specularEdgeOverrideEnabled = m_meshMixSpecularEdgeOverrideEnabled;
    param.treatTextureAsWhite = m_phongTreatTextureAsWhiteEnabled;

    MeshMixSkinAnim* mesh = NEW MeshMixSkinAnim(filePath, pos, rot, scale, param, animSetMap, loadMode);
    mesh->SetAlphaClipEnabled(m_meshMixSkinAnimAlphaClipEnabled);
    mesh->SetIgnoreTransparentMaterial(m_meshMixSkinAnimIgnoreTransparentMaterialEnabled);
    try
    {
        mesh->Initialize(true);
        m_meshMixSkinAnimList.push_back(mesh);
    }
    catch (...)
    {
        SAFE_DELETE(mesh);
        throw;
    }

    return static_cast<int>(m_meshMixSkinAnimList.size()) - 1;
}

int Render::AddMeshMixSkinAnim(const std::wstring& meshFilePath,
                               const std::wstring& animationFilePath,
                               const D3DXVECTOR3& pos,
                               const D3DXVECTOR3& rot,
                               const float scale,
                               const AnimSetMap& animSetMap,
                               const float radius,
                               const bool useParallaxOcclusionMapping,
                               const bool useNormalMapping,
                               const MeshMixSkinAnimLoadMode loadMode)
{
    auto param = GetMeshParamPreset(eMeshParamPreset::GRASS);
    param.smooth = false;
    param.parallaxOcclusionMapping = useParallaxOcclusionMapping;
    param.normalMapping = useNormalMapping;
    param.saturateShadow = m_meshMixSaturateShadowEnabled;
    param.saturateShadowIntensity = m_meshMixSaturateShadowIntensity;
    param.shadowDarkness = m_meshMixShadowDarkness;
    param.specularIntensity = m_meshMixSpecularIntensity;
    param.specularEdge = m_meshMixSpecularEdge;
    param.fresnelIntensity = m_meshMixFresnelIntensity;
    param.specularIntensityOverrideEnabled = m_meshMixSpecularIntensityOverrideEnabled;
    param.specularEdgeOverrideEnabled = m_meshMixSpecularEdgeOverrideEnabled;
    param.treatTextureAsWhite = m_phongTreatTextureAsWhiteEnabled;

    MeshMixSkinAnim* mesh = NEW MeshMixSkinAnim(meshFilePath,
                                               animationFilePath,
                                               pos,
                                               rot,
                                               scale,
                                               param,
                                               animSetMap,
                                               loadMode);
    mesh->SetAlphaClipEnabled(m_meshMixSkinAnimAlphaClipEnabled);
    mesh->SetIgnoreTransparentMaterial(m_meshMixSkinAnimIgnoreTransparentMaterialEnabled);
    try
    {
        mesh->Initialize(true);
        m_meshMixSkinAnimList.push_back(mesh);
    }
    catch (...)
    {
        SAFE_DELETE(mesh);
        throw;
    }

    return static_cast<int>(m_meshMixSkinAnimList.size()) - 1;
}

bool Render::RemoveMeshMixSkinAnim(const int id)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixSkinAnimList.size()) || m_meshMixSkinAnimList.at(id) == nullptr)
    {
        return false;
    }

    SAFE_DELETE(m_meshMixSkinAnimList.at(id));
    m_meshMixSkinAnimList.erase(m_meshMixSkinAnimList.begin() + static_cast<std::ptrdiff_t>(id));
    return true;
}

const std::vector<MeshMixSkinAnim::AnimationInfo>* Render::GetMeshMixSkinAnimAnimationInfoList(const int id) const
{
    if (id < 0 || id >= static_cast<int>(m_meshMixSkinAnimList.size()) || m_meshMixSkinAnimList.at(id) == nullptr)
    {
        return nullptr;
    }

    return &m_meshMixSkinAnimList.at(id)->GetAnimationInfoList();
}

bool Render::PlayMeshMixSkinAnimAnimation(const int id, const std::wstring& name)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixSkinAnimList.size()) || m_meshMixSkinAnimList.at(id) == nullptr)
    {
        return false;
    }

    return m_meshMixSkinAnimList.at(id)->PlayAnimation(name);
}

void Render::SetMeshMixSkinAnimSpeed(const int id, const float speed)
{
    if (id < 0 || id >= static_cast<int>(m_meshMixSkinAnimList.size()) || m_meshMixSkinAnimList.at(id) == nullptr)
    {
        return;
    }

    m_meshMixSkinAnimList.at(id)->SetAnimationSpeed(speed);
}

std::vector<RenderLoadedModelInfo> Render::GetLoadedModelInfoList()
{
    std::vector<RenderLoadedModelInfo> models;

    for (int i = 0; i < static_cast<int>(m_meshMixList.size()); ++i)
    {
        RenderLoadedModelInfo info;
        info.type = RenderLoadedModelType::MeshMix;
        info.renderId = i;
        info.filePath = m_meshMixList.at(i).GetMeshName();
        info.scale = m_meshMixList.at(i).GetScale();
        info.pos = m_meshMixList.at(i).GetPos();
        models.push_back(info);
    }

    for (int i = 0; i < static_cast<int>(m_meshPBRList.size()); ++i)
    {
        RenderLoadedModelInfo info;
        info.type = RenderLoadedModelType::MeshPBR;
        info.renderId = i;
        info.filePath = m_meshPBRList.at(i).GetMeshName();
        info.scale = m_meshPBRList.at(i).GetScale();
        info.pos = m_meshPBRList.at(i).GetPos();
        models.push_back(info);
    }

    int instancingIndex = 0;
    for (const auto& mesh : m_meshInstancingMap)
    {
        RenderLoadedModelInfo info;
        info.type = RenderLoadedModelType::MeshInstancing;
        info.renderId = instancingIndex;
        info.filePath = mesh.first;
        models.push_back(info);
        ++instancingIndex;
    }

    for (int i = 0; i < static_cast<int>(m_meshMixSkinAnimList.size()); ++i)
    {
        const MeshMixSkinAnim* mesh = m_meshMixSkinAnimList.at(i);
        if (mesh == nullptr)
        {
            continue;
        }

        RenderLoadedModelInfo info;
        info.type = RenderLoadedModelType::MeshMixSkinAnim;
        info.renderId = i;
        info.filePath = mesh->GetMeshName();
        info.scale = mesh->GetScale();
        info.pos = mesh->GetPos();
        models.push_back(info);
    }

    return models;
}

void Render::SetMeshMixSkinAnimAlphaClip(const bool enabled)
{
    m_meshMixSkinAnimAlphaClipEnabled = enabled;

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetAlphaClipEnabled(enabled);
        }
    }
}

void Render::SetMeshMixSkinAnimIgnoreTransparentMaterial(const bool enabled)
{
    m_meshMixSkinAnimIgnoreTransparentMaterialEnabled = enabled;

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetIgnoreTransparentMaterial(enabled);
        }
    }
}

void Render::SetMeshMixSaturateShadow(const bool enabled)
{
    m_meshMixSaturateShadowEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSaturateShadow(enabled);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSaturateShadow(enabled);
        }
    }
}

void Render::SetMeshMixSaturateShadowIntensity(const float intensity)
{
    m_meshMixSaturateShadowIntensity = intensity;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSaturateShadowIntensity(intensity);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSaturateShadowIntensity(intensity);
        }
    }
}

void Render::SetMeshMixShadowDarkness(const float darkness)
{
    m_meshMixShadowDarkness = darkness;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetShadowDarkness(darkness);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetShadowDarkness(darkness);
        }
    }
}

void Render::SetMeshMixSpecularIntensity(const float intensity)
{
    m_meshMixSpecularIntensity = intensity;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularIntensity(intensity);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularIntensity(intensity);
        }
    }
}

void Render::SetMeshMixSpecularEdge(const float edge)
{
    m_meshMixSpecularEdge = edge;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularEdge(edge);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularEdge(edge);
        }
    }
}

void Render::SetMeshMixFresnelIntensity(const float intensity)
{
    m_meshMixFresnelIntensity = (std::max)(0.0f, intensity);

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetFresnelIntensity(m_meshMixFresnelIntensity);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetFresnelIntensity(m_meshMixFresnelIntensity);
        }
    }
}

void Render::SetMeshMixEnvMapBlend(const float blend)
{
    m_meshMixCubeMappingRate = blend;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetCubeMappingRate(blend);
    }

    for (auto& mesh : m_meshPBRList)
    {
        mesh.SetCubeMappingRate(blend);
    }
}

void Render::SetMeshPBRRoughness(const float roughness)
{
    m_meshPBRRoughness = roughness;

    for (auto& mesh : m_meshPBRList)
    {
        mesh.SetPBRRoughness(roughness);
    }
}

void Render::SetMeshPBRMetallic(const float metallic)
{
    m_meshPBRMetallic = metallic;

    for (auto& mesh : m_meshPBRList)
    {
        mesh.SetPBRMetallic(metallic);
    }
}

void Render::SetMeshPBREnvReflectionIntensity(const float intensity)
{
    m_meshPBREnvReflectionIntensity = intensity;

    for (auto& mesh : m_meshPBRList)
    {
        mesh.SetPBREnvReflectionIntensity(intensity);
    }
}

void Render::SetMeshPBREnvMaxMipLevel(const float mipLevel)
{
    m_meshPBREnvMaxMipLevel = mipLevel;

    for (auto& mesh : m_meshPBRList)
    {
        mesh.SetPBREnvMaxMipLevel(mipLevel);
    }
}

void Render::SetMeshPBREnvDiffuseIntensity(const float intensity)
{
    m_meshPBREnvDiffuseIntensity = intensity;

    for (auto& mesh : m_meshPBRList)
    {
        mesh.SetPBREnvDiffuseIntensity(intensity);
    }
}

void Render::SetMeshPBREnvDiffuseMipLevel(const float mipLevel)
{
    m_meshPBREnvDiffuseMipLevel = mipLevel;

    for (auto& mesh : m_meshPBRList)
    {
        mesh.SetPBREnvDiffuseMipLevel(mipLevel);
    }
}

bool Render::SetMeshPBREnvMapTexturePath(const std::wstring& envMapTexturePath)
{
    m_pbrEnvMapPath = envMapTexturePath;
    bool allSucceeded = true;
    for (auto& mesh : m_meshPBRList)
    {
        if (!mesh.SetPBREnvMapTexturePath(envMapTexturePath))
        {
            allSucceeded = false;
        }
    }
    return allSucceeded;
}

void Render::SetMeshMixSpecularIntensityOverrideEnabled(const bool enabled)
{
    m_meshMixSpecularIntensityOverrideEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularIntensityOverrideEnabled(enabled);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularIntensityOverrideEnabled(enabled);
        }
    }
}

void Render::SetMeshMixSpecularEdgeOverrideEnabled(const bool enabled)
{
    m_meshMixSpecularEdgeOverrideEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSpecularEdgeOverrideEnabled(enabled);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetSpecularEdgeOverrideEnabled(enabled);
        }
    }
}

void Render::SetPhongTreatTextureAsWhite(const bool enabled)
{
    m_phongTreatTextureAsWhiteEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetTreatTextureAsWhite(enabled);
    }

    for (auto& mesh : m_meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->SetTreatTextureAsWhite(enabled);
        }
    }
}

void Render::SetMeshMixSSS(const bool enabled)
{
    m_meshMixSSSEnabled = enabled;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSSS(enabled);
    }
}

void Render::SetMeshMixSSSIntensity(const float intensity)
{
    m_meshMixSSSIntensity = intensity;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSSSIntensity(intensity);
    }
}

void Render::SetMeshMixSSSColor(const DWORD color)
{
    m_meshMixSSSColor = color;

    for (auto& mesh : m_meshMixList)
    {
        mesh.SetSSSColor(color);
    }
}

bool Render::IsMeshMixSSSEnabled() const { return m_meshMixSSSEnabled; }
float Render::GetMeshMixSSSIntensity() const { return m_meshMixSSSIntensity; }
float Render::GetMeshMixSaturateShadowIntensity() const { return m_meshMixSaturateShadowIntensity; }
float Render::GetMeshMixShadowDarkness() const { return m_meshMixShadowDarkness; }
float Render::GetMeshMixSpecularIntensity() const { return m_meshMixSpecularIntensity; }
float Render::GetMeshMixSpecularEdge() const { return m_meshMixSpecularEdge; }
float Render::GetMeshMixFresnelIntensity() const { return m_meshMixFresnelIntensity; }
float Render::GetMeshMixEnvMapBlend() const { return m_meshMixCubeMappingRate; }
bool Render::IsMeshMixSpecularIntensityOverrideEnabled() const { return m_meshMixSpecularIntensityOverrideEnabled; }
bool Render::IsMeshMixSpecularEdgeOverrideEnabled() const { return m_meshMixSpecularEdgeOverrideEnabled; }
bool Render::IsPhongTreatTextureAsWhiteEnabled() const { return m_phongTreatTextureAsWhiteEnabled; }
bool Render::IsMeshMixSkinAnimAlphaClipEnabled() const { return m_meshMixSkinAnimAlphaClipEnabled; }
bool Render::IsMeshMixSkinAnimIgnoreTransparentMaterialEnabled() const { return m_meshMixSkinAnimIgnoreTransparentMaterialEnabled; }
float Render::GetMeshPBRRoughness() const { return m_meshPBRRoughness; }
float Render::GetMeshPBRMetallic() const { return m_meshPBRMetallic; }
float Render::GetMeshPBREnvReflectionIntensity() const { return m_meshPBREnvReflectionIntensity; }
float Render::GetMeshPBREnvMaxMipLevel() const { return m_meshPBREnvMaxMipLevel; }
float Render::GetMeshPBREnvDiffuseIntensity() const { return m_meshPBREnvDiffuseIntensity; }
float Render::GetMeshPBREnvDiffuseMipLevel() const { return m_meshPBREnvDiffuseMipLevel; }
std::wstring Render::GetMeshPBREnvMapTexturePath() const { return m_pbrEnvMapPath; }

void Render::SetCamera(const D3DXVECTOR3& pos, const D3DXVECTOR3& lookAt)
{
    Camera::SetEyePos(pos);
    Camera::SetLookAtPos(lookAt);
}

void Render::MoveCamera(const D3DXVECTOR3& pos)
{
    auto eyePos = Camera::GetEyePos();
    Camera::SetEyePos(eyePos + pos);

    auto lookAtPos = Camera::GetLookAtPos();
    Camera::SetLookAtPos(lookAtPos + pos);
}

void Render::SetCameraShakeDuration(const float durationSeconds)
{
    m_cameraShakeDurationSeconds = durationSeconds;
    Camera::SetShakeDuration(durationSeconds);
}

void Render::SetCameraShakeIntensity(const float intensity)
{
    m_cameraShakeIntensity = intensity;
    Camera::SetShakeIntensity(intensity);
}

void Render::TriggerCameraShake()
{
    Camera::TriggerShake();
}

void Render::SetCameraClipPlanes(const float nearPlane, const float farPlane)
{
    Camera::SetClipPlanes(nearPlane, farPlane);
    m_GBuffer.SetFogDepthRange(nearPlane, farPlane);
    m_postEffectFog.SetDepthDecodeRange(nearPlane, farPlane);
    m_postEffectFog.SetFogDepthRange(nearPlane, farPlane);
    m_postEffectHeightFog.SetDepthDecodeRange(nearPlane, farPlane);
    m_postEffectGodRay.SetDepthRange(nearPlane, farPlane);
}

void Render::SetCameraHorizontalFovDegrees(const float horizontalFovDegrees)
{
    Camera::SetHorizontalFovDegrees(horizontalFovDegrees);
}

void Render::SetGBufferEnable(const bool enabled)
{
    m_gBufferEnabled = enabled;

    if (!m_gBufferEnabled)
    {
        MeshMixManager::SetSharedThicknessTexture(NULL);
        m_GBuffer.Finalize();
    }
}

void Render::SetGBufferClipPlanes(const float nearPlane, const float farPlane)
{
    m_gBufferNearPlane = nearPlane;
    m_gBufferFarPlane = farPlane;
    const float positionRange = GBuffer::ComputePositionRange(nearPlane, farPlane);
    m_GBuffer.SetDepthRange(nearPlane, farPlane);
    m_postEffectMotionBlurCamera.SetDepthRange(nearPlane, farPlane);
    m_postEffectSSGI.SetDepthRange(nearPlane, farPlane);
    m_postEffectSSAO.SetDepthRange(nearPlane, farPlane);
    m_postEffectDepthOfField.SetPositionRange(positionRange);
}

float Render::GetCameraNearPlane() const
{
    return Camera::GetNear();
}

float Render::GetCameraFarPlane() const
{
    return Camera::GetFar();
}

float Render::GetCameraHorizontalFovDegrees() const
{
    return Camera::GetHorizontalFovDegrees();
}

float Render::GetCameraShakeDuration() const
{
    return m_cameraShakeDurationSeconds;
}

float Render::GetCameraShakeIntensity() const
{
    return m_cameraShakeIntensity;
}

bool Render::IsGBufferEnabled() const
{
    return m_gBufferEnabled;
}

float Render::GetGBufferNearPlane() const
{
    return m_gBufferNearPlane;
}

float Render::GetGBufferFarPlane() const
{
    return m_gBufferFarPlane;
}

RenderingQualitySettings Render::SetRenderQuality(const std::wstring& quality)
{
    RenderingQualitySettings settings;
    std::wstring normalizedQuality = L"LOW";
    if (quality == L"MIDDLE" || quality == L"HIGH")
    {
        normalizedQuality = quality;
    }

    settings.quality = normalizedQuality;
    if (settings.quality == L"LOW")
    {
        settings.gBufferEnabled = false;
        settings.saturateEnabled = false;
        settings.gaussianEnabled = false;
        settings.maskedGaussianEnabled = false;
        settings.postEffectAAEnabled = false;
        settings.fxaaEnabled = false;
        settings.taaEnabled = false;
        settings.motionBlurCameraEnabled = false;
        settings.depthBufferShadowEnabled = false;
        settings.ssaoEnabled = false;
        settings.ssgiEnabled = false;
        settings.fogEnabled = false;
        settings.heightFogEnabled = false;
        settings.bloomEnabled = false;
        settings.haloEnabled = false;
        settings.depthOfFieldMode = DepthOfFieldMode::Disabled;
        settings.starBurstEnabled = false;
        settings.godRayEnabled = false;
    }
    else
    {
        settings.gBufferEnabled = true;
        settings.saturateEnabled = true;
        settings.gaussianEnabled = false;
        settings.maskedGaussianEnabled = false;
        settings.postEffectAAEnabled = false;
        settings.fxaaEnabled = false;
        settings.taaEnabled = true;
        settings.motionBlurCameraEnabled = true;
        settings.depthBufferShadowEnabled = true;
        settings.ssaoEnabled = true;
        settings.ssgiEnabled = true;
        settings.fogEnabled = true;
        settings.heightFogEnabled = true;
        settings.bloomEnabled = true;
        settings.haloEnabled = true;
        settings.depthOfFieldMode = DepthOfFieldMode::Enabled;
        settings.starBurstEnabled = true;
        settings.godRayEnabled = true;
    }

    SetGBufferEnable(settings.gBufferEnabled);
    SetPostEffectSaturateEnable(settings.saturateEnabled);
    SetPostEffectGaussianFilter(settings.gaussianEnabled);
    SetPostEffectMaskedGaussianFilter(settings.maskedGaussianEnabled);
    SetPostEffectAA(settings.postEffectAAEnabled);
    SetPostEffectFXAA(settings.fxaaEnabled);
    SetPostEffectTAA(settings.taaEnabled);
    SetPostEffectMotionBlurCamera(settings.motionBlurCameraEnabled);
    SetPostEffectDepthBufferShadow(settings.depthBufferShadowEnabled);
    SetPostEffectSSAO(settings.ssaoEnabled);
    SetPostEffectSSGI(settings.ssgiEnabled);
    SetPostEffectFog(settings.fogEnabled);
    SetPostEffectHeightFog(settings.heightFogEnabled);
    SetPostEffectBloom(settings.bloomEnabled);
    SetPostEffectHalo(settings.haloEnabled);
    SetPostEffectDepthOfFieldMode(settings.depthOfFieldMode);
    SetPostEffectStarBurst(settings.starBurstEnabled);
    SetPostEffectGodRay(settings.godRayEnabled);

    m_renderingQualitySettings = settings;
    return m_renderingQualitySettings;
}

std::wstring Render::GetRenderQuality() const
{
    return m_renderingQualitySettings.quality;
}

void Render::EnsureGBufferInitialized()
{
    if (!m_GBuffer.IsInitialized())
    {
        m_GBuffer.Initialize();
    }
}

D3DXVECTOR3 Render::GetLookAtPos()
{
    return Camera::GetLookAtPos();
}

D3DXVECTOR3 Render::GetCameraPos()
{
    return Camera::GetEyePos();
}

D3DXVECTOR3 Render::GetCameraRotate()
{
    auto eyePos = Camera::GetEyePos();
    auto lookAtPos = Camera::GetLookAtPos();
    auto dir(lookAtPos - eyePos);
    D3DXVec3Normalize(&dir, &dir);
    return dir;
}

int Render::SetUpFont(const std::wstring& fontName,
                                const int fontSize,
                                const UINT fontColor)
{
    Font* font = NEW Font();
    font->Initialize(fontName, fontSize, fontColor);
    m_fontList.push_back(font);

    return (int)(m_fontList.size() - 1);
}

int Render::SetUpFontEx(const std::wstring& fontName,
                        const int fontSize,
                        const UINT fontColor)
{
    FontEx* font = NEW FontEx();
    font->Initialize(fontName, fontSize, fontColor);
    font->SetGaussianSampleSize(m_fontExGaussianSampleSize);
    m_fontExList.push_back(font);

    return static_cast<int>(m_fontExList.size() - 1);
}

void Render::DrawText_(const int fontId,
                                 const std::wstring& text,
                                 const int X,
                                 const int Y)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddText(text, X, Y);
}

void Render::DrawText_(const int fontId,
                                 const std::wstring& text,
                                 const int X,
                                 const int Y,
                                 const UINT color)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddText(text, X, Y, color);
}

void Render::DrawTextNormalized(const int fontId,
                                const std::wstring& text,
                                const float X,
                                const float Y)
{
    DrawText_(fontId, text, NormalizedTextXToBaseX(X), NormalizedTextYToBaseY(Y));
}

void Render::DrawTextNormalized(const int fontId,
                                const std::wstring& text,
                                const float X,
                                const float Y,
                                const UINT color)
{
    DrawText_(fontId, text, NormalizedTextXToBaseX(X), NormalizedTextYToBaseY(Y), color);
}

void Render::DrawTextEx(const int fontId,
                        const std::wstring& text,
                        const int X,
                        const int Y)
{
    if (fontId >= m_fontExList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontExList.at(fontId)->AddText(text, X, Y);
}

void Render::DrawTextEx(const int fontId,
                        const std::wstring& text,
                        const int X,
                        const int Y,
                        const UINT color)
{
    if (fontId >= m_fontExList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontExList.at(fontId)->AddText(text, X, Y, color);
}

void Render::DrawTextExNormalized(const int fontId,
                                  const std::wstring& text,
                                  const float X,
                                  const float Y)
{
    DrawTextEx(fontId, text, NormalizedTextXToBaseX(X), NormalizedTextYToBaseY(Y));
}

void Render::DrawTextExNormalized(const int fontId,
                                  const std::wstring& text,
                                  const float X,
                                  const float Y,
                                  const UINT color)
{
    DrawTextEx(fontId, text, NormalizedTextXToBaseX(X), NormalizedTextYToBaseY(Y), color);
}

void Render::DrawTextExCenter(const int fontId,
                              const std::wstring& text,
                              const int X,
                              const int Y,
                              const int Width,
                              const int Height)
{
    if (fontId >= m_fontExList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontExList.at(fontId)->AddTextCenter(text, X, Y, Width, Height);
}

void Render::DrawTextExCenter(const int fontId,
                              const std::wstring& text,
                              const int X,
                              const int Y,
                              const int Width,
                              const int Height,
                              const UINT color)
{
    if (fontId >= m_fontExList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontExList.at(fontId)->AddTextCenter(text, X, Y, Width, Height, color);
}

void Render::DrawTextCenter(const int fontId,
                                      const std::wstring& text,
                                      const int X,
                                      const int Y,
                                      const int Width,
                                      const int Height)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddTextCenter(text, X, Y, Width, Height);
}

void Render::DrawTextCenter(const int fontId,
                                      const std::wstring& text,
                                      const int X,
                                      const int Y,
                                      const int Width,
                                      const int Height,
                                      const UINT color)
{
    if (fontId >= m_fontList.size())
    {
        throw std::exception("Illegal fontId");
    }

    m_fontList.at(fontId)->AddTextCenter(text, X, Y, Width, Height, color);
}

void Render::AddSettingsDialogText(const std::wstring& text,
                                   const float X,
                                   const float Y,
                                   const bool decorated)
{
    if (text.empty())
    {
        return;
    }

    RenderSettingsDialogTextInfo textInfo;
    textInfo.text = text;
    textInfo.x = ClampNormalizedTextPosition(X);
    textInfo.y = ClampNormalizedTextPosition(Y);
    textInfo.decorated = decorated;
    m_settingsDialogTextList.push_back(textInfo);
}

std::vector<RenderSettingsDialogTextInfo> Render::GetSettingsDialogTextList() const
{
    return m_settingsDialogTextList;
}

bool Render::SetSettingsDialogTextPosition(const size_t index,
                                           const float X,
                                           const float Y)
{
    if (index >= m_settingsDialogTextList.size())
    {
        return false;
    }

    m_settingsDialogTextList[index].x = ClampNormalizedTextPosition(X);
    m_settingsDialogTextList[index].y = ClampNormalizedTextPosition(Y);
    return true;
}

int Render::AddWorldText(const std::wstring& text,
                          const D3DXVECTOR3& worldPos,
                          const int fontSize,
                          const D3DXCOLOR& color,
                          const bool decorated)
{
    WorldTextInfo info;
    info.text = text;
    info.worldPos = worldPos;
    info.fontSize = fontSize;
    info.color = color;
    info.decorated = decorated;
    m_worldTextList.push_back(info);
    return static_cast<int>(m_worldTextList.size()) - 1;
}

bool Render::RemoveWorldText(const int index)
{
    if (index < 0 || index >= static_cast<int>(m_worldTextList.size()))
    {
        return false;
    }

    m_worldTextList.erase(m_worldTextList.begin() + index);
    return true;
}

void Render::ClearWorldTexts()
{
    m_worldTextList.clear();
}

const std::vector<WorldTextInfo>& Render::GetWorldTextList() const
{
    return m_worldTextList;
}

void Render::DrawImage(const std::wstring& text,
                                 const int X,
                                 const int Y,
                                 const int transparency)
{
    m_sprite.LoadImage_(text);
    m_sprite.PlaceImage(text, X, Y, transparency);
}

void Render::DrawImageStretched(const std::wstring& filename,
                                            const int transparency)
{
    m_sprite.LoadImage_(filename);
    m_sprite.PlaceImage(filename, 0, 0, Common::BASE_W, Common::BASE_H, transparency);
}

void Render::DrawImageStretchedScaled(const std::wstring& filename,
                                                  const float scale,
                                                  const int transparency)
{
    m_sprite.LoadImage_(filename);
    const SIZE imageSize = m_sprite.GetImageSize(filename);
    if (imageSize.cx <= 0 || imageSize.cy <= 0)
    {
        return;
    }

    const int drawW = static_cast<int>(imageSize.cx * scale);
    const int drawH = static_cast<int>(imageSize.cy * scale);
    m_sprite.PlaceImage(filename, 0, 0, drawW, drawH, transparency);
}

void Render::DrawImageEx(const std::wstring& filename,
                                     const int centerX,
                                     const int centerY,
                                     const int transparency,
                                     const bool flipX,
                                     const float scale)
{
    m_sprite.LoadImage_(filename);
    const SIZE imageSize = m_sprite.GetImageSize(filename);
    if (imageSize.cx <= 0 || imageSize.cy <= 0)
    {
        return;
    }

    const int drawW = static_cast<int>(imageSize.cx * scale);
    const int drawH = static_cast<int>(imageSize.cy * scale);
    const int drawX = centerX - drawW / 2;
    const int drawY = centerY - drawH / 2;

    m_sprite.PlaceImage(filename, drawX, drawY, drawW, drawH, transparency, flipX);
}

void Render::DrawImageAutoResize(const std::wstring& text,
                                             const float X,
                                             const float Y,
                                             const int transparency)
{
    DrawImageAutoResizeEx(text, X, Y, 1.0f, false, transparency);
}

void Render::DrawImageAutoResizeEx(const std::wstring& text,
                                   const float X,
                                   const float Y,
                                   const float scale,
                                   const bool flipX,
                                   const int transparency)
{
    m_sprite.LoadImage_(text);
    const SIZE imageSize = m_sprite.GetImageSize(text);
    if (imageSize.cx <= 0 || imageSize.cy <= 0)
    {
        return;
    }

    int drawWidth = static_cast<int>(static_cast<float>(imageSize.cx) * scale);
    int drawHeight = static_cast<int>(static_cast<float>(imageSize.cy) * scale);
    if (drawWidth <= 0)
    {
        drawWidth = 1;
    }
    if (drawHeight <= 0)
    {
        drawHeight = 1;
    }

    const int baseX = static_cast<int>(X * Common::BASE_W) - drawWidth / 2;
    const int baseY = static_cast<int>(Y * Common::BASE_H) - drawHeight / 2;

    m_sprite.PlaceImage(text, baseX, baseY, drawWidth, drawHeight, transparency, flipX);
}

void Render::DrawWorldImage(const std::wstring& filename,
                            const D3DXVECTOR3& worldPos,
                            const int transparency)
{
    const POINT screenPos = Camera::GetScreenPos(worldPos);
    if (screenPos.x < 0 || screenPos.y < 0)
    {
        return;
    }

    m_sprite.LoadImage_(filename);
    const SIZE imageSize = m_sprite.GetImageSize(filename);
    if (imageSize.cx <= 0 || imageSize.cy <= 0)
    {
        return;
    }

    const float scaleX = static_cast<float>(Common::BASE_W) / static_cast<float>(Common::ScreenW());
    const float scaleY = static_cast<float>(Common::BASE_H) / static_cast<float>(Common::ScreenH());
    const int baseX = static_cast<int>(static_cast<float>(screenPos.x) * scaleX);
    const int baseY = static_cast<int>(static_cast<float>(screenPos.y) * scaleY);
    const int imageBaseW = static_cast<int>(static_cast<float>(imageSize.cx) * scaleX);
    const int imageBaseH = static_cast<int>(static_cast<float>(imageSize.cy) * scaleY);

    m_sprite.PlaceImage(filename, baseX - imageBaseW / 2, baseY - imageBaseH / 2, transparency);
}

void Render::SetPostEffectSaturate(const float level)
{
    m_postEffectSaturateLevel = level;
    m_postEffectSaturate.SetPostEffectSaturate(level);
    if (m_postEffectSaturateEnabled)
    {
        EnsurePostEffectSaturateInitialized();
    }
}

void Render::SetPostEffectSaturateEnable(const bool arg)
{
    m_postEffectSaturateEnabled = arg;
    if (m_postEffectSaturateEnabled)
    {
        EnsurePostEffectSaturateInitialized();
    }
    else
    {
        m_postEffectSaturate.Finalize();
    }
}

void Render::SetPostEffectGaussianFilter(const bool arg)
{
    m_postEffectGaussEnabled = arg;
    if (m_postEffectGaussEnabled)
    {
        EnsurePostEffectGaussInitialized();
    }
    else
    {
        m_postEffectGauss.Finalize();
    }
}

void Render::SetPostEffectGaussianSampleSize(const int sampleSize)
{
    m_gaussianSampleSize = NormalizeGaussianSampleSize(sampleSize);
    m_postEffectGauss.SetSampleSize(m_gaussianSampleSize);
    m_postEffectMaskedGauss.SetSampleSize(m_gaussianSampleSize);
}

void Render::SetPostEffectGaussianStrength(const float strength)
{
    m_gaussianStrength = (std::max)(0.0f, (std::min)(strength, 1.0f));
    m_postEffectGauss.SetIntensity(m_gaussianStrength);
    m_postEffectMaskedGauss.SetIntensity(m_gaussianStrength);
}

void Render::SetPostEffectFontSampleSize(const int sampleSize)
{
    m_fontExGaussianSampleSize = NormalizeFontGaussianSampleSize(sampleSize);
    for (auto& font : m_fontExList)
    {
        if (font != nullptr)
        {
            font->SetGaussianSampleSize(m_fontExGaussianSampleSize);
        }
    }
}

void Render::SetPostEffectMaskedGaussianFilter(const bool arg)
{
    m_postEffectMaskedGaussEnabled = arg;
    if (m_postEffectMaskedGaussEnabled)
    {
        EnsurePostEffectMaskedGaussInitialized();
    }
    else
    {
        m_postEffectMaskedGauss.Finalize();
    }
}

void Render::SetPostEffectMaskedGaussianSampleSize(const int sampleSize)
{
    m_gaussianSampleSize = NormalizeGaussianSampleSize(sampleSize);
    m_postEffectMaskedGauss.SetSampleSize(m_gaussianSampleSize);
}

void Render::SetPostEffectMaskedGaussianMaskPath(const std::wstring& maskPath)
{
    m_maskedGaussianMaskPath = maskPath;
    m_postEffectMaskedGauss.SetMaskPath(maskPath);
}

void Render::SetPostEffectAA(const bool arg)
{
    m_postEffectAAEnabled = arg;
    if (m_postEffectAAEnabled)
    {
        EnsurePostEffectAAInitialized();
    }
    else
    {
        m_postEffectAA.Finalize();
    }
}

void Render::SetPostEffectFXAA(const bool arg)
{
    m_postEffectFXAAEnabled = arg;
    if (m_postEffectFXAAEnabled)
    {
        EnsurePostEffectFXAAInitialized();
    }
    else
    {
        m_postEffectFXAA.Finalize();
    }
}

void Render::SetPostEffectFXAAQuality(const int quality)
{
    m_fxaaQuality = NormalizeFXAAQuality(quality);
    m_postEffectFXAA.SetQuality(m_fxaaQuality);
}

void Render::SetPostEffectTAA(const bool arg)
{
    m_postEffectTAAEnabled = arg;
    if (m_postEffectTAAEnabled)
    {
        EnsurePostEffectTAAInitialized();
        m_postEffectTAA.SetHistoryWeight(m_taaHistoryWeight);
        m_taaFrameIndex = 0;
        m_postEffectTAA.ResetHistory();
    }
    else
    {
        m_taaFrameIndex = 0;
        m_postEffectTAA.Finalize();
    }
}

void Render::SetPostEffectTAAHistoryWeight(const float historyWeight)
{
    m_taaHistoryWeight = (std::max)(0.0f, (std::min)(historyWeight, 1.0f));
    m_postEffectTAA.SetHistoryWeight(m_taaHistoryWeight);
}

void Render::SetPostEffectMotionBlurCamera(const bool arg)
{
    m_postEffectMotionBlurCameraEnabled = arg;
    if (m_postEffectMotionBlurCameraEnabled)
    {
        EnsurePostEffectMotionBlurCameraInitialized();
    }
    else
    {
        m_postEffectMotionBlurCamera.Finalize();
    }
}

void Render::SetPostEffectMotionBlurCameraQuality(const int quality)
{
    m_motionBlurCameraQuality = NormalizeMotionBlurCameraQuality(quality);
    m_postEffectMotionBlurCamera.SetQuality(m_motionBlurCameraQuality);
    m_motionBlurCameraMaxBlurPixels = m_postEffectMotionBlurCamera.GetMaxBlurPixels();
    m_motionBlurCameraSampleCount = m_postEffectMotionBlurCamera.GetSampleCount();
}

void Render::SetPostEffectMotionBlurCameraMaxBlurPixels(const float maxBlurPixels)
{
    m_motionBlurCameraMaxBlurPixels = NormalizeMotionBlurCameraMaxBlurPixels(maxBlurPixels);
    m_postEffectMotionBlurCamera.SetMaxBlurPixels(m_motionBlurCameraMaxBlurPixels);
}

void Render::SetPostEffectMotionBlurCameraSampleCount(const int sampleCount)
{
    m_motionBlurCameraSampleCount = NormalizeMotionBlurCameraSampleCount(sampleCount);
    m_postEffectMotionBlurCamera.SetSampleCount(m_motionBlurCameraSampleCount);
}

void Render::SetPostEffectDepthBufferShadow(const bool arg)
{
    m_postEffectZShadowEnabled = arg;
    if (m_gBufferEnabled && m_postEffectZShadowEnabled)
    {
        EnsurePostEffectZShadowInitialized();
    }
    else
    {
        m_postEffectZShadow.Finalize();
    }
}

void Render::SetPostEffectDepthBufferShadowIntensity(const float intensity)
{
    m_postEffectDepthBufferShadowIntensity = intensity;
    m_postEffectZShadow.SetShadowIntensity(intensity);
}

void Render::SetPostEffectDepthBufferShadowSaturationBoost(const float saturationBoost)
{
    m_postEffectDepthBufferShadowSaturationBoost = saturationBoost;
    m_postEffectZShadow.SetShadowSaturationBoost(saturationBoost);
}

void Render::SetPostEffectDepthBufferShadowCoverage(const float coverage)
{
    m_postEffectDepthBufferShadowCoverage = coverage;
    m_postEffectZShadow.SetCoverage(coverage);
}

void Render::SetPostEffectDepthBufferShadowPcfTapCount(const int tapCount)
{
    m_postEffectDepthBufferShadowPcfTapCount = tapCount;
    m_postEffectZShadow.SetPcfTapCount(tapCount);
}

void Render::SetPostEffectDepthBufferShadowCompositeTapCount(const int tapCount)
{
    m_postEffectDepthBufferShadowCompositeTapCount = tapCount;
    m_postEffectZShadow.SetCompositeTapCount(tapCount);
}

void Render::SetPostEffectDepthBufferShadowTexSizeDivisor(const int scaleDivisor)
{
    m_postEffectDepthBufferShadowTexSizeDivisor = scaleDivisor;
    m_postEffectZShadow.SetShadowTextureScaleDivisor(scaleDivisor);
}

float Render::GetPostEffectSaturate() const { return m_postEffectSaturateLevel; }
bool Render::IsPostEffectSaturateEnabled() const { return m_postEffectSaturateEnabled; }
bool Render::IsPostEffectGaussianFilterEnabled() const { return m_postEffectGaussEnabled; }
int Render::GetPostEffectGaussianSampleSize() const { return m_gaussianSampleSize; }
float Render::GetPostEffectGaussianStrength() const { return m_gaussianStrength; }
int Render::GetPostEffectFontSampleSize() const { return m_fontExGaussianSampleSize; }
bool Render::IsPostEffectMaskedGaussianFilterEnabled() const { return m_postEffectMaskedGaussEnabled; }
std::wstring Render::GetPostEffectMaskedGaussianMaskPath() const { return m_maskedGaussianMaskPath; }
bool Render::IsPostEffectAAEnabled() const { return m_postEffectAAEnabled; }
bool Render::IsPostEffectFXAAEnabled() const { return m_postEffectFXAAEnabled; }
int Render::GetPostEffectFXAAQuality() const { return m_fxaaQuality; }
bool Render::IsPostEffectTAAEnabled() const { return m_postEffectTAAEnabled; }
float Render::GetPostEffectTAAHistoryWeight() const { return m_taaHistoryWeight; }
bool Render::IsPostEffectMotionBlurCameraEnabled() const { return m_postEffectMotionBlurCameraEnabled; }
float Render::GetPostEffectMotionBlurCameraMaxBlurPixels() const { return m_motionBlurCameraMaxBlurPixels; }
int Render::GetPostEffectMotionBlurCameraSampleCount() const { return m_motionBlurCameraSampleCount; }
bool Render::IsPostEffectDepthBufferShadowEnabled() const { return m_postEffectZShadowEnabled; }
float Render::GetPostEffectDepthBufferShadowIntensity() const { return m_postEffectDepthBufferShadowIntensity; }
float Render::GetPostEffectDepthBufferShadowSaturationBoost() const { return m_postEffectDepthBufferShadowSaturationBoost; }
float Render::GetPostEffectDepthBufferShadowCoverage() const { return m_postEffectDepthBufferShadowCoverage; }
int Render::GetPostEffectDepthBufferShadowPcfTapCount() const { return m_postEffectDepthBufferShadowPcfTapCount; }
int Render::GetPostEffectDepthBufferShadowCompositeTapCount() const { return m_postEffectDepthBufferShadowCompositeTapCount; }
int Render::GetPostEffectDepthBufferShadowTexSizeDivisor() const { return m_postEffectDepthBufferShadowTexSizeDivisor; }

void Render::EnsurePostEffectSaturateInitialized()
{
    m_postEffectSaturate.Initialize();
}

void Render::EnsurePostEffectGaussInitialized()
{
    m_postEffectGauss.Initialize();
}

void Render::EnsurePostEffectMaskedGaussInitialized()
{
    m_postEffectMaskedGauss.Initialize();
}

void Render::EnsurePostEffectAAInitialized()
{
    m_postEffectAA.Initialize();
}

void Render::EnsurePostEffectFXAAInitialized()
{
    m_postEffectFXAA.Initialize();
}

void Render::EnsurePostEffectTAAInitialized()
{
    m_postEffectTAA.Initialize();
    m_postEffectTAA.SetHistoryWeight(m_taaHistoryWeight);
}

void Render::EnsurePostEffectMotionBlurCameraInitialized()
{
    m_postEffectMotionBlurCamera.Initialize();
}

void Render::EnsurePostEffectZShadowInitialized()
{
    m_postEffectZShadow.Initialize();
}

void Render::EnsurePostEffectSSAOInitialized()
{
    m_postEffectSSAO.Initialize();
}

void Render::EnsurePostEffectSSGIInitialized()
{
    m_postEffectSSGI.Initialize();
}

void Render::EnsurePostEffectFogInitialized()
{
    m_postEffectFog.Initialize();
}

void Render::EnsurePostEffectHeightFogInitialized()
{
    m_postEffectHeightFog.Initialize();
}

void Render::EnsurePostEffectBloomInitialized()
{
    m_PostEffectBloom.Initialize();
}

void Render::EnsurePostEffectHaloInitialized()
{
    m_postEffectHalo.Initialize();
}

void Render::EnsurePostEffectDepthOfFieldInitialized()
{
    m_postEffectDepthOfField.Initialize();
}

void Render::EnsurePostEffectStarBurstInitialized()
{
    m_postEffectStarBurst.Initialize();
}

void Render::EnsurePostEffectGodRayInitialized()
{
    m_postEffectGodRay.Initialize();
}

void Render::SwapPostEffectBuffers(LPDIRECT3DTEXTURE9& texSource,
                                   LPDIRECT3DTEXTURE9& texTarget)
{
    LPDIRECT3DTEXTURE9 temp = texSource;
    texSource = texTarget;
    texTarget = temp;
}

void Render::CopyTexture(LPDIRECT3DTEXTURE9 texSource,
                         LPDIRECT3DTEXTURE9 texTarget)
{
    if (texSource == NULL || texTarget == NULL)
    {
        return;
    }

    LPDIRECT3DSURFACE9 sourceSurface = NULL;
    LPDIRECT3DSURFACE9 targetSurface = NULL;
    LPDIRECT3DSURFACE9 oldRenderTarget = NULL;
    LPDIRECT3DSURFACE9 backBuffer = NULL;
    HRESULT hResult = Common::D3DDevice()->GetRenderTarget(0, &oldRenderTarget);
    assert(hResult == S_OK);
    hResult = Common::D3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    assert(hResult == S_OK);
    hResult = Common::D3DDevice()->SetRenderTarget(0, backBuffer);
    assert(hResult == S_OK);

    hResult = texSource->GetSurfaceLevel(0, &sourceSurface);
    assert(hResult == S_OK);
    hResult = texTarget->GetSurfaceLevel(0, &targetSurface);
    assert(hResult == S_OK);
    hResult = Common::D3DDevice()->StretchRect(sourceSurface,
                                              NULL,
                                              targetSurface,
                                              NULL,
                                              D3DTEXF_NONE);
    assert(hResult == S_OK);
    hResult = Common::D3DDevice()->SetRenderTarget(0, oldRenderTarget);
    assert(hResult == S_OK);
    SAFE_RELEASE(sourceSurface);
    SAFE_RELEASE(targetSurface);
    SAFE_RELEASE(backBuffer);
    SAFE_RELEASE(oldRenderTarget);
}

void Render::SetPostEffectSSAO(const bool arg)
{
    m_postEffectSSAOEnabled = arg;
    if (m_postEffectSSAOEnabled)
    {
        EnsurePostEffectSSAOInitialized();
    }
    else
    {
        m_postEffectSSAO.Finalize();
    }
}

void Render::SetPostEffectSSAOBlur(const bool arg)
{
    m_postEffectSSAOBlurEnabled = arg;
    m_postEffectSSAO.SetBlurEnabled(arg);
}

void Render::SetPostEffectSSAOSeparableBlur(const bool enabled)
{
    m_postEffectSSAOSeparableBlurEnabled = enabled;
    m_postEffectSSAO.SetSeparableBlurEnabled(enabled);
}

void Render::SetPostEffectSSAOShadowStrength(const float shadowStrength)
{
    m_postEffectSSAOShadowStrength = shadowStrength;
    m_postEffectSSAO.SetShadowStrength(shadowStrength);
}

void Render::SetPostEffectSSAOSaturationBoost(const float saturationBoost)
{
    m_postEffectSSAOSaturationBoost = saturationBoost;
    m_postEffectSSAO.SetSaturationBoost(saturationBoost);
}

void Render::SetPostEffectSSAOSampleCount(const int sampleCount)
{
    const int normalizedSampleCount = (std::max)(1, (std::min)(sampleCount, 64));
    m_postEffectSSAOSampleCount = normalizedSampleCount;
    m_postEffectSSAO.SetSampleCount(normalizedSampleCount);
}

void Render::SetPostEffectSSAORandomSamplingDirection(const bool enabled)
{
    m_postEffectSSAORandomSamplingDirectionEnabled = enabled;
    m_postEffectSSAO.SetRandomSamplingDirectionEnabled(enabled);
}

void Render::SetPostEffectSSAODepthScaledSampleDistance(const bool enabled)
{
    m_postEffectSSAODepthScaledSampleDistanceEnabled = enabled;
    m_postEffectSSAO.SetDepthScaledSampleDistanceEnabled(enabled);
}

void Render::SetPostEffectSSAOSampleRadius(const float sampleRadius)
{
    m_postEffectSSAOSampleRadius = sampleRadius;
    m_postEffectSSAO.SetSampleRadius(sampleRadius);
}

void Render::SetPostEffectSSAOBlurKernelSize(const int kernelSize)
{
    m_postEffectSSAOBlurKernelSize = kernelSize;
    m_postEffectSSAO.SetBlurKernelSize(kernelSize);
}

void Render::SetPostEffectSSAOTexSizeDivisor(const int scaleDivisor)
{
    m_postEffectSSAOTexSizeDivisor = scaleDivisor;
    m_postEffectSSAO.SetTextureScaleDivisor(scaleDivisor);
}

void Render::SetPostEffectSSAOCompositeGaussian3x3(const bool enabled)
{
    m_postEffectSSAOCompositeGaussian3x3Enabled = enabled;
    m_postEffectSSAO.SetCompositeGaussian3x3Enabled(enabled);
}

void Render::SetPostEffectSSAOMaxDarknessClamp(const bool enabled)
{
    m_postEffectSSAOMaxDarknessClampEnabled = enabled;
    m_postEffectSSAO.SetMaxDarknessClampEnabled(enabled);
}

void Render::SetPostEffectSSGI(const bool arg)
{
    m_postEffectSSGIEnabled = arg;
    if (m_postEffectSSGIEnabled)
    {
        EnsurePostEffectSSGIInitialized();
    }
    else
    {
        m_postEffectSSGI.Finalize();
    }
}

void Render::SetPostEffectSSGIBlur(const bool arg)
{
    m_postEffectSSGIBlurEnabled = arg;
    m_postEffectSSGI.SetBlurEnabled(arg);
}

void Render::SetPostEffectSSGISeparableBlur(const bool enabled)
{
    m_postEffectSSGISeparableBlurEnabled = enabled;
    m_postEffectSSGI.SetSeparableBlurEnabled(enabled);
}

void Render::SetPostEffectSSGISampleCount(const int sampleCount)
{
    const int normalizedSampleCount = (std::max)(1, (std::min)(sampleCount, 64));
    m_postEffectSSGISampleCount = normalizedSampleCount;
    m_postEffectSSGI.SetSampleCount(normalizedSampleCount);
}

void Render::SetPostEffectSSGIDepthScaledSampleDistance(const bool enabled)
{
    m_postEffectSSGIDepthScaledSampleDistanceEnabled = enabled;
    m_postEffectSSGI.SetDepthScaledSampleDistanceEnabled(enabled);
}

void Render::SetPostEffectSSGISampleRadius(const float sampleRadius)
{
    m_postEffectSSGISampleRadius = sampleRadius;
    m_postEffectSSGI.SetSampleRadius(sampleRadius);
}

void Render::SetPostEffectSSGIBlurKernelSize(const int kernelSize)
{
    m_postEffectSSGIBlurKernelSize = kernelSize;
    m_postEffectSSGI.SetBlurKernelSize(kernelSize);
}

void Render::SetPostEffectSSGITexSizeDivisor(const int scaleDivisor)
{
    m_postEffectSSGITexSizeDivisor = scaleDivisor;
    m_postEffectSSGI.SetTextureScaleDivisor(scaleDivisor);
}

void Render::SetPostEffectSSGIIndirectLightStrength(const float strength)
{
    m_postEffectSSGIIndirectLightStrength = strength;
    m_postEffectSSGI.SetIndirectLightStrength(strength);
}

void Render::SetPostEffectSSGIIndirectLightMaxContribution(const float maxContribution)
{
    m_postEffectSSGIIndirectLightMaxContribution = maxContribution;
    m_postEffectSSGI.SetIndirectLightMaxContribution(maxContribution);
}

void Render::SetPostEffectSSGIUseThickness(const bool enabled)
{
    m_postEffectSSGIUseThicknessEnabled = enabled;
    m_postEffectSSGI.SetUseThicknessEnabled(enabled);
}

void Render::SetPostEffectFog(const bool arg)
{
    m_postEffectFogZEnabled = arg;
    if (m_postEffectFogZEnabled)
    {
        EnsurePostEffectFogInitialized();
    }
    else
    {
        m_postEffectFog.Finalize();
    }
}

void Render::SetPostEffectFogIntensity(const float intensity)
{
    m_postEffectFogIntensity = intensity;
    m_postEffectFog.SetIntensityZ(intensity);
}

void Render::SetPostEffectFogColor(const D3DXCOLOR& color)
{
    m_postEffectFogColor = color;
    m_postEffectFog.SetFogColor(color);
    m_postEffectHeightFog.SetFogColor(color);
}

void Render::SetPostEffectHeightFog(const bool arg)
{
    m_postEffectFogHeightEnabled = arg;
    if (m_postEffectFogHeightEnabled)
    {
        EnsurePostEffectHeightFogInitialized();
    }
    else
    {
        m_postEffectHeightFog.Finalize();
    }
}

void Render::SetPostEffectHeightFogIntensity(const float intensity)
{
    m_postEffectHeightFogIntensity = intensity;
    m_postEffectHeightFog.SetIntensity(intensity);
}

void Render::SetPostEffectHeightFogStart(const float start)
{
    m_postEffectHeightFogStart = start;
    m_postEffectHeightFog.SetStartHeight(start);
}

void Render::SetPostEffectHeightFogMax(const float maxHeight)
{
    m_postEffectHeightFogMax = maxHeight;
    m_postEffectHeightFog.SetMaxHeight(maxHeight);
}

void Render::SetPostEffectHeightFogDistanceStart(const float distanceStart)
{
    m_postEffectHeightFogDistanceStart = distanceStart;
    m_postEffectHeightFog.SetDistanceStart(distanceStart);
}

void Render::SetPostEffectHeightFogDistanceMax(const float distanceMax)
{
    m_postEffectHeightFogDistanceMax = distanceMax;
    m_postEffectHeightFog.SetDistanceMax(distanceMax);
}

void Render::SetPostEffectFogHeightEnable(const bool arg)
{
    SetPostEffectHeightFog(arg);
}

void Render::SetPostEffectFogHeightIntensity(const float intensity)
{
    SetPostEffectHeightFogIntensity(intensity);
}

void Render::SetPostEffectFogHeightStart(const float start)
{
    SetPostEffectHeightFogStart(start);
}

void Render::SetPostEffectBloom(const bool arg)
{
    m_postEffectBloomEnabled = arg;
    if (m_postEffectBloomEnabled)
    {
        EnsurePostEffectBloomInitialized();
    }
    else
    {
        m_PostEffectBloom.Finalize();
    }
}

void Render::SetPostEffectBloomThreshold(const float threshold)
{
    m_postEffectBloomThreshold = threshold;
    m_PostEffectBloom.SetThreshold(threshold);
}

void Render::SetPostEffectBloomWeightSum(const float weightSum)
{
    m_postEffectBloomWeightSum = weightSum;
    m_PostEffectBloom.SetWeightSum(weightSum);
}

void Render::SetPostEffectHalo(const bool arg)
{
    m_postEffectHaloEnabled = arg;
    if (m_postEffectHaloEnabled)
    {
        EnsurePostEffectHaloInitialized();
    }
    else
    {
        m_postEffectHalo.Finalize();
    }
}

void Render::SetPostEffectHaloThreshold(const float threshold)
{
    m_postEffectHaloThreshold = threshold;
    m_postEffectHalo.SetThreshold(threshold);
}

void Render::SetPostEffectDepthOfField(const bool arg)
{
    if (arg)
    {
        SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Enabled);
    }
    else
    {
        SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
    }
}

void Render::SetPostEffectDepthOfFieldMode(const DepthOfFieldMode mode)
{
    m_postEffectDepthOfFieldMode = mode;
    if (mode != DepthOfFieldMode::Disabled)
    {
        EnsurePostEffectDepthOfFieldInitialized();
    }
    if (mode == DepthOfFieldMode::Disabled)
    {
        m_postEffectDepthOfField.SetBlend(0.0f);
        m_postEffectDepthOfField.Finalize();
    }
    else if (mode == DepthOfFieldMode::Enabled)
    {
        m_postEffectDepthOfField.SetBlend(1.0f);
    }
}

void Render::SetPostEffectDepthOfFieldFocalDistance(const float distance)
{
    m_postEffectDepthOfFieldFocalDistance = distance;
    m_postEffectDepthOfField.SetFocalDistance(distance);
}

void Render::SetPostEffectDepthOfFieldStartNear(const float distance)
{
    m_postEffectDepthOfFieldStartNear = distance;
    m_postEffectDepthOfField.SetStartNear(distance);
}

void Render::SetPostEffectDepthOfFieldMaxBlurDistance(const float distance)
{
    m_postEffectDepthOfFieldMaxBlurDistance = distance;
    m_postEffectDepthOfField.SetMaxBlurDistance(distance);
}

void Render::SetPostEffectDepthOfFieldAutoActivationDistance(const float distance)
{
    m_postEffectDepthOfFieldAutoActivationDistance = distance;
    m_postEffectDepthOfField.SetAutoActivationDistance(distance);
}

void Render::SetPostEffectStarBurst(const bool arg)
{
    m_postEffectStarBurstEnabled = arg;
    if (m_postEffectStarBurstEnabled)
    {
        EnsurePostEffectStarBurstInitialized();
    }
    else
    {
        m_postEffectStarBurst.Finalize();
    }
}

void Render::SetPostEffectStarBurstThreshold(const float threshold)
{
    m_postEffectStarBurstThreshold = threshold;
    m_postEffectStarBurst.SetThreshold(threshold);
}

void Render::SetPostEffectStarBurstDistanceFade(const float fade)
{
    m_postEffectStarBurstDistanceFade = fade;
    m_postEffectStarBurst.SetDistanceFade(fade);
}

void Render::SetPostEffectGodRay(const bool arg)
{
    m_postEffectGodRayEnabled = arg;
    if (m_postEffectGodRayEnabled)
    {
        EnsurePostEffectGodRayInitialized();
    }
    else
    {
        m_postEffectGodRay.Finalize();
    }
}

void Render::SetPostEffectGodRayLightPos(const D3DXVECTOR3& pos)
{
    m_postEffectGodRayLightPos = pos;
    m_postEffectGodRay.SetLightPos(pos);
}

void Render::SetPostEffectGodRayReverseSampling(const bool arg)
{
    m_postEffectGodRay.SetReverseSampling(arg);
}

void Render::SetPostEffectGodRayRayLength(const float arg)
{
    m_postEffectGodRay.SetRayLength(arg);
}

void Render::SetPostEffectGodRayIntensity(const float arg)
{
    m_postEffectGodRayIntensity = arg;
    m_postEffectGodRay.SetRayIntensity(arg);
}

void Render::SetPostEffectGodRayVirtualProximityStrength(const float arg)
{
    m_postEffectGodRayVirtualProximityStrength = arg;
    m_postEffectGodRay.SetVirtualProximityStrength(arg);
}

void Render::SetPostEffectGodRayOcclusionFalloff(const float arg)
{
    m_postEffectGodRay.SetOcclusionFalloff(arg);
}

void Render::SetPostEffectGodRayLightColor(const D3DXVECTOR3& color)
{
    m_postEffectGodRayLightColor = color;
    m_postEffectGodRay.SetLightColor(color);
}

void Render::SetDebugGBufferView(const DebugGBufferView view)
{
    m_debugGBufferView = view;
}

bool Render::IsPostEffectSSAOEnabled() const { return m_postEffectSSAOEnabled; }
bool Render::IsPostEffectSSAOBlurEnabled() const { return m_postEffectSSAOBlurEnabled; }
bool Render::IsPostEffectSSAOSeparableBlurEnabled() const { return m_postEffectSSAOSeparableBlurEnabled; }
float Render::GetPostEffectSSAOShadowStrength() const { return m_postEffectSSAOShadowStrength; }
float Render::GetPostEffectSSAOSaturationBoost() const { return m_postEffectSSAOSaturationBoost; }
int Render::GetPostEffectSSAOSampleCount() const { return m_postEffectSSAOSampleCount; }
bool Render::IsPostEffectSSAORandomSamplingDirectionEnabled() const { return m_postEffectSSAORandomSamplingDirectionEnabled; }
bool Render::IsPostEffectSSAODepthScaledSampleDistanceEnabled() const { return m_postEffectSSAODepthScaledSampleDistanceEnabled; }
float Render::GetPostEffectSSAOSampleRadius() const { return m_postEffectSSAOSampleRadius; }
int Render::GetPostEffectSSAOBlurKernelSize() const { return m_postEffectSSAOBlurKernelSize; }
int Render::GetPostEffectSSAOTexSizeDivisor() const { return m_postEffectSSAOTexSizeDivisor; }
bool Render::IsPostEffectSSAOCompositeGaussian3x3Enabled() const { return m_postEffectSSAOCompositeGaussian3x3Enabled; }
bool Render::IsPostEffectSSAOMaxDarknessClampEnabled() const { return m_postEffectSSAOMaxDarknessClampEnabled; }
bool Render::IsPostEffectSSGIEnabled() const { return m_postEffectSSGIEnabled; }
bool Render::IsPostEffectSSGIBlurEnabled() const { return m_postEffectSSGIBlurEnabled; }
bool Render::IsPostEffectSSGISeparableBlurEnabled() const { return m_postEffectSSGISeparableBlurEnabled; }
int Render::GetPostEffectSSGISampleCount() const { return m_postEffectSSGISampleCount; }
bool Render::IsPostEffectSSGIDepthScaledSampleDistanceEnabled() const { return m_postEffectSSGIDepthScaledSampleDistanceEnabled; }
float Render::GetPostEffectSSGISampleRadius() const { return m_postEffectSSGISampleRadius; }
int Render::GetPostEffectSSGIBlurKernelSize() const { return m_postEffectSSGIBlurKernelSize; }
int Render::GetPostEffectSSGITexSizeDivisor() const { return m_postEffectSSGITexSizeDivisor; }
float Render::GetPostEffectSSGIIndirectLightStrength() const { return m_postEffectSSGIIndirectLightStrength; }
float Render::GetPostEffectSSGIIndirectLightMaxContribution() const { return m_postEffectSSGIIndirectLightMaxContribution; }
bool Render::IsPostEffectSSGIUseThicknessEnabled() const { return m_postEffectSSGIUseThicknessEnabled; }
bool Render::IsPostEffectFogEnabled() const { return m_postEffectFogZEnabled; }
float Render::GetPostEffectFogIntensity() const { return m_postEffectFogIntensity; }
D3DXCOLOR Render::GetPostEffectFogColor() const { return m_postEffectFogColor; }
bool Render::IsPostEffectHeightFogEnabled() const { return m_postEffectFogHeightEnabled; }
float Render::GetPostEffectHeightFogIntensity() const { return m_postEffectHeightFogIntensity; }
float Render::GetPostEffectHeightFogStart() const { return m_postEffectHeightFogStart; }
float Render::GetPostEffectHeightFogMax() const { return m_postEffectHeightFogMax; }
float Render::GetPostEffectHeightFogDistanceStart() const { return m_postEffectHeightFogDistanceStart; }
float Render::GetPostEffectHeightFogDistanceMax() const { return m_postEffectHeightFogDistanceMax; }
bool Render::IsPostEffectBloomEnabled() const { return m_postEffectBloomEnabled; }
float Render::GetPostEffectBloomThreshold() const { return m_postEffectBloomThreshold; }
float Render::GetPostEffectBloomWeightSum() const { return m_postEffectBloomWeightSum; }
bool Render::IsPostEffectHaloEnabled() const { return m_postEffectHaloEnabled; }
float Render::GetPostEffectHaloThreshold() const { return m_postEffectHaloThreshold; }
DepthOfFieldMode Render::GetPostEffectDepthOfFieldMode() const { return m_postEffectDepthOfFieldMode; }
float Render::GetPostEffectDepthOfFieldFocalDistance() const { return m_postEffectDepthOfFieldFocalDistance; }
float Render::GetPostEffectDepthOfFieldStartNear() const { return m_postEffectDepthOfFieldStartNear; }
float Render::GetPostEffectDepthOfFieldMaxBlurDistance() const { return m_postEffectDepthOfFieldMaxBlurDistance; }
float Render::GetPostEffectDepthOfFieldAutoActivationDistance() const { return m_postEffectDepthOfFieldAutoActivationDistance; }
bool Render::IsPostEffectStarBurstEnabled() const { return m_postEffectStarBurstEnabled; }
float Render::GetPostEffectStarBurstThreshold() const { return m_postEffectStarBurstThreshold; }
float Render::GetPostEffectStarBurstDistanceFade() const { return m_postEffectStarBurstDistanceFade; }
bool Render::IsPostEffectGodRayEnabled() const { return m_postEffectGodRayEnabled; }
D3DXVECTOR3 Render::GetPostEffectGodRayLightPos() const { return m_postEffectGodRayLightPos; }
float Render::GetPostEffectGodRayIntensity() const { return m_postEffectGodRayIntensity; }
float Render::GetPostEffectGodRayVirtualProximityStrength() const { return m_postEffectGodRayVirtualProximityStrength; }
D3DXVECTOR3 Render::GetPostEffectGodRayLightColor() const { return m_postEffectGodRayLightColor; }
DebugGBufferView Render::GetDebugGBufferView() const { return m_debugGBufferView; }

void Render::SetShowFPS(const bool arg)
{
    m_bShowFPS = arg;
}

bool Render::IsShowFPS() const
{
    return m_bShowFPS;
}

void Render::SetShowCameraPosition(const bool arg)
{
    m_bShowCameraPosition = arg;
}

std::vector<std::pair<int, int>> Render::GetResolutionList()
{
    return m_windowManager.GetResolutionList();
}

void Render::SetLightDir(const D3DXVECTOR3& dir)
{
    D3DXVECTOR4 normal(dir, 0.f);
    D3DXVec4Normalize(&normal, &normal);
    Light::SetLightDir(normal);
}

void Render::SetLightColor(const D3DXCOLOR& color)
{
    Light::SetLightColor(color);
}

void Render::SetLightBrightness(const float brightness)
{
    Light::SetBrightness(brightness);
}

void Render::SetAmbientLightColor(const D3DXCOLOR& color)
{
    Light::SetAmbientColor(color);
}

void Render::SetAmbientLightBrightness(const float brightness)
{
    Light::SetAmbientBrightness(brightness);
}

D3DXCOLOR Render::GetLightColor() const
{
    return Light::GetLightColor();
}

float Render::GetLightBrightness() const
{
    return Light::GetBrightness();
}

D3DXCOLOR Render::GetAmbientLightColor() const
{
    return Light::GetAmbientColor();
}

float Render::GetAmbientLightBrightness() const
{
    return Light::GetAmbientBrightness();
}

void Render::AddPointLight(const D3DXVECTOR3& pos,
                           const float brightness,
                           const D3DXCOLOR color,
                           const PointLightShape shape,
                           const float lineLength,
                           const float squareWidth,
                           const float squareHeight,
                           const D3DXVECTOR3& rotation)
{
    Light::AddPointLight(pos, color, brightness, shape, lineLength, squareWidth, squareHeight, rotation);
}

void Render::PlaceParticleEffect(const ParticleEffectPreset preset, const D3DXVECTOR3& origin)
{
    m_particleSystem.PlaceEffect(preset, origin);
}

void Render::ClearParticleEffect()
{
    m_particleSystem.ClearEffect();
}

void Render::SetDustFixedScreenSize(const bool enabled)
{
    m_particleSystem.SetDustFixedScreenSize(enabled);
}

ParticleEffectPreset Render::GetParticleEffectPreset() const
{
    return m_particleSystem.GetPreset();
}

void Render::RotateCamera(const D3DXVECTOR3& rot)
{
    D3DXVECTOR3 lookAt = Camera::GetLookAtPos();
    D3DXVECTOR3 eye = Camera::GetEyePos();

    // 注視点から見た相対位置
    D3DXVECTOR3 rel = eye - lookAt;

    // 現在の距離
    float r = D3DXVec3Length(&rel);

    // 現在の角度を求める（spherical座標）

    // 水平方向
    float yaw = atan2f(rel.x, rel.z);

    // 上下方向
    float pitch = asinf(rel.y / r);

    // 回転を加える
    yaw += rot.y;
    pitch += rot.x;

    //---------------------------------------------------------
    // ピッチ角を制限する
    //---------------------------------------------------------

    // 真上/真下を少し手前で止める
    const float limit = D3DXToRadian(89.0f);
    if (pitch > limit)
    {
        pitch = limit;
    }

    if (pitch < -limit)
    {
        pitch = -limit;
    }

    // 極座標 → デカルト座標に戻す
    D3DXVECTOR3 newRel;
    newRel.x = r * cosf(pitch) * sinf(yaw);
    newRel.y = r * sinf(pitch);
    newRel.z = r * cosf(pitch) * cosf(yaw);

    // 新しいeye位置をセット
    D3DXVECTOR3 newEye = lookAt + newRel;

    Camera::SetEyePos(newEye);
    Camera::SetLookAtPos(lookAt);
}

int Render::FindActiveMirrorMeshIndex() const
{
    for (int i = static_cast<int>(m_meshMixList.size()) - 1; i >= 0; --i)
    {
        const auto& mesh = m_meshMixList[static_cast<size_t>(i)];
        if (mesh.IsEnabled() && mesh.IsLoaded() && mesh.IsMirror())
        {
            return i;
        }
    }

    return -1;
}

bool Render::RenderMirrorTexture(const int activeMirrorMeshIndex)
{
    if (activeMirrorMeshIndex < 0 ||
        activeMirrorMeshIndex >= static_cast<int>(m_meshMixList.size()) ||
        m_pMirrorRenderTarget == NULL)
    {
        return false;
    }

    D3DXVECTOR3 planePoint;
    D3DXVECTOR3 planeNormal;
    if (!m_meshMixList[static_cast<size_t>(activeMirrorMeshIndex)].TryGetMirrorPlaneWorld(planePoint, planeNormal))
    {
        return false;
    }

    HRESULT hResult = E_FAIL;
    LPDIRECT3DSURFACE9 oldRenderTarget = NULL;
    LPDIRECT3DSURFACE9 oldDepthStencil = NULL;
    LPDIRECT3DSURFACE9 mirrorRenderSurface = NULL;

    hResult = Common::D3DDevice()->GetRenderTarget(0, &oldRenderTarget);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->GetDepthStencilSurface(&oldDepthStencil);
    assert(hResult == S_OK);

    hResult = m_pMirrorRenderTarget->GetSurfaceLevel(0, &mirrorRenderSurface);
    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->SetRenderTarget(0, mirrorRenderSurface);
    assert(hResult == S_OK);

    const D3DXVECTOR3 eye = Camera::GetEyePos();
    const D3DXVECTOR3 target = Camera::GetLookAtPos();
    D3DXPLANE mirrorPlane(planeNormal.x,
                          planeNormal.y,
                          planeNormal.z,
                          -D3DXVec3Dot(&planeNormal, &planePoint));
    D3DXMATRIX matReflection;
    D3DXMatrixReflect(&matReflection, &mirrorPlane);

    D3DXVECTOR3 clipNormal = planeNormal;
    const D3DXVECTOR3 eyeToPlane = eye - planePoint;
    if (D3DXVec3Dot(&clipNormal, &eyeToPlane) < 0.0f)
    {
        clipNormal *= -1.0f;
    }

    const D3DXVECTOR4 clipPlane(clipNormal.x,
                                clipNormal.y,
                                clipNormal.z,
                                -D3DXVec3Dot(&clipNormal, &planePoint));

    D3DXVECTOR3 reflectedEye;
    D3DXVECTOR3 reflectedTarget;
    D3DXVec3TransformCoord(&reflectedEye, &eye, &matReflection);
    D3DXVec3TransformCoord(&reflectedTarget, &target, &matReflection);

    const D3DXVECTOR3 originalEye = eye;
    const D3DXVECTOR3 originalLookAt = target;
    Camera::SetEyePos(reflectedEye);
    Camera::SetLookAtPos(reflectedTarget);

    const D3DXMATRIX mirrorViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    MeshMixManager::SetSharedMirrorViewProj(mirrorViewProj);

    MeshMixManager::SetSharedMirrorClipPlane(true, clipPlane);
    MeshMixSkinAnim::SetSharedMirrorClipPlane(true, clipPlane);

    DrawPass1(false, activeMirrorMeshIndex);

    const D3DXVECTOR4 disabledClipPlane(0.0f, 1.0f, 0.0f, 0.0f);
    MeshMixManager::SetSharedMirrorClipPlane(false, disabledClipPlane);
    MeshMixSkinAnim::SetSharedMirrorClipPlane(false, disabledClipPlane);

    Camera::SetEyePos(originalEye);
    Camera::SetLookAtPos(originalLookAt);

    hResult = Common::D3DDevice()->SetRenderTarget(0, oldRenderTarget);
    assert(hResult == S_OK);
    hResult = Common::D3DDevice()->SetDepthStencilSurface(oldDepthStencil);
    assert(hResult == S_OK);

    SAFE_RELEASE(mirrorRenderSurface);
    SAFE_RELEASE(oldDepthStencil);
    SAFE_RELEASE(oldRenderTarget);
    return true;
}

void Render::DrawSceneGeometry(const int activeMirrorMeshIndex,
                               const bool renderActiveMirrorAsMirror,
                               const int skippedMeshMixIndex)
{
    for (size_t i = 0; i < m_meshList.size(); ++i)
    {
        if (i < m_meshEnabledList.size() && m_meshEnabledList[i])
        {
            m_meshList[i].Render();
        }
    }

    for (auto& elem : m_meshSmoothList)
    {
        elem.Draw();
    }

    for (auto& elem : m_meshSSSLikeList)
    {
        elem.Draw();
    }

    for (size_t i = 0; i < m_meshSSSList.size(); ++i)
    {
        if (i < m_meshSSSEnabledList.size() && m_meshSSSEnabledList[i])
        {
            m_meshSSSList[i].Render();
        }
    }

    for (size_t i = 0; i < m_meshPointLightList.size(); ++i)
    {
        if (i < m_meshPointLightEnabledList.size() && m_meshPointLightEnabledList[i])
        {
            m_meshPointLightList[i].Draw();
        }
    }

    for (size_t i = 0; i < m_meshNormalMapList.size(); ++i)
    {
        if (i < m_meshNormalMapEnabledList.size() && m_meshNormalMapEnabledList[i])
        {
            m_meshNormalMapList[i].Draw();
        }
    }

    for (size_t i = 0; i < m_meshPOMList.size(); ++i)
    {
        if (i < m_meshPOMEnabledList.size() && m_meshPOMEnabledList[i])
        {
            m_meshPOMList[i].Draw();
        }
    }

    for (auto& elem : m_animMeshList)
    {
        if (elem != nullptr)
        {
            elem->Render();
        }
    }

    for (auto& elem : m_skinAnimMeshList)
    {
        if (elem != nullptr)
        {
            elem->Render(Camera::GetViewMatrix(),
                         Camera::GetProjMatrix(),
                         Light::GetLightDir(),
                         Light::GetBrightness());
        }
    }

    for (auto& elem : m_meshPBRList)
    {
        elem.Render();
    }

    const D3DXVECTOR3 eyePos = Camera::GetEyePos();
    std::vector<size_t> transparentWaterMeshIndices;
    for (size_t i = 0; i < m_meshMixList.size(); ++i)
    {
        if (static_cast<int>(i) == skippedMeshMixIndex)
        {
            continue;
        }

        if (m_meshMixList[i].UsesWaterTextureAlpha())
        {
            transparentWaterMeshIndices.push_back(i);
            continue;
        }

        const bool renderAsMirror =
            renderActiveMirrorAsMirror &&
            static_cast<int>(i) == activeMirrorMeshIndex;
        m_meshMixList[i].Render(renderAsMirror);
    }

    for (auto& elem : m_meshMixSkinAnimList)
    {
        if (elem != nullptr)
        {
            elem->Render();
        }
    }

    for (auto& elem : m_meshInstancingMap)
    {
        elem.second->Draw();
    }

    std::stable_sort(transparentWaterMeshIndices.begin(),
                     transparentWaterMeshIndices.end(),
                     [&](const size_t lhs, const size_t rhs)
                     {
                         const D3DXVECTOR3 lhsDelta = m_meshMixList[lhs].GetPos() - eyePos;
                         const D3DXVECTOR3 rhsDelta = m_meshMixList[rhs].GetPos() - eyePos;
                         const float lhsDistanceSq = D3DXVec3LengthSq(&lhsDelta);
                         const float rhsDistanceSq = D3DXVec3LengthSq(&rhsDelta);
                         return lhsDistanceSq > rhsDistanceSq;
                     });

    for (const size_t index : transparentWaterMeshIndices)
    {
        m_meshMixList[index].Render(false);
    }

    m_particleSystem.Draw(Camera::GetViewMatrix(), Camera::GetProjMatrix());
}

void Render::DrawPass1(const bool renderToSceneRenderTargets, const int activeMirrorMeshIndex)
{
    HRESULT hResult = E_FAIL;

    LPDIRECT3DSURFACE9 surfaceOld = NULL;
    LPDIRECT3DSURFACE9 surfaceRenderTarget0 = NULL;
    LPDIRECT3DSURFACE9 surfaceRenderTarget1 = NULL;

    if (renderToSceneRenderTargets)
    {
        hResult = Common::D3DDevice()->GetRenderTarget(0, &surfaceOld);
        assert(hResult == S_OK);

        hResult = m_pRenderTarget1->GetSurfaceLevel(0, &surfaceRenderTarget0);
        assert(hResult == S_OK);

        hResult = m_pRenderTarget2->GetSurfaceLevel(0, &surfaceRenderTarget1);
        assert(hResult == S_OK);

        hResult = Common::D3DDevice()->SetRenderTarget(0, surfaceRenderTarget0);
        assert(hResult == S_OK);

        hResult = Common::D3DDevice()->SetRenderTarget(1, surfaceRenderTarget1);
        assert(hResult == S_OK);
    }
    else
    {
        hResult = Common::D3DDevice()->SetRenderTarget(1, NULL);
        assert(hResult == S_OK);
    }

    hResult = Common::D3DDevice()->Clear(0,
                                         NULL,
                                         D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                         D3DCOLOR_RGBA(200, 200, 200, 255),
                                         1.0f,
                                         0);

    assert(hResult == S_OK);

    hResult = Common::D3DDevice()->BeginScene();
    assert(hResult == S_OK);

    DrawSceneGeometry(activeMirrorMeshIndex, renderToSceneRenderTargets, renderToSceneRenderTargets ? -1 : activeMirrorMeshIndex);

    hResult = Common::D3DDevice()->EndScene();
    assert(hResult == S_OK);

    if (renderToSceneRenderTargets)
    {
        hResult = Common::D3DDevice()->SetRenderTarget(1, NULL);
        assert(hResult == S_OK);

        hResult = Common::D3DDevice()->SetRenderTarget(0, surfaceOld);
        assert(hResult == S_OK);
    }

    SAFE_RELEASE(surfaceRenderTarget0);
    SAFE_RELEASE(surfaceRenderTarget1);
    SAFE_RELEASE(surfaceOld);
}

float Render::CalcFPS()
{
    //--------------------------------------------------------
    // 毎フレーム、現在時刻を記録し、リストの末尾に追加する。
    // 先頭ほど古い時刻が記録される。
    // リストの先頭から、記録された時刻と現在時刻の差を比較していくと、
    // 最初は1秒以上の差があるが、やがて1秒以下の要素が見つかる。（Aとする）
    // そのときの、A以降の要素の総数がFPSである
    //--------------------------------------------------------

    using ClockType = std::chrono::steady_clock;

    const int timeRecordCapacity = 300;

    ClockType::time_point nowTime = ClockType::now();

    m_vecTime.push_back(nowTime);

    const int overflowCount = (int)m_vecTime.size() - timeRecordCapacity;
    if (overflowCount > 0)
    {
        m_vecTime.erase(m_vecTime.begin(), m_vecTime.begin() + overflowCount);
    }

    ClockType::time_point oneSecondAgo = nowTime - std::chrono::seconds(1);

    auto windowBegin = std::lower_bound(m_vecTime.begin(), m_vecTime.end(), oneSecondAgo);
    int framesInWindow = (int)std::distance(windowBegin, m_vecTime.end());

    if (framesInWindow <= 1)
    {
        return 0.0f;
    }

    double secondsSpan = std::chrono::duration<double>(m_vecTime.back() - *windowBegin).count();
    if (secondsSpan <= 0.0)
    {
        return 0.0f;
    }

    float fpsFloat = (float)((framesInWindow - 1) / secondsSpan);
    return fpsFloat;
}

void Render::ShowFPS(const float arg)
{
    if (m_fontID == -1)
    {
        m_fontID = SetUpFont(L"BIZ UDゴシック",
                             20,
                             D3DCOLOR_RGBA(0, 255, 0, 255));
    }

    wchar_t buffer[64];
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%.2f (sleep %ums)", arg, m_lastSleepMs);

    std::wstring fps(buffer);

    DrawText_(m_fontID, fps, 10, 10);
}

void Render::ShowCameraPosition()
{
    if (m_cameraPositionFontId == -1)
    {
        m_cameraPositionFontId = SetUpFont(L"BIZ UDゴシック",
                                           20,
                                           D3DCOLOR_RGBA(255, 255, 255, 255));
    }

    const D3DXVECTOR3 cameraPos = Camera::GetEyePos();

    wchar_t buffer[128];
    std::swprintf(buffer,
                  sizeof(buffer) / sizeof(buffer[0]),
                  L"X: %.2f  Y: %.2f  Z: %.2f",
                  cameraPos.x,
                  cameraPos.y,
                  cameraPos.z);

    const int y = m_bShowFPS ? 34 : 10;
    DrawText_(m_cameraPositionFontId, buffer, 10, y);
}

float Render::CalcFrameDeltaSeconds()
{
    using ClockType = std::chrono::steady_clock;
    const ClockType::time_point now = ClockType::now();

    if (!m_hasLastFrameTime)
    {
        m_lastFrameTime = now;
        m_hasLastFrameTime = true;
        return 1.0f / 60.0f;
    }

    const float deltaSeconds =
        static_cast<float>(std::chrono::duration<double>(now - m_lastFrameTime).count());
    m_lastFrameTime = now;
    return (std::max)(0.0f, (std::min)(deltaSeconds, 0.033f));
}

void Render::WaitForTargetFrameRate()
{
    using ClockType = std::chrono::steady_clock;
    const ClockType::time_point now = ClockType::now();

    m_lastSleepMs = 0;

    if (!m_hasLastFramePacingTime)
    {
        m_lastFramePacingTime = now;
        m_hasLastFramePacingTime = true;
        return;
    }

    ClockType::time_point currentTime = now;
    std::chrono::duration<double> elapsed = currentTime - m_lastFramePacingTime;
    while (elapsed < kTargetFrameDuration)
    {
        const std::chrono::duration<double> remaining = kTargetFrameDuration - elapsed;
        const DWORD sleepMilliseconds = (remaining > std::chrono::milliseconds(2))
                                            ? static_cast<DWORD>(
                                                  std::chrono::duration_cast<std::chrono::milliseconds>(
                                                      remaining - std::chrono::milliseconds(1))
                                                      .count())
                                            : 0;
        Sleep(sleepMilliseconds);
        if (sleepMilliseconds > 0)
        {
            m_lastSleepMs = sleepMilliseconds;
        }
        currentTime = ClockType::now();
        elapsed = currentTime - m_lastFramePacingTime;
    }

    m_lastFramePacingTime = currentTime;
}

void Render::DrawWorldTexts()
{
    if (m_worldTextList.empty())
    {
        return;
    }

    const float scaleX = static_cast<float>(Common::BASE_W) / static_cast<float>(Common::ScreenW());
    const float scaleY = static_cast<float>(Common::BASE_H) / static_cast<float>(Common::ScreenH());

    for (const auto& worldText : m_worldTextList)
    {
        const POINT screenPos = Camera::GetScreenPos(worldText.worldPos);
        if (screenPos.x < 0 || screenPos.y < 0)
        {
            continue;
        }

        const int baseX = static_cast<int>(static_cast<float>(screenPos.x) * scaleX);
        const int baseY = static_cast<int>(static_cast<float>(screenPos.y) * scaleY);

        SIZE textSize = { 0, 0 };
        if (worldText.decorated)
        {
            auto found = m_worldTextFontExIdBySize.find(worldText.fontSize);
            if (found == m_worldTextFontExIdBySize.end())
            {
                const int newId = SetUpFontEx(L"BIZ UDゴシック", worldText.fontSize, D3DCOLOR_ARGB(255, 255, 255, 255));
                m_worldTextFontExIdBySize[worldText.fontSize] = newId;
                found = m_worldTextFontExIdBySize.find(worldText.fontSize);
            }

            textSize = m_fontExList.at(found->second)->GetTextSize(worldText.text);

            const UINT drawColor = D3DCOLOR_ARGB(
                static_cast<int>(worldText.color.a * 255.0f),
                static_cast<int>(worldText.color.r * 255.0f),
                static_cast<int>(worldText.color.g * 255.0f),
                static_cast<int>(worldText.color.b * 255.0f));

            const int centerX = baseX - textSize.cx / 2;
            const int centerY = baseY - textSize.cy / 2;

            DrawTextEx(found->second, worldText.text, centerX, centerY, drawColor);
        }
        else
        {
            auto found = m_worldTextFontIdBySize.find(worldText.fontSize);
            if (found == m_worldTextFontIdBySize.end())
            {
                const int newId = SetUpFont(L"BIZ UDゴシック", worldText.fontSize, D3DCOLOR_ARGB(255, 255, 255, 255));
                m_worldTextFontIdBySize[worldText.fontSize] = newId;
                found = m_worldTextFontIdBySize.find(worldText.fontSize);
            }

            textSize = m_fontList.at(found->second)->GetTextSize(worldText.text);

            const UINT drawColor = D3DCOLOR_ARGB(
                static_cast<int>(worldText.color.a * 255.0f),
                static_cast<int>(worldText.color.r * 255.0f),
                static_cast<int>(worldText.color.g * 255.0f),
                static_cast<int>(worldText.color.b * 255.0f));

            const int centerX = baseX - textSize.cx / 2;
            const int centerY = baseY - textSize.cy / 2;

            DrawText_(found->second, worldText.text, centerX, centerY, drawColor);
        }
    }
}

void Render::Draw2D()
{
    if (m_bShowCameraPosition)
    {
        ShowCameraPosition();
    }

    m_sprite.Draw();

    DrawSettingsDialogText();
    DrawWorldTexts();

    for (auto& elem : m_fontList)
    {
        elem->Draw();
    }

    for (auto& elem : m_fontExList)
    {
        elem->Draw();
    }

    DrawFadeOverlay();
    DrawLoadingScreen();
}

void Render::EnsureFadeTexture()
{
    if (m_fadeTexture != NULL)
    {
        return;
    }

    HRESULT hr = Common::D3DDevice()->CreateTexture(
        2,
        2,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &m_fadeTexture,
        NULL);
    if (FAILED(hr) || m_fadeTexture == NULL)
    {
        return;
    }

    D3DLOCKED_RECT lockedRect;
    hr = m_fadeTexture->LockRect(0, &lockedRect, NULL, 0);
    if (SUCCEEDED(hr))
    {
        DWORD* pixels = static_cast<DWORD*>(lockedRect.pBits);
        for (int i = 0; i < 4; ++i)
        {
            pixels[i] = D3DCOLOR_ARGB(255, 0, 0, 0);
        }
        m_fadeTexture->UnlockRect(0);
    }

    m_sprite.RegisterTexture(L"__fade_black__", m_fadeTexture);
}

void Render::UpdateFade(const float deltaSeconds)
{
    if (!m_fadeActive)
    {
        return;
    }

    m_fadeElapsed += deltaSeconds;
    if (m_fadeElapsed >= m_fadeDuration)
    {
        m_fadeAlpha = m_fadeTargetAlpha;
        m_fadeActive = false;
        return;
    }

    const float t = m_fadeElapsed / m_fadeDuration;
    m_fadeAlpha = m_fadeStartAlpha + (m_fadeTargetAlpha - m_fadeStartAlpha) * t;
}

void Render::StartFadeIn(const float durationSeconds)
{
    if (durationSeconds <= 0.0f)
    {
        m_fadeAlpha = 0.0f;
        m_fadeActive = false;
        return;
    }

    m_fadeDuration = durationSeconds;
    m_fadeStartAlpha = 1.0f;
    m_fadeTargetAlpha = 0.0f;
    m_fadeElapsed = 0.0f;
    m_fadeActive = true;
}

void Render::StartFadeOut(const float durationSeconds)
{
    if (durationSeconds <= 0.0f)
    {
        m_fadeAlpha = 1.0f;
        m_fadeActive = false;
        return;
    }

    m_fadeDuration = durationSeconds;
    m_fadeStartAlpha = 0.0f;
    m_fadeTargetAlpha = 1.0f;
    m_fadeElapsed = 0.0f;
    m_fadeActive = true;
}

void Render::SetFadeAlpha(const float alpha)
{
    float clamped = alpha;
    if (clamped < 0.0f)
    {
        clamped = 0.0f;
    }
    if (clamped > 1.0f)
    {
        clamped = 1.0f;
    }
    m_fadeAlpha = clamped;
    m_fadeActive = false;
}

float Render::GetFadeAlpha() const
{
    return m_fadeAlpha;
}

void Render::StartLoadingScreen()
{
    m_loadingScreen.Start();
}

void Render::EndLoadingScreen()
{
    m_loadingScreen.SetProgress(100);
    m_loadingScreen.End();
}

void Render::SetLoadingScreenTitle(const std::wstring& title)
{
    m_loadingScreen.SetTitle(title);
}

void Render::SetLoadingScreenTitleFontPath(const std::wstring& fontPath)
{
    if (m_loadingScreenTitleFontRegistered)
    {
        RemoveFontResourceExW(m_loadingScreenTitleFontPath.c_str(), FR_PRIVATE, NULL);
        m_loadingScreenTitleFontRegistered = false;
    }

    if (PathIsRelative(fontPath.c_str()))
    {
        m_loadingScreenTitleFontPath = Util::GetExeDir() + fontPath;
    }
    else
    {
        m_loadingScreenTitleFontPath = fontPath;
    }

    m_loadingScreenTitleFontName = L"BIZ UDMincho";
    m_loadingScreenTitleFontId = -1;
}

void Render::SetLoadingScreenProgress(const int progress)
{
    m_loadingScreen.SetProgress(progress);
}

void Render::SetLoadingScreenShowTitle(const bool show)
{
    m_loadingScreen.SetShowTitle(show);
}

void Render::EnsureLoadingScreenFont()
{
    if (m_loadingScreenFontId < 0)
    {
        m_loadingScreenFontId = SetUpFont(L"BIZ UDゴシック",
                                          20,
                                          D3DCOLOR_RGBA(255, 255, 255, 255));
    }

    if (m_loadingScreenTitleFontId < 0)
    {
        if (!m_loadingScreenTitleFontPath.empty() && !m_loadingScreenTitleFontRegistered)
        {
            const int addedFontCount = AddFontResourceExW(m_loadingScreenTitleFontPath.c_str(), FR_PRIVATE, NULL);
            if (addedFontCount > 0)
            {
                m_loadingScreenTitleFontRegistered = true;
            }
            else
            {
                m_loadingScreenTitleFontName = L"BIZ UDゴシック";
            }
        }

        m_loadingScreenTitleFontId = SetUpFont(m_loadingScreenTitleFontName,
                                               50,
                                               D3DCOLOR_RGBA(255, 255, 255, 255));
    }
}

void Render::DrawLoadingScreen()
{
    if (!m_loadingScreen.IsVisible())
    {
        return;
    }

    EnsureLoadingScreenFont();
    if (m_loadingScreenFontId < 0 || m_loadingScreenTitleFontId < 0)
    {
        return;
    }

    m_loadingScreen.Draw(m_sprite,
                         *m_fontList.at(m_loadingScreenFontId),
                         *m_fontList.at(m_loadingScreenTitleFontId));
}

void Render::DrawFadeOverlay()
{
    if (m_fadeAlpha <= 0.0f)
    {
        return;
    }

    const int alpha255 = static_cast<int>(m_fadeAlpha * 255.0f);
    if (alpha255 <= 0)
    {
        return;
    }

    EnsureFadeTexture();
    if (m_fadeTexture == NULL)
    {
        return;
    }

    const float screenW = static_cast<float>(Common::ScreenW());
    const float screenH = static_cast<float>(Common::ScreenH());
    const float baseW = static_cast<float>(Common::BASE_W);
    const float baseH = static_cast<float>(Common::BASE_H);

    const int w = static_cast<int>(baseW);
    const int h = static_cast<int>(baseH);

    m_sprite.PlaceImage(L"__fade_black__", 0, 0, w, h, alpha255);
    m_sprite.Draw();
}

void Render::EnsureSettingsDialogTextFonts()
{
    if (m_settingsDialogTextFontId < 0)
    {
        m_settingsDialogTextFontId = SetUpFont(L"BIZ UDゴシック", 20, D3DCOLOR_RGBA(255, 255, 255, 255));
    }
    if (m_settingsDialogTextFontExId < 0)
    {
        m_settingsDialogTextFontExId = SetUpFontEx(L"BIZ UDゴシック", 20, D3DCOLOR_RGBA(255, 255, 255, 255));
    }
}

void Render::DrawSettingsDialogText()
{
    if (m_settingsDialogTextList.empty())
    {
        return;
    }

    EnsureSettingsDialogTextFonts();
    for (const auto& textInfo : m_settingsDialogTextList)
    {
        if (textInfo.decorated)
        {
            DrawTextExNormalized(m_settingsDialogTextFontExId, textInfo.text, textInfo.x, textInfo.y);
        }
        else
        {
            DrawTextNormalized(m_settingsDialogTextFontId, textInfo.text, textInfo.x, textInfo.y);
        }
    }
}

void Render::OnDeviceLost()
{
    SAFE_RELEASE(m_pRenderTarget1);
    SAFE_RELEASE(m_pRenderTarget2);
    SAFE_RELEASE(m_pLightEffectSourceTexture);
    SAFE_RELEASE(m_pMirrorRenderTarget);
    m_sprite.OnDeviceLost();
    m_particleSystem.OnDeviceLost();
}

void Render::OnDeviceReset()
{
    CreateTexture();
    m_sprite.OnDeviceReset();
    m_particleSystem.OnDeviceReset();
}

void Render::CreateTexture()
{
    HRESULT hr = E_FAIL;

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A16B16G16R16F,
                           //D3DFMT_A8R8G8B8,
                           D3DPOOL_DEFAULT,
                           &m_pRenderTarget1);
    assert(hr == S_OK);

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A16B16G16R16F,
                           D3DPOOL_DEFAULT,
                           &m_pRenderTarget2);
    assert(hr == S_OK);

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A16B16G16R16F,
                           D3DPOOL_DEFAULT,
                           &m_pLightEffectSourceTexture);
    assert(hr == S_OK);

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A16B16G16R16F,
                           D3DPOOL_DEFAULT,
                           &m_pMirrorRenderTarget);
    assert(hr == S_OK);

}

}
