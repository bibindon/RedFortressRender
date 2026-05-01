#pragma once

#include "../Render/Render.h"

#include <string>
#include <vector>
#include <windows.h>

constexpr int WINDOW_SIZE_W = 1600;
constexpr int WINDOW_SIZE_H = 900;
constexpr float MOUSE_CAMERA_SENSITIVITY = 0.005f;
constexpr float MODEL_SPAWN_FORWARD_OFFSET = 6.0f;
constexpr float MOUSE_WHEEL_CAMERA_DISTANCE = 1.0f;
constexpr float SATURATE_MIN = 0.0f;
constexpr float SATURATE_MAX = 4.0f;
constexpr float SATURATE_STEP = 0.1f;
constexpr float FOG_INTENSITY_MIN = 0.0f;
constexpr float FOG_INTENSITY_MAX = 20.0f;
constexpr float FOG_INTENSITY_STEP = 0.1f;
constexpr float SUN_LIGHT_INTENSITY_MIN = 0.0f;
constexpr float SUN_LIGHT_INTENSITY_MAX = 5.0f;
constexpr float SUN_LIGHT_INTENSITY_STEP = 0.1f;
constexpr float SHADOW_INTENSITY_MIN = 0.0f;
constexpr float SHADOW_INTENSITY_MAX = 1.0f;
constexpr float SHADOW_INTENSITY_STEP = 0.05f;
constexpr float SHADOW_SATURATION_BOOST_MIN = 0.0f;
constexpr float SHADOW_SATURATION_BOOST_MAX = 1.0f;
constexpr float SHADOW_SATURATION_BOOST_STEP = 0.05f;
constexpr float SSAO_BRIGHTNESS_MIN = 0.25f;
constexpr float SSAO_BRIGHTNESS_MAX = 4.0f;
constexpr float SSAO_BRIGHTNESS_STEP = 0.05f;
constexpr float SSAO_SATURATION_BOOST_MIN = 0.0f;
constexpr float SSAO_SATURATION_BOOST_MAX = 1.0f;
constexpr float SSAO_SATURATION_BOOST_STEP = 0.05f;
constexpr float HALF_LAMBERT_SHADOW_SATURATION_MIN = 0.0f;
constexpr float HALF_LAMBERT_SHADOW_SATURATION_MAX = 2.0f;
constexpr float HALF_LAMBERT_SHADOW_SATURATION_STEP = 0.05f;
constexpr float SHADOW_DARKNESS_MIN = 0.0f;
constexpr float SHADOW_DARKNESS_MAX = 1.0f;
constexpr float SHADOW_DARKNESS_STEP = 0.05f;
constexpr float SPECULAR_INTENSITY_MIN = 0.0f;
constexpr float SPECULAR_INTENSITY_MAX = 2.0f;
constexpr float SPECULAR_INTENSITY_STEP = 0.05f;
constexpr float SPECULAR_EDGE_MIN = 0.0f;
constexpr float SPECULAR_EDGE_MAX = 1.0f;
constexpr float SPECULAR_EDGE_STEP = 0.05f;
constexpr float BLOOM_THRESHOLD_MIN = 0.0f;
constexpr float BLOOM_THRESHOLD_MAX = 5.0f;
constexpr float BLOOM_THRESHOLD_STEP = 0.1f;
constexpr float DOF_FOCAL_DISTANCE_MIN = 0.5f;
constexpr float DOF_FOCAL_DISTANCE_MAX = 50.0f;
constexpr float DOF_FOCAL_DISTANCE_STEP = 0.1f;
constexpr float STARBURST_THRESHOLD_MIN = 0.0f;
constexpr float STARBURST_THRESHOLD_MAX = 5.0f;
constexpr float STARBURST_THRESHOLD_STEP = 0.1f;
constexpr float MODEL_LOAD_SCALE_MIN = 0.1f;
constexpr float MODEL_LOAD_SCALE_MAX = 10.0f;
constexpr float MODEL_LOAD_SCALE_STEP = 0.1f;
constexpr float POINT_LIGHT_COLOR_MIN = 0.0f;
constexpr float POINT_LIGHT_COLOR_MAX = 1.0f;
constexpr float POINT_LIGHT_COLOR_STEP = 0.05f;
constexpr float POINT_LIGHT_BRIGHTNESS_MIN = 0.0f;
constexpr float POINT_LIGHT_BRIGHTNESS_MAX = 5.0f;
constexpr float POINT_LIGHT_BRIGHTNESS_STEP = 0.1f;
constexpr int GAUSSIAN_SAMPLE_MIN = 1;
constexpr int GAUSSIAN_SAMPLE_MAX = 101;
constexpr float GODRAY_LIGHT_COLOR_MIN = 0.0f;
constexpr float GODRAY_LIGHT_COLOR_MAX = 1.0f;
constexpr float GODRAY_LIGHT_COLOR_STEP = 0.05f;
constexpr float GODRAY_INTENSITY_MIN = 0.0f;
constexpr float GODRAY_INTENSITY_MAX = 3.0f;
constexpr float GODRAY_INTENSITY_STEP = 0.05f;
constexpr float GODRAY_LIGHT_POS_MIN = -200.0f;
constexpr float GODRAY_LIGHT_POS_MAX = 200.0f;
constexpr float GODRAY_LIGHT_POS_STEP = 1.0f;

struct ImageInfo
{
    std::wstring m_imageName;
    RECT m_rect { };
};

struct TextInfo
{
    std::wstring m_text;
    RECT m_rect { };
};

struct LoadedModelInfo
{
    std::wstring m_type;
    std::wstring m_path;
    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    float m_scale = 1.0f;
    int m_renderId = -1;
};

enum class MixMeshShaderMode
{
    None,
    ParallaxOcclusionMapping,
    NormalMapping,
};

extern bool g_bClose;
extern NSRender::Render g_Render;
extern int g_fontId;
extern bool g_bRecenteringMouse;
extern bool g_bMouseLookEnabled;
extern bool g_bPrevMouseClientPosValid;
extern POINT g_prevMouseClientPos;
extern bool g_bMoveForward;
extern bool g_bMoveBackward;
extern bool g_bMoveLeft;
extern bool g_bMoveRight;
extern bool g_bMoveUp;
extern bool g_bMoveDown;
extern float g_saturateLevel;
extern HWND g_hSettingsDialog;
extern std::wstring g_selectedMixMeshPath;
extern std::wstring g_selectedMeshPath;
extern std::wstring g_selectedAnimMeshPath;
extern std::wstring g_selectedSkinAnimMeshPath;
extern std::wstring g_selectedMixSkinAnimMeshPath;
extern bool g_bAnimateLight;
extern bool g_bRemoteDesktop;
extern bool g_bGaussianFilter;
extern bool g_bDepthBufferShadow;
extern bool g_bSSAO;
extern bool g_bFog;
extern bool g_bSaturateFilter;
extern bool g_bBloom;
extern bool g_bDepthOfField;
extern bool g_bStarBurst;
extern bool g_bShowOverlay;
extern float g_fogIntensity;
extern float g_sunLightIntensity;
extern float g_shadowIntensity;
extern float g_shadowSaturationBoost;
extern float g_ssaoBrightness;
extern float g_ssaoSaturationBoost;
extern float g_halfLambertShadowSaturation;
extern float g_shadowDarkness;
extern float g_specularIntensity;
extern float g_specularEdge;
extern bool g_bUseSpecularEdgeOverride;
extern float g_bloomThreshold;
extern float g_dofFocalDistance;
extern float g_starBurstThreshold;
extern float g_modelLoadScale;
extern D3DXCOLOR g_pointLightColor;
extern float g_pointLightBrightness;
extern int g_gaussianSampleSize;
extern int g_sunId;
extern int g_resolutionWidth;
extern int g_resolutionHeight;
extern NSRender::eWindowMode g_windowMode;
extern std::vector<ImageInfo> g_imageInfoList;
extern std::vector<TextInfo> g_textInfoList;
extern std::vector<LoadedModelInfo> g_loadedModelList;
extern MixMeshShaderMode g_mixMeshShaderMode;
extern bool g_bGodRay;
extern D3DXVECTOR3 g_godRayLightColor;
extern float g_godRayIntensity;
extern D3DXVECTOR3 g_godRayLightPos;
extern int g_godRayMarkerMeshId;

void UpdateCameraMoveByKeyboard();
void MoveCameraAwayFromLookAtByWheel(short wheelDelta);
POINT GetClientCenter(HWND hWnd);
void RecenterMouseCursor(HWND hWnd);
void HideMouseCursor();
void ShowMouseCursor();
void EnableMouseLook(HWND hWnd);
void DisableMouseLook();

void ApplySaturateLevel();
void ApplyPostEffectToggleSettings();
void ApplyFogIntensity();
void ApplySunLightIntensity();
void ApplyShadowIntensity();
void ApplyShadowSaturationBoost();
void ApplySSAOBrightness();
void ApplySSAOSaturationBoost();
void ApplyHalfLambertShadowSaturation();
void ApplyShadowDarkness();
void ApplySpecularIntensity();
void ApplySpecularEdge();
void ApplySpecularEdgeOverride();
void ApplyBloomThreshold();
void ApplyDepthOfFieldFocalDistance();
void ApplyStarBurstThreshold();
void ApplyModelLoadScale();
void ApplyPointLightColor();
void ApplyPointLightBrightness();
void ApplyGaussianSampleSize();
void ApplyResolution();
void ApplyWindowMode();
void InitializeRemoteDesktopDefault();
int SaturateLevelToSliderValue(float level);
float SliderValueToSaturateLevel(int sliderValue);
int FogIntensityToSliderValue(float intensity);
float SliderValueToFogIntensity(int sliderValue);
int SunLightIntensityToSliderValue(float intensity);
float SliderValueToSunLightIntensity(int sliderValue);
int ShadowIntensityToSliderValue(float intensity);
float SliderValueToShadowIntensity(int sliderValue);
int ShadowSaturationBoostToSliderValue(float boost);
float SliderValueToShadowSaturationBoost(int sliderValue);
int SSAOBrightnessToSliderValue(float brightness);
float SliderValueToSSAOBrightness(int sliderValue);
int SSAOSaturationBoostToSliderValue(float boost);
float SliderValueToSSAOSaturationBoost(int sliderValue);
int HalfLambertShadowSaturationToSliderValue(float boost);
float SliderValueToHalfLambertShadowSaturation(int sliderValue);
int ShadowDarknessToSliderValue(float darkness);
float SliderValueToShadowDarkness(int sliderValue);
int SpecularIntensityToSliderValue(float intensity);
float SliderValueToSpecularIntensity(int sliderValue);
int SpecularEdgeToSliderValue(float edge);
float SliderValueToSpecularEdge(int sliderValue);
int BloomThresholdToSliderValue(float threshold);
float SliderValueToBloomThreshold(int sliderValue);
int DepthOfFieldFocalDistanceToSliderValue(float distance);
float SliderValueToDepthOfFieldFocalDistance(int sliderValue);
int StarBurstThresholdToSliderValue(float threshold);
float SliderValueToStarBurstThreshold(int sliderValue);
int ModelLoadScaleToSliderValue(float scale);
float SliderValueToModelLoadScale(int sliderValue);
int PointLightColorToSliderValue(float value);
float SliderValueToPointLightColor(int sliderValue);
int PointLightBrightnessToSliderValue(float brightness);
float SliderValueToPointLightBrightness(int sliderValue);
int GaussianSampleSizeToSliderValue(int sampleSize);
int SliderValueToGaussianSampleSize(int sliderValue);
void ApplyGodRay();
void ApplyGodRayLightColor();
void ApplyGodRayIntensity();
void ApplyGodRayLightPos();
int GodRayLightColorToSliderValue(float value);
float SliderValueToGodRayLightColor(int sliderValue);
int GodRayIntensityToSliderValue(float intensity);
float SliderValueToGodRayIntensity(int sliderValue);
int GodRayLightPosToSliderValue(float pos);
float SliderValueToGodRayLightPos(int sliderValue);
void RegisterLoadedModel(const std::wstring& type,
                         const std::wstring& path,
                         const D3DXVECTOR3& pos,
                         float scale,
                         int renderId);
bool RemoveLoadedModel(size_t modelIndex);

void SpawnMeshAtLookAt(const std::wstring& filePath);
void SpawnMeshMixAtLookAt(const std::wstring& filePath);
void SpawnAnimMeshAtLookAt(const std::wstring& filePath);
void SpawnSkinAnimMeshAtLookAt(const std::wstring& filePath);
void SpawnMeshMixSkinAnimAtLookAt(const std::wstring& filePath);
NSRender::AnimSetMap CreateDefaultAnimSetMap();
bool ShowOpenFileDialog(HWND hWnd, const wchar_t* filter, std::wstring& selectedPath);
bool ShowSaveBinaryXFileDialog(HWND hWnd, const std::wstring& sourcePath, std::wstring& selectedPath);
bool ExportLoadedModelAsBinaryX(size_t modelIndex, const std::wstring& outputPath);
void LoadSampleSettingsFromCsv(const std::wstring& settingsCsvPath);
void AddPointLightAtLookAt();

void DrawSampleOverlay();
void UpdateDirectionalLight();
