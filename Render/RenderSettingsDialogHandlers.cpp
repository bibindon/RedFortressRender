#include "RenderSettingsDialogInternal.h"
namespace NSRender
{
namespace RenderSettingsDialogInternal
{
namespace
{
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

bool ResolveXFileListPath(const std::wstring& csvDirectoryPath,
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

MeshMixSkinAnimLoadMode GetMeshMixSkinAnimLoadMode(const RenderSettingsDialogState* state)
{
    if (state != nullptr && state->useCustomMeshMixSkinAnimLoader)
    {
        return MeshMixSkinAnimLoadMode::Custom;
    }

    return MeshMixSkinAnimLoadMode::DirectX;
}

bool LoadXFileListCsv(RenderSettingsDialogState* state,
                      const std::wstring& csvPath,
                      int& loadedCount,
                      int& skippedCount)
{
    loadedCount = 0;
    skippedCount = 0;
    if (state == nullptr || state->render == nullptr || csvPath.empty())
    {
        return false;
    }
    std::wifstream file(csvPath);
    if (!file)
    {
        return false;
    }
    const std::wstring csvDirectoryPath = GetCsvParentDirectoryPath(csvPath);
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
            ++skippedCount;
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
            if (!ResolveXFileListPath(csvDirectoryPath, fields[1], resolvedPath))
            {
                ++skippedCount;
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
            RenderSettingsDialogState::LoadedModelType modelType = RenderSettingsDialogState::LoadedModelType::MeshMix;

            if (loadType == L"instancing")
            {
                renderId = state->render->AddMeshInstansing(resolvedPath, pos, rot, modelScale);
                modelType = RenderSettingsDialogState::LoadedModelType::MeshInstancing;
            }
            else if (loadType == L"skinanim")
            {
                AnimSetMap emptyAnimSetMap;
                renderId = state->render->AddMeshMixSkinAnim(resolvedPath,
                                                             pos,
                                                             rot,
                                                             modelScale,
                                                             emptyAnimSetMap,
                                                             -1.0f,
                                                             false,
                                                             false,
                                                             GetMeshMixSkinAnimLoadMode(state));
                modelType = RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim;
            }
            else
            {
                renderId = state->render->AddMeshMix(resolvedPath, pos, rot, modelScale);
                modelType = RenderSettingsDialogState::LoadedModelType::MeshMix;
            }

            AddLoadedModelRecord(state,
                                 modelType,
                                 renderId,
                                 resolvedPath,
                                 pos);

            const int csvId = std::stoi(TrimCsvField(fields[0]));
            state->render->RegisterCsvIdMapping(csvId, renderId);

            ++loadedCount;
        }
        catch (...)
        {
            ++skippedCount;
        }
    }
    UpdateLoadedModelsList(state);
    return loadedCount > 0;
}
}

BOOL CALLBACK CaptureRenderSettingsChildPlacementProc(HWND child, LPARAM lParam)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(lParam);
    if (state == nullptr)
    {
        return TRUE;
    }
    RECT rect { };
    GetWindowRect(child, &rect);
    POINT topLeft { rect.left, rect.top };
    POINT bottomRight { rect.right, rect.bottom };
    HWND parent = GetParent(child);
    ScreenToClient(parent, &topLeft);
    ScreenToClient(parent, &bottomRight);
    RenderSettingsDialogState::ChildPlacement placement;
    placement.hWnd = child;
    placement.rect.left = topLeft.x;
    placement.rect.top = topLeft.y;
    placement.rect.right = bottomRight.x;
    placement.rect.bottom = bottomRight.y;
    state->childPlacements.push_back(placement);
    return TRUE;
}
void CaptureRenderSettingsChildPlacements(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }
    state->childPlacements.clear();
    EnumChildWindows(hWnd, CaptureRenderSettingsChildPlacementProc, reinterpret_cast<LPARAM>(state));
    int maxBottom = 0;
    for (const auto& placement : state->childPlacements)
    {
        maxBottom = (std::max)(maxBottom, static_cast<int>(placement.rect.bottom));
    }
    state->contentHeight = maxBottom + RENDER_SETTINGS_CONTENT_BOTTOM_MARGIN;
}
void ApplyRenderSettingsChildPositions(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }
    for (const auto& placement : state->childPlacements)
    {
        int width = placement.rect.right - placement.rect.left;
        int height = placement.rect.bottom - placement.rect.top;
        SetWindowPos(placement.hWnd,
                     NULL,
                     placement.rect.left,
                     placement.rect.top - state->scrollPos,
                     width,
                     height,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}
bool HandleRenderSettingsEditCommit(HWND hWnd, int id)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Render* render = (state != nullptr) ? state->render : nullptr;
    if (render == nullptr)
    {
        return false;
    }
    float floatValue = 0.0f;
    int intValue = 0;
    if (id >= 41100 && id <= 41111 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        int index = id - 41100;
        if (index == 0)
        {
            render->SetLightBrightness(floatValue);
            SetTrackbarFromFloat(hWnd, 31100, floatValue, 0.0f, 5.0f);
        }
        else if (index == 1)
        {
            render->SetAmbientLightBrightness(floatValue);
            SetTrackbarFromFloat(hWnd, 31101, floatValue, 0.0f, 5.0f);
        }
        else if (index == 2)
        {
            render->SetMeshMixSaturateShadowIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31102, floatValue, 0.0f, 10.0f);
        }
        else if (index == 3)
        {
            render->SetMeshMixShadowDarkness(floatValue);
            SetTrackbarFromFloat(hWnd, 31103, floatValue, 0.0f, 1.0f);
        }
        else if (index == 4)
        {
            render->SetMeshMixSpecularIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31104, floatValue, 0.0f, 2.0f);
        }
        else if (index == 5)
        {
            render->SetMeshMixSpecularEdge(floatValue);
            SetTrackbarFromFloat(hWnd, 31105, floatValue, 0.0f, 1.0f);
        }
        else if (index == 6)
        {
            render->SetMeshMixEnvMapBlend(floatValue);
            SetTrackbarFromFloat(hWnd, 31106, floatValue, 0.0f, 1.0f);
        }
        else if (index == 7)
        {
            render->SetMeshMixSSSIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31107, floatValue, 0.0f, 30.0f);
        }
        else if (index == 11)
        {
            state->modelLoadScale = floatValue;
            SetTrackbarFromFloat(hWnd, 31111, floatValue, 0.1f, 10.0f);
        }
        return true;
    }
    if (id == 41124 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetMeshMixFresnelIntensity(floatValue);
        SetTrackbarFromFloat(hWnd, 31124, floatValue, 0.0f, 2.0f);
        return true;
    }
    if (id >= 41000 && id <= 41011 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41000)
        {
            state->cameraNearPlane = floatValue;
            render->SetCameraClipPlanes(state->cameraNearPlane, state->cameraFarPlane);
        }
        else if (id == 41001)
        {
            state->cameraFarPlane = floatValue;
            render->SetCameraClipPlanes(state->cameraNearPlane, state->cameraFarPlane);
        }
        else if (id == 41003)
        {
            render->SetCameraHorizontalFovDegrees(floatValue);
            state->cameraHorizontalFovDegrees = render->GetCameraHorizontalFovDegrees();
            SetSettingsEditFloat(hWnd, 41003, state->cameraHorizontalFovDegrees, L"%.0f");
            SetTrackbarFromFloat(hWnd, 31003, state->cameraHorizontalFovDegrees, 1.0f, 180.0f);
        }
        else if (id == 41004)
        {
            state->cameraShakeDuration = floatValue;
            render->SetCameraShakeDuration(state->cameraShakeDuration);
            SetTrackbarFromFloat(hWnd, 31004, state->cameraShakeDuration, 0.1f, 5.0f);
        }
        else if (id == 41005)
        {
            state->cameraShakeIntensity = floatValue;
            render->SetCameraShakeIntensity(state->cameraShakeIntensity);
            SetTrackbarFromFloat(hWnd, 31005, state->cameraShakeIntensity, 0.0f, 1.0f);
        }
        else if (id == 41010)
        {
            state->gBufferNearPlane = floatValue;
            render->SetGBufferClipPlanes(state->gBufferNearPlane, state->gBufferFarPlane);
        }
        else if (id == 41011)
        {
            state->gBufferFarPlane = floatValue;
            render->SetGBufferClipPlanes(state->gBufferNearPlane, state->gBufferFarPlane);
        }
        return true;
    }
    if (id >= 41400 && id <= 41403 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41400) state->pointLightColor.r = floatValue;
        if (id == 41401) state->pointLightColor.g = floatValue;
        if (id == 41402) state->pointLightColor.b = floatValue;
        if (id == 41403) state->pointLightBrightness = floatValue;
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, (id == 41403) ? 100.0f : 1.0f);
        return true;
    }
    if (id >= 41200 && id <= 41205 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41200) state->lightColor.r = floatValue;
        if (id == 41201) state->lightColor.g = floatValue;
        if (id == 41202) state->lightColor.b = floatValue;
        if (id == 41203) state->ambientLightColor.r = floatValue;
        if (id == 41204) state->ambientLightColor.g = floatValue;
        if (id == 41205) state->ambientLightColor.b = floatValue;
        render->SetLightColor(state->lightColor);
        render->SetAmbientLightColor(state->ambientLightColor);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 1.0f);
        return true;
    }
    if (id >= 41500 && id <= 41505 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        int index = id - 41500;
        if (index == 0) render->SetMeshPBRRoughness(floatValue);
        if (index == 1) render->SetMeshPBRMetallic(floatValue);
        if (index == 2) render->SetMeshPBREnvReflectionIntensity(floatValue);
        if (index == 3) render->SetMeshPBREnvMaxMipLevel(floatValue);
        if (index == 4) render->SetMeshPBREnvDiffuseIntensity(floatValue);
        if (index == 5) render->SetMeshPBREnvDiffuseMipLevel(floatValue);
        const float minValue = (index == 0) ? 0.04f : 0.0f;
        const float maxValue = (index == 2 || index == 4) ? 3.0f : ((index == 3 || index == 5) ? 10.0f : 1.0f);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, minValue, maxValue);
        return true;
    }
    if (id >= 41600 && id <= 41602 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41600) render->SetPostEffectDepthBufferShadowIntensity(floatValue);
        if (id == 41601) render->SetPostEffectDepthBufferShadowSaturationBoost(floatValue);
        if (id == 41602) render->SetPostEffectDepthBufferShadowCoverage(floatValue);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 1.0f);
        return true;
    }
    if (id == 41710 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSSGIIndirectLightStrength(floatValue);
        return true;
    }
    if (id == 41711 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSSGIIndirectLightMaxContribution(floatValue);
        return true;
    }
    if (id == 41701 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSSGISampleRadius(floatValue);
        SetTrackbarFromFloat(hWnd, 31701, floatValue, 0.1f, 10.0f);
        return true;
    }
    if ((id == 41800 || id == 41803 || id == 41804) && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41800) render->SetPostEffectSSAOSampleRadius(floatValue);
        if (id == 41803) render->SetPostEffectSSAOShadowStrength(floatValue);
        if (id == 41804) render->SetPostEffectSSAOSaturationBoost(floatValue);
        const float minValue = (id == 41800) ? 0.05f : 0.0f;
        const float maxValue = (id == 41800) ? 10.0f : ((id == 41803) ? 4.0f : 5.0f);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, minValue, maxValue);
        return true;
    }
    if (id >= 41900 && id <= 41903 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41900)
        {
            render->SetPostEffectFogIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31900, floatValue, 0.0f, 20.0f);
        }
        else
        {
            if (id == 41901) state->fogColor.r = floatValue;
            if (id == 41902) state->fogColor.g = floatValue;
            if (id == 41903) state->fogColor.b = floatValue;
            render->SetPostEffectFogColor(state->fogColor);
            SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 1.0f);
        }
        return true;
    }
    if (id >= 41920 && id <= 41924 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41920) render->SetPostEffectHeightFogIntensity(floatValue);
        if (id == 41921) render->SetPostEffectHeightFogStart(floatValue);
        if (id == 41922) render->SetPostEffectHeightFogMax(floatValue);
        if (id == 41923) render->SetPostEffectHeightFogDistanceStart(floatValue);
        if (id == 41924) render->SetPostEffectHeightFogDistanceMax(floatValue);
        if (id == 41920) SetTrackbarFromFloat(hWnd, 31920, floatValue, 0.0f, 5.0f);
        if (id == 41921 || id == 41922) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, -50000.0f, 50000.0f);
        if (id == 41923 || id == 41924) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 100000.0f);
        return true;
    }
    if (id == 41940 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSaturate(floatValue);
        SetTrackbarFromFloat(hWnd, 31940, floatValue, 0.0f, 4.0f);
        return true;
    }
    if (id >= 41950 && id <= 41953 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41950) render->SetPostEffectDepthOfFieldFocalDistance(floatValue);
        if (id == 41951) render->SetPostEffectDepthOfFieldMaxBlurDistance(floatValue);
        if (id == 41952) render->SetPostEffectDepthOfFieldAutoActivationDistance(floatValue);
        if (id == 41953) render->SetPostEffectDepthOfFieldStartNear(floatValue);
        const float minValue = (id == 41953) ? 0.0f : 0.5f;
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, minValue, 50.0f);
        return true;
    }
    if ((id >= 41970 && id <= 41972 || id >= 41980 && id <= 41981) && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41970) render->SetPostEffectBloomThreshold(floatValue);
        if (id == 41971) render->SetPostEffectBloomWeightSum(floatValue);
        if (id == 41972) render->SetPostEffectHaloThreshold(floatValue);
        if (id == 41980) render->SetPostEffectStarBurstThreshold(floatValue);
        if (id == 41981) render->SetPostEffectStarBurstDistanceFade(floatValue);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, (id == 41971) ? 1.0f : 0.0f, (id == 41971) ? 100.0f : ((id == 41981) ? 1.0f : 5.0f));
        return true;
    }
    if (id >= 42000 && id <= 42007 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 42000) state->godRayColor.x = floatValue;
        if (id == 42001) state->godRayColor.y = floatValue;
        if (id == 42002) state->godRayColor.z = floatValue;
        if (id >= 42000 && id <= 42002) render->SetPostEffectGodRayLightColor(state->godRayColor);
        if (id == 42003) render->SetPostEffectGodRayIntensity(floatValue);
        if (id == 42004) render->SetPostEffectGodRayVirtualProximityStrength(floatValue);
        if (id == 42005) state->godRayPos.x = floatValue;
        if (id == 42006) state->godRayPos.y = floatValue;
        if (id == 42007) state->godRayPos.z = floatValue;
        if (id >= 42005 && id <= 42007) render->SetPostEffectGodRayLightPos(state->godRayPos);
        if (id <= 42002) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 1.0f);
        if (id == 42003) SetTrackbarFromFloat(hWnd, 32003, floatValue, 0.0f, 3.0f);
        if (id == 42004) SetTrackbarFromFloat(hWnd, 32004, floatValue, 0.0f, 5.0f);
        if (id >= 42005) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, -200.0f, 200.0f);
        return true;
    }
    if (id == 42021 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectGaussianStrength(floatValue);
        SetTrackbarFromFloat(hWnd, 32021, floatValue, 0.0f, 1.0f);
        return true;
    }
    if (id == 42101 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectTAAHistoryWeight(floatValue);
        return true;
    }
    if (id == 42110 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectMotionBlurCameraMaxBlurPixels(floatValue);
        SetTrackbarFromFloat(hWnd, 32110, floatValue, 1.0f, 64.0f);
        return true;
    }
    if (id == 42111 && TryGetSettingsEditInt(hWnd, id, intValue))
    {
        render->SetPostEffectMotionBlurCameraSampleCount(intValue);
        SetTrackbarFromInt(hWnd, 32111, intValue, 2, 21);
        return true;
    }
    if (id == 42120 && TryGetSettingsEditInt(hWnd, id, intValue))
    {
        render->SetPostEffectFXAAQuality(intValue);
        SetTrackbarFromInt(hWnd, 32120, intValue, 1, 8);
        return true;
    }
    if (id == 42130 && TryGetSettingsEditInt(hWnd, id, intValue))
    {
        render->SetPostEffectFontSampleSize(intValue);
        SetTrackbarFromInt(hWnd, 32130, intValue, 1, 21);
        return true;
    }
    if (id == 42150 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        state->settingsTextX = (std::max)(0.0f, (std::min)(floatValue, 1.0f));
        SetSettingsEditFloat(hWnd, id, state->settingsTextX);
        SetTrackbarFromFloat(hWnd, 32150, state->settingsTextX, 0.0f, 1.0f);
        const int index = GetSelectedListViewIndex(state->settingsTextList);
        if (render->SetSettingsDialogTextPosition(static_cast<size_t>(index), state->settingsTextX, state->settingsTextY))
        {
            UpdateSettingsTextList(state);
            SelectSettingsTextListItem(state, index);
        }
        return true;
    }
    if (id == 42151 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        state->settingsTextY = (std::max)(0.0f, (std::min)(floatValue, 1.0f));
        SetSettingsEditFloat(hWnd, id, state->settingsTextY);
        SetTrackbarFromFloat(hWnd, 32151, state->settingsTextY, 0.0f, 1.0f);
        const int index = GetSelectedListViewIndex(state->settingsTextList);
        if (render->SetSettingsDialogTextPosition(static_cast<size_t>(index), state->settingsTextX, state->settingsTextY))
        {
            UpdateSettingsTextList(state);
            SelectSettingsTextListItem(state, index);
        }
        return true;
    }
    return false;
}
void HandleRenderSettingsCommand(HWND hWnd, WPARAM wParam)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Render* render = (state != nullptr) ? state->render : nullptr;
    if (render == nullptr)
    {
        return;
    }
    int id = LOWORD(wParam);
    int notifyCode = HIWORD(wParam);
    if (notifyCode == EN_KILLFOCUS && HandleRenderSettingsEditCommit(hWnd, id))
    {
        return;
    }
    if (notifyCode == BN_CLICKED)
    {
        if (id == 31002)
        {
            render->SetShowFPS(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_SATURATE_ENABLE)
        {
            render->SetPostEffectSaturateEnable(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_GBUFFER_ENABLE)
        {
            render->SetGBufferEnable(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_GAUSSIAN_ENABLE)
        {
            render->SetPostEffectGaussianFilter(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_BLOOM_ENABLE)
        {
            render->SetPostEffectBloom(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31006)
        {
            render->SetPostEffectDepthBufferShadow(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31007)
        {
            render->SetPostEffectSSGI(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_SSAO_ENABLE)
        {
            render->SetPostEffectSSAO(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_FOG_ENABLE)
        {
            render->SetPostEffectFog(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31008)
        {
            render->SetPostEffectHeightFog(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31009)
        {
            render->SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
        }
        else if (id == 31010)
        {
            render->SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Enabled);
        }
        else if (id == 31011)
        {
            render->SetPostEffectDepthOfFieldMode(DepthOfFieldMode::AutoNear);
        }
        else if (id == 31012)
        {
            render->SetPostEffectStarBurst(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31013)
        {
            render->SetPostEffectGodRay(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31014)
        {
            render->SetPostEffectHalo(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31120)
        {
            render->SetMeshMixSSS(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31121)
        {
            render->SetPhongTreatTextureAsWhite(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31122)
        {
            render->SetMeshMixSpecularIntensityOverrideEnabled(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31123)
        {
            render->SetMeshMixSpecularEdgeOverrideEnabled(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31301)
        {
            render->SetMeshInstancingHighQuality(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31302)
        {
            render->SetMeshMixSkinAnimAlphaClip(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31303)
        {
            render->SetMeshMixSkinAnimIgnoreTransparentMaterial(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31304)
        {
            state->useCustomMeshMixSkinAnimLoader = IsSettingsCheckboxChecked(hWnd, id);
        }
        else if (id == 31702)
        {
            render->SetPostEffectSSGIBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31704)
        {
            render->SetPostEffectSSGISeparableBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31806)
        {
            render->SetPostEffectSSAOCompositeGaussian3x3(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31807)
        {
            render->SetPostEffectSSAOSeparableBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31808)
        {
            render->SetPostEffectSSAOBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31809)
        {
            render->SetPostEffectSSAODepthScaledSampleDistance(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31810)
        {
            render->SetPostEffectSSAORandomSamplingDirection(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31811)
        {
            render->SetPostEffectSSAOMaxDarknessClamp(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32020)
        {
            render->SetPostEffectGaussianFilter(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32030)
        {
            render->SetPostEffectMaskedGaussianFilter(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32100)
        {
            render->SetPostEffectAA(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32101)
        {
            render->SetPostEffectTAA(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32112)
        {
            render->SetPostEffectMotionBlurCamera(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32141)
        {
            render->PlaceParticleEffect(state->particleEffectPreset, render->GetLookAtPos());
        }
        else if (id == 32154)
        {
            wchar_t text[512] { };
            GetDlgItemTextW(hWnd, 32152, text, static_cast<int>(_countof(text)));
            render->AddSettingsDialogText(text,
                                          state->settingsTextX,
                                          state->settingsTextY,
                                          IsSettingsCheckboxChecked(hWnd, 32153));
            UpdateSettingsTextList(state);
            const auto textList = render->GetSettingsDialogTextList();
            SelectSettingsTextListItem(state, static_cast<int>(textList.size()) - 1);
        }
        else if (id == 32254)
        {
            wchar_t text[512] { };
            GetDlgItemTextW(hWnd, 32252, text, static_cast<int>(_countof(text)));

            float posX = 0.0f;
            float posY = 0.0f;
            float posZ = 0.0f;
            int fontSize = 20;
            int r = 255;
            int g = 255;
            int b = 255;
            TryGetSettingsEditFloat(hWnd, 42250, posX);
            TryGetSettingsEditFloat(hWnd, 42251, posY);
            TryGetSettingsEditFloat(hWnd, 42252, posZ);
            TryGetSettingsEditInt(hWnd, 42253, fontSize);
            TryGetSettingsEditInt(hWnd, 42254, r);
            TryGetSettingsEditInt(hWnd, 42255, g);
            TryGetSettingsEditInt(hWnd, 42256, b);
            const D3DXCOLOR color(static_cast<float>(r) / 255.0f,
                                  static_cast<float>(g) / 255.0f,
                                  static_cast<float>(b) / 255.0f,
                                  1.0f);
            const bool decorated = IsSettingsCheckboxChecked(hWnd, 32253);

            render->AddWorldText(text,
                                 D3DXVECTOR3(posX, posY, posZ),
                                 fontSize,
                                 color,
                                 decorated);
            UpdateWorldTextList(state);
        }
        else if (id == 32255)
        {
            const int index = GetSelectedListViewIndex(state->worldTextList);
            if (index >= 0)
            {
                render->RemoveWorldText(index);
                UpdateWorldTextList(state);
            }
        }
        else if (id == 32300)
        {
            float duration = 0.5f;
            TryGetSettingsEditFloat(hWnd, 42300, duration);
            render->StartFadeIn(duration);
        }
        else if (id == 32301)
        {
            float duration = 0.5f;
            TryGetSettingsEditFloat(hWnd, 42300, duration);
            render->StartFadeOut(duration);
        }
        else if (id == 31411)
        {
            render->AddPointLight(render->GetLookAtPos(),
                                  state->pointLightBrightness,
                                  state->pointLightColor,
                                  state->pointLightShape);
            UpdatePointLightsList(state);
        }
        else if (id == 31412)
        {
            RemoveSelectedPointLight(hWnd);
        }
        else if (id == 41002)
        {
            render->SetCameraShakeDuration(state->cameraShakeDuration);
            render->SetCameraShakeIntensity(state->cameraShakeIntensity);
            render->TriggerCameraShake();
        }
        else if (id == 31311)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"Mix Mesh Files (*.x;*.blend.x)\0*.x;*.blend.x\0All Files (*.*)\0*.*\0",
                                           state->meshMixPath))
            {
                SetDlgItemTextW(hWnd, 31310, state->meshMixPath.c_str());
                const D3DXVECTOR3 pos = render->GetLookAtPos();
                int renderId = render->AddMeshMix(state->meshMixPath,
                                                        pos,
                                                        D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                        state->modelLoadScale);
                AddLoadedModelRecord(state, RenderSettingsDialogState::LoadedModelType::MeshMix, renderId, state->meshMixPath, pos);
            }
        }
        else if (id == 32211)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"PBR Mesh Files (*.x;*.blend.x)\0*.x;*.blend.x\0All Files (*.*)\0*.*\0",
                                           state->pbrMeshPath))
            {
                SetDlgItemTextW(hWnd, 32210, state->pbrMeshPath.c_str());
                const D3DXVECTOR3 pos = render->GetLookAtPos();
                int renderId = render->AddMeshPBR(state->pbrMeshPath,
                                                        pos,
                                                        D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                        state->modelLoadScale,
                                                        -1.0f,
                                                        state->pbrEnvMapPath);
                AddLoadedModelRecord(state, RenderSettingsDialogState::LoadedModelType::MeshPBR, renderId, state->pbrMeshPath, pos);
            }
        }
        else if (id == 31321)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"Mesh Instancing Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                                           state->meshInstancingPath))
            {
                SetDlgItemTextW(hWnd, 31320, state->meshInstancingPath.c_str());
                const D3DXVECTOR3 pos = render->GetLookAtPos();
                render->AddMeshInstansing(state->meshInstancingPath,
                                          pos,
                                          D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                          state->modelLoadScale);
                AddLoadedModelRecord(state,
                                     RenderSettingsDialogState::LoadedModelType::MeshInstancing,
                                     -1,
                                     state->meshInstancingPath,
                                     pos);
            }
        }
        else if (id == 31331)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"MeshMix Skin Anim Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                                           state->meshMixSkinAnimPath))
            {
                SetDlgItemTextW(hWnd, 31330, state->meshMixSkinAnimPath.c_str());
                const D3DXVECTOR3 pos = render->GetLookAtPos();
                int renderId = render->AddMeshMixSkinAnim(state->meshMixSkinAnimPath,
                                                          pos,
                                                          D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                          state->modelLoadScale,
                                                          AnimSetMap(),
                                                          -1.0f,
                                                          false,
                                                          false,
                                                          GetMeshMixSkinAnimLoadMode(state));
                AddLoadedModelRecord(state,
                                     RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim,
                                     renderId,
                                     state->meshMixSkinAnimPath,
                                     pos);
            }
        }
        else if (id == 31332)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0",
                                           state->xFileListPath))
            {
                int loadedCount = 0;
                int skippedCount = 0;
                LoadXFileListCsv(state, state->xFileListPath, loadedCount, skippedCount);

                wchar_t message[160] { };
                std::swprintf(message,
                              _countof(message),
                              L"Loaded: %d\nSkipped: %d",
                              loadedCount,
                              skippedCount);
                MessageBoxW(hWnd, message, L"Load XFileList", MB_OK | MB_ICONINFORMATION);
            }
        }
        else if (id == 31333)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0",
                                           state->xFileListMovePath))
            {
                int loadedCount = 0;
                int skippedCount = 0;
                state->render->LoadXFileListMoveFromCsv(state->xFileListMovePath, &loadedCount, &skippedCount);

                wchar_t message[160] { };
                std::swprintf(message,
                              _countof(message),
                              L"Move: %d\nSkip: %d",
                              loadedCount,
                              skippedCount);
                MessageBoxW(hWnd, message, L"Load XFileList Move", MB_OK | MB_ICONINFORMATION);
            }
        }
        else if (id == 31334)
        {
            render->ResetMovingPlatforms();
        }
        else if (id == 31360)
        {
            ShowSettingsOpenFileDialog(hWnd,
                                       L"MeshMix Non Animation Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                                       state->meshMixSkinNonAnimPath);
        }
        else if (id == 31361)
        {
            ShowSettingsOpenFileDialog(hWnd,
                                       L"MeshMix Animation Only Files (*.x)\0*.x\0All Files (*.*)\0*.*\0",
                                       state->meshMixSkinAnimOnlyPath);
        }
        else if (id == 31362)
        {
            if (!state->meshMixSkinNonAnimPath.empty() && !state->meshMixSkinAnimOnlyPath.empty())
            {
                const D3DXVECTOR3 pos = render->GetLookAtPos();
                int renderId = render->AddMeshMixSkinAnim(state->meshMixSkinNonAnimPath,
                                                          state->meshMixSkinAnimOnlyPath,
                                                          pos,
                                                          D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                          state->modelLoadScale,
                                                          AnimSetMap(),
                                                          -1.0f,
                                                          false,
                                                          false,
                                                          GetMeshMixSkinAnimLoadMode(state));
                AddLoadedModelRecord(state,
                                     RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim,
                                     renderId,
                                     state->meshMixSkinNonAnimPath,
                                     pos);
            }
        }
        else if (id == 31016)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0",
                                           state->settingsCsvPath))
            {
                render->ReloadSettingsCsv(state->settingsCsvPath);
                SyncRenderSettingsDialogFromRender(hWnd);
                UpdatePointLightsList(state);
            }
        }
        else if (id == 32213)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"Cube Environment Map Files (*.dds)\0*.dds\0All Files (*.*)\0*.*\0",
                                           state->pbrEnvMapPath))
            {
                SetDlgItemTextW(hWnd, 32212, state->pbrEnvMapPath.c_str());
                render->SetMeshPBREnvMapTexturePath(state->pbrEnvMapPath);
            }
        }
        else if (id == 32221)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.dds;*.tga)\0*.png;*.jpg;*.jpeg;*.bmp;*.dds;*.tga\0All Files (*.*)\0*.*\0",
                                           state->maskedGaussianMaskPath))
            {
                SetDlgItemTextW(hWnd, 32220, state->maskedGaussianMaskPath.c_str());
                render->SetPostEffectMaskedGaussianMaskPath(state->maskedGaussianMaskPath);
            }
        }
        else if (id == IDOK || id == IDCANCEL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }
        else if (id == 31350)
        {
            RemoveSelectedLoadedModel(hWnd);
        }
        else if (id == 31351)
        {
            PlaySelectedAnimation(hWnd);
        }
        else if (id == IDC_RENDER_SETTINGS_WINDOW_MODE_WINDOW)
        {
            render->ChangeWindowMode(eWindowMode::WINDOW);
        }
        else if (id == IDC_RENDER_SETTINGS_WINDOW_MODE_BORDERLESS)
        {
            render->ChangeWindowMode(eWindowMode::BORDERLESS);
        }
        else if (id == IDC_RENDER_SETTINGS_WINDOW_MODE_FULLSCREEN)
        {
            render->ChangeWindowMode(eWindowMode::FULLSCREEN);
        }
        return;
    }
    if (notifyCode == CBN_SELCHANGE)
    {
        if (id == 31610)
        {
            render->SetPostEffectDepthBufferShadowPcfTapCount(ComboIndexToTapCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31000)
        {
            int index = GetSettingsComboSelection(hWnd, id);
            if (index != CB_ERR)
            {
                wchar_t label[64] { };
                SendDlgItemMessageW(hWnd, id, CB_GETLBTEXT, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(label));
                int width = 0;
                int height = 0;
                if (TryParseResolutionLabel(label, width, height))
                {
                    render->ChangeResolution(width, height);
                }
            }
        }
        else if (id == 31001)
        {
            render->SetRenderQuality(ComboIndexToRenderingQuality(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31611)
        {
            render->SetPostEffectDepthBufferShadowCompositeTapCount(ComboIndexToTapCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31612)
        {
            render->SetPostEffectDepthBufferShadowTexSizeDivisor(ComboIndexToTexSizeDivisor(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31700)
        {
            render->SetPostEffectSSGISampleCount(ComboIndexToSampleCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31703)
        {
            render->SetPostEffectSSGIBlurKernelSize(ComboIndexToSSAOBlurKernelSize(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31801)
        {
            render->SetPostEffectSSAOTexSizeDivisor(ComboIndexToTexSizeDivisor(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31802)
        {
            render->SetPostEffectSSAOBlurKernelSize(ComboIndexToSSAOBlurKernelSize(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31805)
        {
            render->SetPostEffectSSAOSampleCount(ComboIndexToSampleCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 32140)
        {
            state->particleEffectPreset = ComboIndexToParticleEffectPreset(GetSettingsComboSelection(hWnd, id));
        }
        else if (id == 31410)
        {
            state->pointLightShape = ComboIndexToPointLightShape(GetSettingsComboSelection(hWnd, id));
        }
        return;
    }
}
void UpdateRenderSettingsScrollBar(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }
    RECT clientRect { };
    GetClientRect(hWnd, &clientRect);
    SCROLLINFO scrollInfo { };
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = (std::max)(0, state->contentHeight - 1);
    scrollInfo.nPage = clientRect.bottom - clientRect.top;
    scrollInfo.nPos = state->scrollPos;
    SetScrollInfo(hWnd, SB_VERT, &scrollInfo, TRUE);
}
void ScrollRenderSettingsTo(HWND hWnd, int newScrollPos)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }
    RECT clientRect { };
    GetClientRect(hWnd, &clientRect);
    int clientHeight = static_cast<int>(clientRect.bottom - clientRect.top);
    int maxScrollPos = (std::max)(0, state->contentHeight - clientHeight);
    int clampedPos = (std::max)(0, (std::min)(newScrollPos, maxScrollPos));
    int delta = state->scrollPos - clampedPos;
    if (delta == 0)
    {
        return;
    }
    state->scrollPos = clampedPos;
    ApplyRenderSettingsChildPositions(hWnd);
    UpdateRenderSettingsScrollBar(hWnd);
}
void HandleRenderSettingsVScroll(HWND hWnd, WPARAM wParam)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }
    SCROLLINFO scrollInfo { };
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_ALL;
    GetScrollInfo(hWnd, SB_VERT, &scrollInfo);
    int newPos = state->scrollPos;
    switch (LOWORD(wParam))
    {
    case SB_LINEUP:
        newPos -= 24;
        break;
    case SB_LINEDOWN:
        newPos += 24;
        break;
    case SB_PAGEUP:
        newPos -= static_cast<int>(scrollInfo.nPage);
        break;
    case SB_PAGEDOWN:
        newPos += static_cast<int>(scrollInfo.nPage);
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newPos = scrollInfo.nTrackPos;
        break;
    default:
        break;
    }
    ScrollRenderSettingsTo(hWnd, newPos);
}
void HandleRenderSettingsHScroll(HWND hWnd, LPARAM lParam)
{
    HWND trackbar = reinterpret_cast<HWND>(lParam);
    if (trackbar == NULL)
    {
        return;
    }
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Render* render = (state != nullptr) ? state->render : nullptr;
    if (render == nullptr)
    {
        return;
    }
    int id = GetDlgCtrlID(trackbar);
    switch (id)
    {
    case 31003:
        state->cameraHorizontalFovDegrees = TrackbarToFloat(trackbar, 1.0f, 180.0f);
        render->SetCameraHorizontalFovDegrees(state->cameraHorizontalFovDegrees);
        state->cameraHorizontalFovDegrees = render->GetCameraHorizontalFovDegrees();
        SetEditFloat(hWnd, trackbar, state->cameraHorizontalFovDegrees, L"%.0f");
        SetTrackbarFromFloat(hWnd, 31003, state->cameraHorizontalFovDegrees, 1.0f, 180.0f);
        break;
    case 31004:
        state->cameraShakeDuration = TrackbarToFloat(trackbar, 0.1f, 5.0f);
        render->SetCameraShakeDuration(state->cameraShakeDuration);
        SetEditFloat(hWnd, trackbar, state->cameraShakeDuration, L"%.1f");
        break;
    case 31005:
        state->cameraShakeIntensity = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        render->SetCameraShakeIntensity(state->cameraShakeIntensity);
        SetEditFloat(hWnd, trackbar, state->cameraShakeIntensity, L"%.2f");
        break;
    case IDC_RENDER_SETTINGS_SATURATE_LEVEL:
    case 31940:
    {
        float value = TrackbarToFloat(trackbar, 0.0f, 4.0f);
        render->SetPostEffectSaturate(value);
        SetEditFloat(hWnd, trackbar, value, L"%.1f");
        break;
    }
    case 31100:
        render->SetLightBrightness(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f), L"%.1f");
        break;
    case 31101:
        render->SetAmbientLightBrightness(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f), L"%.1f");
        break;
    case 31102:
        render->SetMeshMixSaturateShadowIntensity(TrackbarToFloat(trackbar, 0.0f, 10.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 10.0f));
        break;
    case 31103:
        render->SetMeshMixShadowDarkness(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 31104:
        render->SetMeshMixSpecularIntensity(TrackbarToFloat(trackbar, 0.0f, 2.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 2.0f));
        break;
    case 31105:
        render->SetMeshMixSpecularEdge(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 31106:
        render->SetMeshMixEnvMapBlend(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 31107:
        render->SetMeshMixSSSIntensity(TrackbarToFloat(trackbar, 0.0f, 30.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 30.0f));
        break;
    case 31111:
        state->modelLoadScale = TrackbarToFloat(trackbar, 0.1f, 10.0f);
        SetEditFloat(hWnd, trackbar, state->modelLoadScale, L"%.2f");
        break;
    case 31124:
        render->SetMeshMixFresnelIntensity(TrackbarToFloat(trackbar, 0.0f, 2.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 2.0f));
        break;
    case 31200:
    case 31201:
    case 31202:
    {
        float value = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        if (id == 31200) state->lightColor.r = value;
        if (id == 31201) state->lightColor.g = value;
        if (id == 31202) state->lightColor.b = value;
        render->SetLightColor(state->lightColor);
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 31203:
    case 31204:
    case 31205:
    {
        float value = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        if (id == 31203) state->ambientLightColor.r = value;
        if (id == 31204) state->ambientLightColor.g = value;
        if (id == 31205) state->ambientLightColor.b = value;
        render->SetAmbientLightColor(state->ambientLightColor);
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 31400:
    case 31401:
    case 31402:
    {
        float value = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        if (id == 31400) state->pointLightColor.r = value;
        if (id == 31401) state->pointLightColor.g = value;
        if (id == 31402) state->pointLightColor.b = value;
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 31403:
        state->pointLightBrightness = TrackbarToFloat(trackbar, 0.0f, 100.0f);
        SetEditFloat(hWnd, trackbar, state->pointLightBrightness);
        break;
    case 31500:
        render->SetMeshPBRRoughness(TrackbarToFloat(trackbar, 0.04f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.04f, 1.0f), L"%.3f");
        break;
    case 31501:
        render->SetMeshPBRMetallic(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f), L"%.3f");
        break;
    case 31502:
        render->SetMeshPBREnvReflectionIntensity(TrackbarToFloat(trackbar, 0.0f, 3.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 3.0f), L"%.3f");
        break;
    case 31503:
        render->SetMeshPBREnvMaxMipLevel(TrackbarToFloat(trackbar, 0.0f, 10.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 10.0f), L"%.3f");
        break;
    case 31504:
        render->SetMeshPBREnvDiffuseIntensity(TrackbarToFloat(trackbar, 0.0f, 3.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 3.0f), L"%.3f");
        break;
    case 31505:
        render->SetMeshPBREnvDiffuseMipLevel(TrackbarToFloat(trackbar, 0.0f, 10.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 10.0f), L"%.3f");
        break;
    case 31600:
        render->SetPostEffectDepthBufferShadowIntensity(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 31601:
        render->SetPostEffectDepthBufferShadowSaturationBoost(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 31602:
        render->SetPostEffectDepthBufferShadowCoverage(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 31701:
        render->SetPostEffectSSGISampleRadius(TrackbarToFloat(trackbar, 0.1f, 10.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.1f, 10.0f));
        break;
    case 31800:
        render->SetPostEffectSSAOSampleRadius(TrackbarToFloat(trackbar, 0.05f, 10.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.05f, 10.0f));
        break;
    case 31803:
        render->SetPostEffectSSAOShadowStrength(TrackbarToFloat(trackbar, 0.0f, 4.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 4.0f));
        break;
    case 31804:
        render->SetPostEffectSSAOSaturationBoost(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f));
        break;
    case 31900:
        render->SetPostEffectFogIntensity(TrackbarToFloat(trackbar, 0.0f, 20.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 20.0f), L"%.1f");
        break;
    case 31901:
    case 31902:
    case 31903:
    {
        float value = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        if (id == 31901) state->fogColor.r = value;
        if (id == 31902) state->fogColor.g = value;
        if (id == 31903) state->fogColor.b = value;
        render->SetPostEffectFogColor(state->fogColor);
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 31920:
        render->SetPostEffectHeightFogIntensity(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f));
        break;
    case 31921:
        render->SetPostEffectHeightFogStart(TrackbarToFloat(trackbar, -50000.0f, 50000.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, -50000.0f, 50000.0f), L"%.1f");
        break;
    case 31922:
        render->SetPostEffectHeightFogMax(TrackbarToFloat(trackbar, -50000.0f, 50000.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, -50000.0f, 50000.0f), L"%.1f");
        break;
    case 31923:
        render->SetPostEffectHeightFogDistanceStart(TrackbarToFloat(trackbar, 0.0f, 100000.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 100000.0f), L"%.1f");
        break;
    case 31924:
        render->SetPostEffectHeightFogDistanceMax(TrackbarToFloat(trackbar, 0.0f, 100000.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 100000.0f), L"%.1f");
        break;
    case 31950:
        render->SetPostEffectDepthOfFieldFocalDistance(TrackbarToFloat(trackbar, 0.5f, 50.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.5f, 50.0f), L"%.1f");
        break;
    case 31951:
        render->SetPostEffectDepthOfFieldMaxBlurDistance(TrackbarToFloat(trackbar, 0.5f, 50.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.5f, 50.0f), L"%.1f");
        break;
    case 31952:
        render->SetPostEffectDepthOfFieldAutoActivationDistance(TrackbarToFloat(trackbar, 0.5f, 50.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.5f, 50.0f), L"%.1f");
        break;
    case 31953:
        render->SetPostEffectDepthOfFieldStartNear(TrackbarToFloat(trackbar, 0.0f, 50.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 50.0f), L"%.1f");
        break;
    case 31970:
        render->SetPostEffectBloomThreshold(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f), L"%.1f");
        break;
    case 31971:
        render->SetPostEffectBloomWeightSum(TrackbarToFloat(trackbar, 1.0f, 100.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 1.0f, 100.0f), L"%.1f");
        break;
    case 31972:
        render->SetPostEffectHaloThreshold(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f), L"%.1f");
        break;
    case 31980:
        render->SetPostEffectStarBurstThreshold(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f), L"%.1f");
        break;
    case 31981:
        render->SetPostEffectStarBurstDistanceFade(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 32000:
    case 32001:
    case 32002:
    {
        float value = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        if (id == 32000) state->godRayColor.x = value;
        if (id == 32001) state->godRayColor.y = value;
        if (id == 32002) state->godRayColor.z = value;
        render->SetPostEffectGodRayLightColor(state->godRayColor);
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 32003:
        render->SetPostEffectGodRayIntensity(TrackbarToFloat(trackbar, 0.0f, 3.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 3.0f));
        break;
    case 32004:
        render->SetPostEffectGodRayVirtualProximityStrength(TrackbarToFloat(trackbar, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 5.0f));
        break;
    case 32005:
    case 32006:
    case 32007:
    {
        float value = TrackbarToFloat(trackbar, -200.0f, 200.0f);
        if (id == 32005) state->godRayPos.x = value;
        if (id == 32006) state->godRayPos.y = value;
        if (id == 32007) state->godRayPos.z = value;
        render->SetPostEffectGodRayLightPos(state->godRayPos);
        SetEditFloat(hWnd, trackbar, value, L"%.1f");
        break;
    }
    case 32021:
        render->SetPostEffectGaussianStrength(TrackbarToFloat(trackbar, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 0.0f, 1.0f));
        break;
    case 32110:
        render->SetPostEffectMotionBlurCameraMaxBlurPixels(TrackbarToFloat(trackbar, 1.0f, 64.0f));
        SetEditFloat(hWnd, trackbar, TrackbarToFloat(trackbar, 1.0f, 64.0f), L"%.0f");
        break;
    case 32111:
    {
        int sampleCount = TrackbarToInt(trackbar, 2, 21);
        render->SetPostEffectMotionBlurCameraSampleCount(sampleCount);
        SetEditInt(hWnd, trackbar, sampleCount);
        break;
    }
    case 32120:
    {
        int quality = TrackbarToInt(trackbar, 1, 8);
        render->SetPostEffectFXAAQuality(quality);
        SetEditInt(hWnd, trackbar, quality);
        break;
    }
    case 32130:
    {
        int sampleSize = TrackbarToInt(trackbar, 1, 21) | 1;
        render->SetPostEffectFontSampleSize(sampleSize);
        SetEditInt(hWnd, trackbar, sampleSize);
        break;
    }
    case 32150:
        state->settingsTextX = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        SetEditFloat(hWnd, trackbar, state->settingsTextX);
        {
            const int index = GetSelectedListViewIndex(state->settingsTextList);
            if (render->SetSettingsDialogTextPosition(static_cast<size_t>(index), state->settingsTextX, state->settingsTextY))
            {
                UpdateSettingsTextList(state);
                SelectSettingsTextListItem(state, index);
            }
        }
        break;
    case 32151:
        state->settingsTextY = TrackbarToFloat(trackbar, 0.0f, 1.0f);
        SetEditFloat(hWnd, trackbar, state->settingsTextY);
        {
            const int index = GetSelectedListViewIndex(state->settingsTextList);
            if (render->SetSettingsDialogTextPosition(static_cast<size_t>(index), state->settingsTextX, state->settingsTextY))
            {
                UpdateSettingsTextList(state);
                SelectSettingsTextListItem(state, index);
            }
        }
        break;
    default:
        break;
    }
}
void HandleRenderSettingsNotify(HWND hWnd, LPARAM lParam)
{
    const NMHDR* header = reinterpret_cast<const NMHDR*>(lParam);
    if (header == nullptr)
    {
        return;
    }
    if (header->idFrom == 31340 && header->code == LVN_ITEMCHANGED)
    {
        const NMLISTVIEW* listView = reinterpret_cast<const NMLISTVIEW*>(lParam);
        if ((listView->uChanged & LVIF_STATE) != 0 &&
            (listView->uNewState & LVIS_SELECTED) != 0)
        {
            RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            PopulateAnimationListForModel(state, listView->iItem);
        }
    }
    else if (header->idFrom == 32155 && header->code == LVN_ITEMCHANGED)
    {
        const NMLISTVIEW* listView = reinterpret_cast<const NMLISTVIEW*>(lParam);
        if ((listView->uChanged & LVIF_STATE) != 0 &&
            (listView->uNewState & LVIS_SELECTED) != 0)
        {
            LoadSelectedSettingsTextPosition(hWnd);
        }
    }
    else if (header->idFrom == 31341 && header->code == NM_DBLCLK)
    {
        PlaySelectedAnimation(hWnd);
    }
}
}
}
