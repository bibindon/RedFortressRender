#pragma once
#include "RenderSettingsDialog.h"
#include <algorithm>
#include <commctrl.h>
#include <commdlg.h>
#include <cstdlib>
#include <cwchar>
#include <cwctype>
#include <fstream>
#include <string>
#include <vector>
#include "Render.h"
namespace NSRender
{
namespace RenderSettingsDialogInternal
{
static constexpr const wchar_t* RENDER_SETTINGS_DIALOG_CLASS_NAME = L"NSRenderSettingsDialog";
static constexpr int RENDER_SETTINGS_CONTENT_BOTTOM_MARGIN = 24;
static constexpr UINT_PTR RENDER_SETTINGS_SYNC_TIMER_ID = 1;
static constexpr UINT RENDER_SETTINGS_SYNC_INTERVAL_MS = 1000;
struct RenderSettingsDialogState
{
    enum class LoadedModelType
    {
        MeshMix2,
        MeshPBR,
        MeshInstancing2,
        MeshMixSkinAnim2,
        MeshMixAnimNoBone2
    };
    struct LoadedModelRecord
    {
        LoadedModelType type = LoadedModelType::MeshMix2;
        int renderId = -1;
        std::wstring filePath;
        float scale = 1.0f;
        D3DXVECTOR3 pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    };
    Render* render = nullptr;
    int scrollPos = 0;
    int contentHeight = 0;
    HWND loadedModelsList = NULL;
    HWND animationList = NULL;
    HWND pointLightsList = NULL;
    HWND settingsTextList = NULL;
    D3DXCOLOR lightColor = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    D3DXCOLOR ambientLightColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
    D3DXCOLOR fogColor = D3DXCOLOR(0.72f, 0.78f, 0.86f, 1.0f);
    D3DXVECTOR3 godRayColor = D3DXVECTOR3(1.0f, 0.9f, 0.8f);
    D3DXVECTOR3 godRayPos = D3DXVECTOR3(1000.0f, 100.0f, 1000.0f);
    D3DXCOLOR pointLightColor = D3DXCOLOR(1.0f, 0.35f, 0.10f, 1.0f);
    float pointLightBrightness = 1.0f;
    PointLightShape pointLightShape = PointLightShape::Point;
    float settingsTextX = 0.1f;
    float settingsTextY = 0.1f;
    float cameraNearPlane = 0.1f;
    float cameraFarPlane = 30000.0f;
    float cameraHorizontalFovDegrees = 90.0f;
    float cameraShakeDuration = 1.0f;
    float cameraShakeIntensity = 0.12f;
    float gBufferNearPlane = 0.1f;
    float gBufferFarPlane = 30.0f;
    float modelLoadScale = 1.0f;
    ParticleEffectPreset particleEffectPreset = ParticleEffectPreset::Smoke;
    std::wstring meshMixPath;
    std::wstring pbrMeshPath;
    std::wstring pbrEnvMapPath;
    std::wstring meshInstancingPath;
    std::wstring meshMixSkinAnimPath;
    std::wstring meshMixSkinNonAnimPath;
    std::wstring meshMixSkinAnimOnlyPath;
    std::wstring xFileListPath;
    std::wstring xFileListMovePath;
    std::wstring maskedGaussianMaskPath;
    std::wstring settingsCsvPath;
    std::vector<LoadedModelRecord> loadedModels;
    int activeAnimationModelId = -1;
    struct ChildPlacement
    {
        HWND hWnd = NULL;
        RECT rect { };
    };
    std::vector<ChildPlacement> childPlacements;

    HWND worldTextList = NULL;
    std::wstring worldTextText = L"Hello";
    float worldTextX = 0.0f;
    float worldTextY = 0.0f;
    float worldTextZ = 0.0f;
    int worldTextFontSize = 20;
    int worldTextColorR = 255;
    int worldTextColorG = 255;
    int worldTextColorB = 255;
    bool worldTextDecorated = false;
};
enum RenderSettingsControlId
{
    IDC_RENDER_SETTINGS_WINDOW_MODE = 30001,
    IDC_RENDER_SETTINGS_GBUFFER_ENABLE,
    IDC_RENDER_SETTINGS_SATURATE_ENABLE,
    IDC_RENDER_SETTINGS_SATURATE_LEVEL,
    IDC_RENDER_SETTINGS_GAUSSIAN_ENABLE,
    IDC_RENDER_SETTINGS_BLOOM_ENABLE,
    IDC_RENDER_SETTINGS_SSAO_ENABLE,
    IDC_RENDER_SETTINGS_FOG_ENABLE,
    IDC_RENDER_SETTINGS_DEBUG_VIEW,
    IDC_RENDER_SETTINGS_WINDOW_MODE_WINDOW,
    IDC_RENDER_SETTINGS_WINDOW_MODE_BORDERLESS,
    IDC_RENDER_SETTINGS_WINDOW_MODE_FULLSCREEN,
    IDC_RENDER_SETTINGS_GBUFFER_DEPTH_R32F,
    IDC_RENDER_SETTINGS_GBUFFER_DEPTH_R16F,
    IDC_RENDER_SETTINGS_GBUFFER_FOG_DEPTH_R32F,
    IDC_RENDER_SETTINGS_GBUFFER_FOG_DEPTH_R16F,
    IDC_RENDER_SETTINGS_GBUFFER_POSITION_RGBA16F,
    IDC_RENDER_SETTINGS_GBUFFER_POSITION_ABGR8,
    IDC_RENDER_SETTINGS_GBUFFER_NORMAL_RGBA16F,
    IDC_RENDER_SETTINGS_GBUFFER_NORMAL_ABGR8,
    IDC_RENDER_SETTINGS_GBUFFER_THICKNESS_RGBA16F,
    IDC_RENDER_SETTINGS_GBUFFER_THICKNESS_ABGR8,
    IDC_RENDER_SETTINGS_GBUFFER_BACK_DEPTH_R32F,
    IDC_RENDER_SETTINGS_GBUFFER_BACK_DEPTH_R16F,
    IDC_RENDER_SETTINGS_DEBUG_WORLD_POS,
    IDC_RENDER_SETTINGS_DEBUG_NORMAL,
    IDC_RENDER_SETTINGS_DEBUG_DEPTH,
    IDC_RENDER_SETTINGS_DEBUG_THICKNESS,
    IDC_RENDER_SETTINGS_DEBUG_BACK_DEPTH,
    IDC_RENDER_SETTINGS_GBUFFER_FRONT_BACKFACE_CULLING_ENABLE,
    IDC_RENDER_SETTINGS_POINT_LIGHT_DISABLE,
    IDC_RENDER_SETTINGS_FRAME_RATE_SLEEP_DISABLE,
};
void SetDefaultGuiFont(HWND hWnd);
void CreateSettingsStatic(HWND parent, const wchar_t* text, int x, int y, int w, int h);
HWND CreateSettingsCheckbox(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h);
HWND CreateSettingsRadio(HWND parent,
                         int id,
                         const wchar_t* text,
                         int x,
                         int y,
                         int w,
                         int h,
                         bool beginGroup = false);
void CreateSettingsGroupBox(HWND parent, const wchar_t* text, int x, int y, int w, int h);
void CreateSettingsButton(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id = 0);
void CreateSettingsEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id = 0);
void SetNearestValueEditText(HWND hWnd, HWND trackbar, const wchar_t* text);
float SliderToFloat(int sliderPos, float minValue, float maxValue);
int SliderToInt(int sliderPos, int minValue, int maxValue);
float TrackbarToFloat(HWND trackbar, float minValue, float maxValue);
int TrackbarToInt(HWND trackbar, int minValue, int maxValue);
void SetEditFloat(HWND hWnd, HWND trackbar, float value, const wchar_t* format = L"%.2f");
void SetEditInt(HWND hWnd, HWND trackbar, int value);
HWND CreateSettingsCombo(HWND parent, int id, int x, int y, int w, int h);
void CreateSettingsTrackbar(HWND parent, int id, int x, int y, int w, int h, int minValue, int maxValue, int value);
HWND CreateSettingsListView(HWND parent, int id, int x, int y, int w, int h, const wchar_t* const* columns, const int* widths, int columnCount);
void AddSettingsListViewRow(HWND listView, int row, const wchar_t* const* values, int valueCount);
std::wstring FormatResolutionLabel(int width, int height);
bool TryParseResolutionLabel(const wchar_t* label, int& width, int& height);
void InitializeRenderSettingsControls(HWND hWnd, RenderSettingsDialogState* state);
std::wstring GetDisplayFileName(const std::wstring& filePath);
const wchar_t* LoadedModelTypeToText(RenderSettingsDialogState::LoadedModelType type);
void UpdateLoadedModelsList(RenderSettingsDialogState* state);
void SyncLoadedModelsFromRender(RenderSettingsDialogState* state);
const wchar_t* PointLightShapeToText(PointLightShape shape);
void UpdatePointLightsList(RenderSettingsDialogState* state);
void UpdateSettingsTextList(RenderSettingsDialogState* state);
void UpdateWorldTextList(RenderSettingsDialogState* state);
void SelectSettingsTextListItem(RenderSettingsDialogState* state, int index);
void LoadSelectedSettingsTextPosition(HWND hWnd);
int GetSelectedListViewIndex(HWND listView);
void RemoveSelectedPointLight(HWND hWnd);
void ClearPointLights(HWND hWnd);
void ClearAnimationList(RenderSettingsDialogState* state);
void PopulateAnimationListForModel(RenderSettingsDialogState* state, int modelIndex);
void AddLoadedModelRecord(RenderSettingsDialogState* state, RenderSettingsDialogState::LoadedModelType type, int renderId, const std::wstring& filePath, const D3DXVECTOR3& pos);
void AdjustLoadedModelIdsAfterRemove(RenderSettingsDialogState* state, RenderSettingsDialogState::LoadedModelType type, int removedRenderId);
void RemoveSelectedLoadedModel(HWND hWnd);
void PlaySelectedAnimation(HWND hWnd);
bool IsSettingsCheckboxChecked(HWND hWnd, int id);
int GetSettingsComboSelection(HWND hWnd, int id);
int ComboIndexToTapCount(int index);
int TapCountToComboIndex(int tapCount);
int ComboIndexToTexSizeDivisor(int index);
int TexSizeDivisorToComboIndex(int divisor);
int ComboIndexToSSAOBlurKernelSize(int index);
int BlurKernelSizeToComboIndex(int kernelSize);
int ComboIndexToSampleCount(int index);
int SampleCountToComboIndex(int sampleCount);
std::wstring ComboIndexToRenderingQuality(int index);
int RenderingQualityToComboIndex(const std::wstring& quality);
ParticleEffectPreset ComboIndexToParticleEffectPreset(int index);
PointLightShape ComboIndexToPointLightShape(int index);
bool ShowSettingsOpenFileDialog(HWND owner, const wchar_t* filter, std::wstring& path);
bool TryGetSettingsEditFloat(HWND hWnd, int id, float& value);
bool TryGetSettingsEditInt(HWND hWnd, int id, int& value);
void SetTrackbarFromFloat(HWND hWnd, int trackbarId, float value, float minValue, float maxValue);
void SetTrackbarFromInt(HWND hWnd, int trackbarId, int value, int minValue, int maxValue);
void SetSettingsCheckbox(HWND hWnd, int id, bool checked);
void SetSettingsEditTextIfNotFocused(HWND hWnd, int id, const wchar_t* text);
void SetSettingsEditFloat(HWND hWnd, int id, float value, const wchar_t* format = L"%.2f");
void SetSettingsEditInt(HWND hWnd, int id, int value);
void SetSettingsComboSelection(HWND hWnd, int id, int index);
void SetSettingsComboTextSelection(HWND hWnd, int id, const std::wstring& text);
BOOL CALLBACK CaptureRenderSettingsChildPlacementProc(HWND child, LPARAM lParam);
void CaptureRenderSettingsChildPlacements(HWND hWnd);
void ApplyRenderSettingsChildPositions(HWND hWnd);
bool HandleRenderSettingsEditCommit(HWND hWnd, int id);
void HandleRenderSettingsCommand(HWND hWnd, WPARAM wParam);
void UpdateRenderSettingsScrollBar(HWND hWnd);
void ScrollRenderSettingsTo(HWND hWnd, int newScrollPos);
void HandleRenderSettingsVScroll(HWND hWnd, WPARAM wParam);
void HandleRenderSettingsHScroll(HWND hWnd, LPARAM lParam);
void HandleRenderSettingsNotify(HWND hWnd, LPARAM lParam);
void SyncDebugGBufferCheckboxes(HWND hWnd, Render* render);
void SyncRenderSettingsDialogFromRender(HWND hWnd);
LRESULT CALLBACK RenderSettingsDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool EnsureRenderSettingsDialogClass(HINSTANCE hInstance);
void MoveWindowNearParent(HWND hWnd, HWND parent);
}
}
