#include "RenderSettingsDialogInternal.h"
namespace NSRender
{
namespace RenderSettingsDialogInternal
{
std::wstring GetDisplayFileName(const std::wstring& filePath)
{
    const std::wstring::size_type pos = filePath.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
    {
        return filePath;
    }
    return filePath.substr(pos + 1);
}
const wchar_t* LoadedModelTypeToText(const RenderSettingsDialogState::LoadedModelType type)
{
    switch (type)
    {
    case RenderSettingsDialogState::LoadedModelType::MeshMix:
        return L"MeshMix";
    case RenderSettingsDialogState::LoadedModelType::MeshPBR:
        return L"MeshPBR";
    case RenderSettingsDialogState::LoadedModelType::MeshInstancing:
        return L"Instancing";
    case RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim:
        return L"SkinAnim";
    default:
        return L"Model";
    }
}
void UpdateLoadedModelsList(RenderSettingsDialogState* state)
{
    if (state == nullptr || state->loadedModelsList == NULL)
    {
        return;
    }
    ListView_DeleteAllItems(state->loadedModelsList);
    for (int i = 0; i < static_cast<int>(state->loadedModels.size()); ++i)
    {
        const auto& model = state->loadedModels.at(i);
        const std::wstring fileName = GetDisplayFileName(model.filePath);
        wchar_t scaleText[32] { };
        wchar_t posText[96] { };
        std::swprintf(scaleText, sizeof(scaleText) / sizeof(scaleText[0]), L"%.2f", model.scale);
        std::swprintf(posText,
                      sizeof(posText) / sizeof(posText[0]),
                      L"(%.3f, %.3f, %.3f)",
                      model.pos.x,
                      model.pos.y,
                      model.pos.z);
        LVITEMW item { };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(LoadedModelTypeToText(model.type));
        item.lParam = i;
        ListView_InsertItem(state->loadedModelsList, &item);
        ListView_SetItemText(state->loadedModelsList, i, 1, const_cast<LPWSTR>(fileName.c_str()));
        ListView_SetItemText(state->loadedModelsList, i, 2, scaleText);
        ListView_SetItemText(state->loadedModelsList, i, 3, posText);
    }
}

RenderSettingsDialogState::LoadedModelType ToDialogLoadedModelType(const RenderLoadedModelType type)
{
    switch (type)
    {
    case RenderLoadedModelType::MeshMix:
        return RenderSettingsDialogState::LoadedModelType::MeshMix;
    case RenderLoadedModelType::MeshPBR:
        return RenderSettingsDialogState::LoadedModelType::MeshPBR;
    case RenderLoadedModelType::MeshInstancing:
        return RenderSettingsDialogState::LoadedModelType::MeshInstancing;
    case RenderLoadedModelType::MeshMixSkinAnim:
        return RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim;
    default:
        return RenderSettingsDialogState::LoadedModelType::MeshMix;
    }
}

bool LoadedModelRecordsEqual(const RenderSettingsDialogState::LoadedModelRecord& left,
                             const RenderSettingsDialogState::LoadedModelRecord& right)
{
    return left.type == right.type &&
           left.renderId == right.renderId &&
           left.filePath == right.filePath &&
           left.scale == right.scale &&
           left.pos.x == right.pos.x &&
           left.pos.y == right.pos.y &&
           left.pos.z == right.pos.z;
}

void SyncLoadedModelsFromRender(RenderSettingsDialogState* state)
{
    if (state == nullptr || state->render == nullptr)
    {
        return;
    }

    const auto renderModels = state->render->GetLoadedModelInfoList();
    std::vector<RenderSettingsDialogState::LoadedModelRecord> syncedModels;
    syncedModels.reserve(renderModels.size());

    for (const auto& renderModel : renderModels)
    {
        RenderSettingsDialogState::LoadedModelRecord record;
        record.type = ToDialogLoadedModelType(renderModel.type);
        record.renderId = renderModel.renderId;
        record.filePath = renderModel.filePath;
        record.scale = renderModel.scale;
        record.pos = renderModel.pos;
        syncedModels.push_back(record);
    }

    bool changed = state->loadedModels.size() != syncedModels.size();
    if (!changed)
    {
        for (size_t i = 0; i < syncedModels.size(); ++i)
        {
            if (!LoadedModelRecordsEqual(state->loadedModels.at(i), syncedModels.at(i)))
            {
                changed = true;
                break;
            }
        }
    }

    if (!changed)
    {
        return;
    }

    const int selectedIndex = GetSelectedListViewIndex(state->loadedModelsList);
    state->loadedModels = syncedModels;
    UpdateLoadedModelsList(state);

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(state->loadedModels.size()))
    {
        ListView_SetItemState(state->loadedModelsList, selectedIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        PopulateAnimationListForModel(state, selectedIndex);
    }
    else
    {
        ClearAnimationList(state);
    }
}
const wchar_t* PointLightShapeToText(const PointLightShape shape)
{
    switch (shape)
    {
    case PointLightShape::Point:
        return L"Point";
    case PointLightShape::Line:
        return L"Line";
    case PointLightShape::Square:
        return L"Square";
    case PointLightShape::Cube:
        return L"Cube";
    case PointLightShape::Sphere:
        return L"Sphere";
    default:
        return L"Unknown";
    }
}
void UpdatePointLightsList(RenderSettingsDialogState* state)
{
    if (state == nullptr || state->pointLightsList == NULL)
    {
        return;
    }
    ListView_DeleteAllItems(state->pointLightsList);
    const auto pointLights = Light::GetPointLightList();
    for (int i = 0; i < static_cast<int>(pointLights.size()); ++i)
    {
        const auto& light = pointLights.at(i);
        wchar_t posText[96] { };
        wchar_t colorText[96] { };
        wchar_t brightnessText[32] { };
        std::swprintf(posText,
                      sizeof(posText) / sizeof(posText[0]),
                      L"(%.3f, %.3f, %.3f)",
                      light.m_pos.x,
                      light.m_pos.y,
                      light.m_pos.z);
        std::swprintf(colorText,
                      sizeof(colorText) / sizeof(colorText[0]),
                      L"%.2f, %.2f, %.2f",
                      light.m_color.r,
                      light.m_color.g,
                      light.m_color.b);
        std::swprintf(brightnessText, sizeof(brightnessText) / sizeof(brightnessText[0]), L"%.2f", light.m_brightness);
        LVITEMW item { };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = posText;
        item.lParam = i;
        ListView_InsertItem(state->pointLightsList, &item);
        ListView_SetItemText(state->pointLightsList, i, 1, const_cast<LPWSTR>(PointLightShapeToText(light.m_shape)));
        ListView_SetItemText(state->pointLightsList, i, 2, colorText);
        ListView_SetItemText(state->pointLightsList, i, 3, brightnessText);
    }
}

void UpdateSettingsTextList(RenderSettingsDialogState* state)
{
    if (state == nullptr || state->render == nullptr || state->settingsTextList == NULL)
    {
        return;
    }

    const int selectedIndex = GetSelectedListViewIndex(state->settingsTextList);
    const auto textList = state->render->GetSettingsDialogTextList();
    ListView_DeleteAllItems(state->settingsTextList);
    for (int i = 0; i < static_cast<int>(textList.size()); ++i)
    {
        wchar_t xText[32] { };
        wchar_t yText[32] { };
        const wchar_t* decoratedText = textList[i].decorated ? L"Yes" : L"No";
        std::swprintf(xText, sizeof(xText) / sizeof(xText[0]), L"%.2f", textList[i].x);
        std::swprintf(yText, sizeof(yText) / sizeof(yText[0]), L"%.2f", textList[i].y);
        const wchar_t* values[] = { textList[i].text.c_str(), xText, yText, decoratedText };
        AddSettingsListViewRow(state->settingsTextList, i, values, 4);
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(textList.size()))
    {
        SelectSettingsTextListItem(state, selectedIndex);
    }
}

void SelectSettingsTextListItem(RenderSettingsDialogState* state, int index)
{
    if (state == nullptr || state->settingsTextList == NULL || index < 0)
    {
        return;
    }
    ListView_SetItemState(state->settingsTextList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(state->settingsTextList, index, FALSE);
}

void LoadSelectedSettingsTextPosition(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr || state->render == nullptr)
    {
        return;
    }
    const int index = GetSelectedListViewIndex(state->settingsTextList);
    const auto textList = state->render->GetSettingsDialogTextList();
    if (index < 0 || index >= static_cast<int>(textList.size()))
    {
        return;
    }
    state->settingsTextX = textList[index].x;
    state->settingsTextY = textList[index].y;
    SetSettingsEditFloat(hWnd, 42150, state->settingsTextX);
    SetSettingsEditFloat(hWnd, 42151, state->settingsTextY);
    SetTrackbarFromFloat(hWnd, 32150, state->settingsTextX, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 32151, state->settingsTextY, 0.0f, 1.0f);
}

void UpdateWorldTextList(RenderSettingsDialogState* state)
{
    if (state == nullptr || state->render == nullptr || state->worldTextList == NULL)
    {
        return;
    }

    const int selectedIndex = GetSelectedListViewIndex(state->worldTextList);
    const auto& worldTextList = state->render->GetWorldTextList();
    ListView_DeleteAllItems(state->worldTextList);
    for (int i = 0; i < static_cast<int>(worldTextList.size()); ++i)
    {
        const auto& entry = worldTextList[i];
        wchar_t xText[16];
        wchar_t yText[16];
        wchar_t zText[16];
        wchar_t szText[16];
        wchar_t rText[8];
        wchar_t gText[8];
        wchar_t bText[8];
        std::swprintf(xText, 16, L"%.3f", entry.worldPos.x);
        std::swprintf(yText, 16, L"%.3f", entry.worldPos.y);
        std::swprintf(zText, 16, L"%.3f", entry.worldPos.z);
        std::swprintf(szText, 16, L"%d", entry.fontSize);
        std::swprintf(rText, 8, L"%d", static_cast<int>(entry.color.r * 255.0f));
        std::swprintf(gText, 8, L"%d", static_cast<int>(entry.color.g * 255.0f));
        std::swprintf(bText, 8, L"%d", static_cast<int>(entry.color.b * 255.0f));
        const wchar_t* exText = entry.decorated ? L"Y" : L"N";
        const wchar_t* values[] = { entry.text.c_str(), xText, yText, zText, szText, rText, gText, bText, exText };
        AddSettingsListViewRow(state->worldTextList, i, values, 9);
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(worldTextList.size()))
    {
        ListView_SetItemState(state->worldTextList, selectedIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

int GetSelectedListViewIndex(HWND listView)
{
    if (listView == NULL)
    {
        return -1;
    }
    return ListView_GetNextItem(listView, -1, LVNI_SELECTED);
}
void RemoveSelectedPointLight(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }
    int index = GetSelectedListViewIndex(state->pointLightsList);
    if (index < 0)
    {
        return;
    }
    if (Light::RemovePointLight(static_cast<size_t>(index)))
    {
        UpdatePointLightsList(state);
    }
}

void ClearPointLights(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }

    Light::ClearPointLights();
    UpdatePointLightsList(state);
}
void ClearAnimationList(RenderSettingsDialogState* state)
{
    if (state == nullptr)
    {
        return;
    }
    state->activeAnimationModelId = -1;
    if (state->animationList != NULL)
    {
        ListView_DeleteAllItems(state->animationList);
    }
}
void PopulateAnimationListForModel(RenderSettingsDialogState* state, int modelIndex)
{
    ClearAnimationList(state);
    if (state == nullptr || state->render == nullptr || state->animationList == NULL ||
        modelIndex < 0 || modelIndex >= static_cast<int>(state->loadedModels.size()))
    {
        return;
    }
    const auto& model = state->loadedModels.at(modelIndex);
    if (model.type != RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim)
    {
        return;
    }
    const auto* animationInfoList = state->render->GetMeshMixSkinAnimAnimationInfoList(model.renderId);
    if (animationInfoList == nullptr)
    {
        return;
    }
    state->activeAnimationModelId = model.renderId;
    for (int i = 0; i < static_cast<int>(animationInfoList->size()); ++i)
    {
        const auto& animation = animationInfoList->at(i);
        const std::wstring fileName = GetDisplayFileName(animation.filePath);
        LVITEMW item { };
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(animation.name.c_str());
        item.lParam = i;
        ListView_InsertItem(state->animationList, &item);
        ListView_SetItemText(state->animationList, i, 1, const_cast<LPWSTR>(fileName.c_str()));
        ListView_SetItemText(state->animationList, i, 2, const_cast<LPWSTR>(animation.mode.c_str()));
    }
}
void AddLoadedModelRecord(RenderSettingsDialogState* state,
                          const RenderSettingsDialogState::LoadedModelType type,
                          int renderId,
                          const std::wstring& filePath,
                          const D3DXVECTOR3& pos)
{
    if (state == nullptr)
    {
        return;
    }
    RenderSettingsDialogState::LoadedModelRecord record;
    record.type = type;
    record.renderId = renderId;
    record.filePath = filePath;
    record.scale = state->modelLoadScale;
    record.pos = pos;
    state->loadedModels.push_back(record);
    UpdateLoadedModelsList(state);
    if (type == RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim)
    {
        PopulateAnimationListForModel(state, static_cast<int>(state->loadedModels.size()) - 1);
    }
}
void AdjustLoadedModelIdsAfterRemove(RenderSettingsDialogState* state,
                                     const RenderSettingsDialogState::LoadedModelType type,
                                     int removedRenderId)
{
    if (state == nullptr)
    {
        return;
    }
    for (auto& model : state->loadedModels)
    {
        if (model.type == type && model.renderId > removedRenderId)
        {
            --model.renderId;
        }
    }
}
void RemoveSelectedLoadedModel(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr || state->render == nullptr)
    {
        return;
    }
    int index = GetSelectedListViewIndex(state->loadedModelsList);
    if (index < 0 || index >= static_cast<int>(state->loadedModels.size()))
    {
        return;
    }
    const auto record = state->loadedModels.at(index);
    bool removed = false;
    switch (record.type)
    {
    case RenderSettingsDialogState::LoadedModelType::MeshMix:
        removed = state->render->RemoveMeshMix(record.renderId);
        break;
    case RenderSettingsDialogState::LoadedModelType::MeshPBR:
        removed = state->render->RemoveMeshPBR(record.renderId);
        break;
    case RenderSettingsDialogState::LoadedModelType::MeshInstancing:
        removed = state->render->RemoveMeshInstancing(record.filePath);
        break;
    case RenderSettingsDialogState::LoadedModelType::MeshMixSkinAnim:
        removed = state->render->RemoveMeshMixSkinAnim(record.renderId);
        break;
    default:
        break;
    }
    if (!removed)
    {
        return;
    }
    state->loadedModels.erase(state->loadedModels.begin() + index);
    if (record.type != RenderSettingsDialogState::LoadedModelType::MeshInstancing)
    {
        AdjustLoadedModelIdsAfterRemove(state, record.type, record.renderId);
    }
    UpdateLoadedModelsList(state);
    ClearAnimationList(state);
}
void PlaySelectedAnimation(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr || state->render == nullptr || state->animationList == NULL || state->activeAnimationModelId < 0)
    {
        return;
    }
    int index = GetSelectedListViewIndex(state->animationList);
    if (index < 0)
    {
        return;
    }
    wchar_t animationName[256] { };
    ListView_GetItemText(state->animationList, index, 0, animationName, static_cast<int>(sizeof(animationName) / sizeof(animationName[0])));
    if (animationName[0] != L'\0')
    {
        state->render->PlayMeshMixSkinAnimAnimation(state->activeAnimationModelId, animationName);
    }
}
}
}
