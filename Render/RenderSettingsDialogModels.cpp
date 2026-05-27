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
                      L"(%.1f, %.1f, %.1f)",
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
                      L"(%.1f, %.1f, %.1f)",
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
