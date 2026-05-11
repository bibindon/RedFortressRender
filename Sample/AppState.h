#pragma once

#include "../Render/Render.h"

#include <string>
#include <vector>
#include <windows.h>

// AppState.h はサンプル全体で共有する実行時状態と、
// その状態を Render や UI へ反映する操作関数をまとめた中心ヘッダ。
// 入力、ダイアログ、メインループが同じ値を参照する前提になっている。

constexpr int WINDOW_SIZE_W = 1600;
constexpr int WINDOW_SIZE_H = 900;
constexpr float MOUSE_CAMERA_SENSITIVITY = 0.005f;
constexpr float MODEL_SPAWN_FORWARD_OFFSET = 6.0f;
constexpr float MOUSE_WHEEL_CAMERA_DISTANCE = 0.1f;
constexpr float SATURATE_MIN = 0.0f;
constexpr float SATURATE_MAX = 4.0f;
constexpr float SATURATE_STEP = 0.1f;
constexpr float FOG_INTENSITY_MIN = 0.0f;
constexpr float FOG_INTENSITY_MAX = 20.0f;
constexpr float FOG_INTENSITY_STEP = 0.1f;
constexpr float HEIGHT_FOG_INTENSITY_MIN = 0.0f;
constexpr float HEIGHT_FOG_INTENSITY_MAX = 5.0f;
constexpr float HEIGHT_FOG_INTENSITY_STEP = 0.05f;
constexpr float HEIGHT_FOG_HEIGHT_MIN = -100.0f;
constexpr float HEIGHT_FOG_HEIGHT_MAX = 100.0f;
constexpr float HEIGHT_FOG_HEIGHT_STEP = 0.5f;
constexpr float HEIGHT_FOG_DISTANCE_MIN = 0.0f;
constexpr float HEIGHT_FOG_DISTANCE_MAX = 200.0f;
constexpr float HEIGHT_FOG_DISTANCE_STEP = 0.5f;
constexpr float SUN_LIGHT_INTENSITY_MIN = 0.0f;
constexpr float SUN_LIGHT_INTENSITY_MAX = 5.0f;
constexpr float SUN_LIGHT_INTENSITY_STEP = 0.1f;
constexpr float SUN_LIGHT_COLOR_MIN = 0.0f;
constexpr float SUN_LIGHT_COLOR_MAX = 1.0f;
constexpr float SUN_LIGHT_COLOR_STEP = 0.05f;
constexpr float AMBIENT_LIGHT_INTENSITY_MIN = 0.0f;
constexpr float AMBIENT_LIGHT_INTENSITY_MAX = 5.0f;
constexpr float AMBIENT_LIGHT_INTENSITY_STEP = 0.1f;
constexpr float AMBIENT_LIGHT_COLOR_MIN = 0.0f;
constexpr float AMBIENT_LIGHT_COLOR_MAX = 1.0f;
constexpr float AMBIENT_LIGHT_COLOR_STEP = 0.05f;
constexpr float SHADOW_INTENSITY_MIN = 0.0f;
constexpr float SHADOW_INTENSITY_MAX = 1.0f;
constexpr float SHADOW_INTENSITY_STEP = 0.05f;
constexpr float SHADOW_COVERAGE_MIN = 0.0f;
constexpr float SHADOW_COVERAGE_MAX = 1.0f;
constexpr float SHADOW_COVERAGE_STEP = 0.05f;
constexpr float SHADOW_SATURATION_BOOST_MIN = 0.0f;
constexpr float SHADOW_SATURATION_BOOST_MAX = 1.0f;
constexpr float SHADOW_SATURATION_BOOST_STEP = 0.05f;
constexpr float SSAO2_SHADOW_STRENGTH_MIN = 0.0f;
constexpr float SSAO2_SHADOW_STRENGTH_MAX = 4.0f;
constexpr float SSAO2_SHADOW_STRENGTH_STEP = 0.05f;
constexpr float SSAO2_SHADOW_SATURATION_BOOST_MIN = 0.0f;
constexpr float SSAO2_SHADOW_SATURATION_BOOST_MAX = 5.0f;
constexpr float SSAO2_SHADOW_SATURATION_BOOST_STEP = 0.05f;
constexpr int SSAO2_SAMPLE_COUNT_MIN = 1;
constexpr int SSAO2_SAMPLE_COUNT_MAX = 64;
constexpr float SSAO2_SAMPLE_RADIUS_MIN = 0.05f;
constexpr float SSAO2_SAMPLE_RADIUS_MAX = 10.0f;
constexpr float SSAO2_SAMPLE_RADIUS_STEP = 0.05f;
constexpr float CAMERA_NEAR_MIN = 0.01f;
constexpr float CAMERA_NEAR_MAX = 1000.0f;
constexpr float CAMERA_FAR_MIN = 1.0f;
constexpr float CAMERA_FAR_MAX = 30000.0f;
constexpr float GBUFFER_NEAR_MIN = 0.01f;
constexpr float GBUFFER_NEAR_MAX = 1000.0f;
constexpr float GBUFFER_FAR_MIN = 1.0f;
constexpr float GBUFFER_FAR_MAX = 30000.0f;
constexpr float HALF_LAMBERT_SHADOW_SATURATION_MIN = 0.0f;
constexpr float HALF_LAMBERT_SHADOW_SATURATION_MAX = 10.0f;
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
constexpr float SSS_INTENSITY_MIN = 0.0f;
constexpr float SSS_INTENSITY_MAX = 30.0f;
constexpr float SSS_INTENSITY_STEP = 0.25f;
constexpr float SSS_COLOR_MIN = 0.0f;
constexpr float SSS_COLOR_MAX = 1.0f;
constexpr float SSS_COLOR_STEP = 0.05f;
constexpr float BLOOM_THRESHOLD_MIN = 0.0f;
constexpr float BLOOM_THRESHOLD_MAX = 5.0f;
constexpr float BLOOM_THRESHOLD_STEP = 0.1f;
constexpr float DOF_FOCAL_DISTANCE_MIN = 0.5f;
constexpr float DOF_FOCAL_DISTANCE_MAX = 50.0f;
constexpr float DOF_FOCAL_DISTANCE_STEP = 0.1f;
constexpr float DOF_MAX_BLUR_DISTANCE_MIN = 0.5f;
constexpr float DOF_MAX_BLUR_DISTANCE_MAX = 50.0f;
constexpr float DOF_MAX_BLUR_DISTANCE_STEP = 0.1f;
constexpr float DOF_AUTO_ACTIVATION_DISTANCE_MIN = 0.5f;
constexpr float DOF_AUTO_ACTIVATION_DISTANCE_MAX = 50.0f;
constexpr float DOF_AUTO_ACTIVATION_DISTANCE_STEP = 0.1f;
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
constexpr float POINT_LIGHT_LINE_LENGTH_MIN = 0.1f;
constexpr float POINT_LIGHT_LINE_LENGTH_MAX = 100.0f;
constexpr float POINT_LIGHT_SQUARE_SIZE_MIN = 0.1f;
constexpr float POINT_LIGHT_SQUARE_SIZE_MAX = 100.0f;
constexpr float POINT_LIGHT_ROTATION_MIN_DEGREES = -180.0f;
constexpr float POINT_LIGHT_ROTATION_MAX_DEGREES = 180.0f;
constexpr int GAUSSIAN_SAMPLE_MIN = 1;
constexpr int GAUSSIAN_SAMPLE_MAX = 101;
constexpr int FXAA_QUALITY_MIN = 1;
constexpr int FXAA_QUALITY_MAX = 8;
constexpr int MOTION_BLUR_CAMERA_QUALITY_MIN = 1;
constexpr int MOTION_BLUR_CAMERA_QUALITY_MAX = 8;
constexpr float MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_MIN = 1.0f;
constexpr float MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_MAX = 64.0f;
constexpr float MOTION_BLUR_CAMERA_MAX_BLUR_PIXELS_STEP = 1.0f;
constexpr int MOTION_BLUR_CAMERA_SAMPLE_COUNT_MIN = 2;
constexpr int MOTION_BLUR_CAMERA_SAMPLE_COUNT_MAX = 21;
constexpr int SHADOW_BLUR_TAP_COUNT_MIN = 1;
constexpr int SHADOW_BLUR_TAP_COUNT_MAX = 11;
constexpr float GODRAY_LIGHT_COLOR_MIN = 0.0f;
constexpr float GODRAY_LIGHT_COLOR_MAX = 1.0f;
constexpr float GODRAY_LIGHT_COLOR_STEP = 0.05f;
constexpr float GODRAY_INTENSITY_MIN = 0.0f;
constexpr float GODRAY_INTENSITY_MAX = 3.0f;
constexpr float GODRAY_INTENSITY_STEP = 0.05f;
constexpr float GODRAY_VIRTUAL_PROXIMITY_MIN = 0.0f;
constexpr float GODRAY_VIRTUAL_PROXIMITY_MAX = 5.0f;
constexpr float GODRAY_VIRTUAL_PROXIMITY_STEP = 0.05f;
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

// ここから下の extern 変数群は「現在のサンプル状態そのもの」を表す。
// 描画設定、入力状態、生成済みモデル一覧までを共有し、
// 各 subsystem が同じ情報源を読む設計にしている。
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
extern std::wstring g_selectedMaskedGaussianMaskPath;
extern bool g_bAnimateLight;
extern bool g_bRemoteDesktop;
extern bool g_bGaussianFilter;
extern bool g_bMaskedGaussianFilter;
extern bool g_bFXAA;
extern bool g_bMotionBlurCamera;
extern float g_motionBlurCameraMaxBlurPixels;
extern int g_motionBlurCameraSampleCount;
extern bool g_bDepthBufferShadow;
extern bool g_bGBuffer;
extern bool g_bSSAO2;
extern bool g_bSSAO2Blur;
extern bool g_bSSAO2DepthScaledSampleDistance;
extern bool g_bFog;
extern bool g_bHeightFog;
extern bool g_bSaturateFilter;
extern bool g_bBloom;
extern NSRender::DepthOfFieldMode g_depthOfFieldMode;
extern bool g_bStarBurst;
extern NSRender::DebugGBufferView g_debugGBufferView;
extern bool g_bShowOverlay;
extern float g_fogIntensity;
extern float g_heightFogIntensity;
extern float g_heightFogStart;
extern float g_heightFogMax;
extern float g_heightFogDistanceStart;
extern float g_heightFogDistanceMax;
extern float g_sunLightIntensity;
extern D3DXCOLOR g_sunLightColor;
extern float g_ambientLightIntensity;
extern D3DXCOLOR g_ambientLightColor;
extern float g_shadowIntensity;
extern float g_shadowCoverage;
extern float g_shadowSaturationBoost;
extern float g_ssao2ShadowStrength;
extern float g_ssao2ShadowSaturationBoost;
extern int g_ssao2SampleCount;
extern float g_ssao2SampleRadius;
extern float g_cameraNearPlane;
extern float g_cameraFarPlane;
extern float g_gbufferNearPlane;
extern float g_gbufferFarPlane;
extern float g_halfLambertShadowSaturation;
extern float g_shadowDarkness;
extern float g_specularIntensity;
extern float g_specularEdge;
extern bool g_bUseSpecularIntensityOverride;
extern bool g_bUseSpecularEdgeOverride;
extern bool g_bSSS;
extern float g_sssIntensity;
extern D3DXCOLOR g_sssColor;
extern float g_bloomThreshold;
extern float g_dofFocalDistance;
extern float g_dofMaxBlurDistance;
extern float g_dofAutoActivationDistance;
extern float g_starBurstThreshold;
extern float g_modelLoadScale;
extern D3DXCOLOR g_pointLightColor;
extern float g_pointLightBrightness;
extern NSRender::PointLightShape g_pointLightShape;
extern float g_pointLightLineLength;
extern float g_pointLightSquareWidth;
extern float g_pointLightSquareHeight;
extern D3DXVECTOR3 g_pointLightRotationDegrees;
extern int g_gaussianSampleSize;
extern int g_fxaaQuality;
extern int g_motionBlurCameraQuality;
extern int g_shadowPcfTapCount;
extern int g_shadowCompositeTapCount;
extern int g_zShadowTexSizeDivisor;
extern int g_ssao2TexSizeDivisor;
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
extern float g_godRayVirtualProximityStrength;
extern D3DXVECTOR3 g_godRayLightPos;
extern int g_godRaySourceMarkerMeshId;
extern int g_godRayEffectiveMarkerMeshId;

// カメラ・マウス関連の補助関数。
// 生入力を実際の視点移動へ変換する役割を持つ。
void UpdateCameraMoveByKeyboard();
void MoveCameraAwayFromLookAtByWheel(short wheelDelta);
POINT GetClientCenter(HWND hWnd);
void RecenterMouseCursor(HWND hWnd);
void HideMouseCursor();
void ShowMouseCursor();
void EnableMouseLook(HWND hWnd);
void DisableMouseLook();

// Apply 系関数は AppState に蓄えた値を Render 側の実設定へ反映する。
// UI で値を書き換えただけでは描画内容は変わらないため、
// 変更確定時にこれらを呼ぶ前提になっている。
void ApplySaturateLevel();
void ApplyPostEffectToggleSettings();
void ApplyMaskedGaussianMaskPath();
void ApplyFogIntensity();
void ApplyHeightFogIntensity();
void ApplyHeightFogStart();
void ApplyHeightFogMax();
void ApplyHeightFogDistanceStart();
void ApplyHeightFogDistanceMax();
void ApplySunLightIntensity();
void ApplySunLightColor();
void ApplyAmbientLightIntensity();
void ApplyAmbientLightColor();
void ApplyShadowIntensity();
void ApplyShadowCoverage();
void ApplyShadowSaturationBoost();
void ApplyShadowPcfTapCount();
void ApplyShadowCompositeTapCount();
void ApplyZShadowTexSize();
void ApplySSAO2ShadowStrength();
void ApplySSAO2ShadowSaturationBoost();
void ApplySSAO2SampleCount();
void ApplySSAO2DepthScaledSampleDistance();
void ApplySSAO2SampleRadius();
void ApplySSAO2TexSize();
void ApplyCameraClipPlanes();
void ApplyGBufferClipPlanes();
void ApplySSAO2Blur();
void ApplyHalfLambertShadowSaturation();
void ApplyShadowDarkness();
void ApplySpecularIntensity();
void ApplySpecularIntensityOverride();
void ApplySpecularEdge();
void ApplySpecularEdgeOverride();
void ApplySSS();
void ApplySSSIntensity();
void ApplySSSColor();
void ApplyBloomThreshold();
void ApplyDepthOfFieldMode();
void ApplyDepthOfFieldFocalDistance();
void ApplyDepthOfFieldMaxBlurDistance();
void ApplyDepthOfFieldAutoActivationDistance();
void ApplyStarBurstThreshold();
void ApplyModelLoadScale();
void ApplyPointLightColor();
void ApplyPointLightBrightness();
void ApplyPointLightShape();
void ApplyPointLightLineSettings();
void ApplyPointLightSquareSettings();
void ApplyGaussianSampleSize();
void ApplyFXAAQuality();
void ApplyMotionBlurCameraSettings();
void ApplyResolution();
void ApplyWindowMode();
void InitializeRemoteDesktopDefault();

// Slider 変換関数は整数スライダー値と内部 float/int 値の往復を担当する。
// step 幅や clamp の知識を UI 実装から分離するためにまとめている。
int SaturateLevelToSliderValue(float level);
float SliderValueToSaturateLevel(int sliderValue);
int SunLightColorToSliderValue(float value);
float SliderValueToSunLightColor(int sliderValue);
int FogIntensityToSliderValue(float intensity);
float SliderValueToFogIntensity(int sliderValue);
int HeightFogIntensityToSliderValue(float intensity);
float SliderValueToHeightFogIntensity(int sliderValue);
int HeightFogHeightToSliderValue(float height);
float SliderValueToHeightFogHeight(int sliderValue);
int HeightFogDistanceToSliderValue(float distance);
float SliderValueToHeightFogDistance(int sliderValue);
int SunLightIntensityToSliderValue(float intensity);
float SliderValueToSunLightIntensity(int sliderValue);
int AmbientLightIntensityToSliderValue(float intensity);
float SliderValueToAmbientLightIntensity(int sliderValue);
int ShadowIntensityToSliderValue(float intensity);
float SliderValueToShadowIntensity(int sliderValue);
int ShadowCoverageToSliderValue(float coverage);
float SliderValueToShadowCoverage(int sliderValue);
int ShadowSaturationBoostToSliderValue(float boost);
float SliderValueToShadowSaturationBoost(int sliderValue);
int ShadowTapCountToSliderValue(int tapCount);
int SliderValueToShadowTapCount(int sliderValue);
int SSAO2ShadowStrengthToSliderValue(float shadowStrength);
float SliderValueToSSAO2ShadowStrength(int sliderValue);
int SSAO2ShadowSaturationBoostToSliderValue(float boost);
float SliderValueToSSAO2ShadowSaturationBoost(int sliderValue);
int SSAO2SampleCountToSliderValue(int sampleCount);
int SliderValueToSSAO2SampleCount(int sliderValue);
int SSAO2SampleRadiusToSliderValue(float sampleRadius);
float SliderValueToSSAO2SampleRadius(int sliderValue);
int HalfLambertShadowSaturationToSliderValue(float boost);
float SliderValueToHalfLambertShadowSaturation(int sliderValue);
int ShadowDarknessToSliderValue(float darkness);
float SliderValueToShadowDarkness(int sliderValue);
int SpecularIntensityToSliderValue(float intensity);
float SliderValueToSpecularIntensity(int sliderValue);
int SpecularEdgeToSliderValue(float edge);
float SliderValueToSpecularEdge(int sliderValue);
int SSSIntensityToSliderValue(float intensity);
float SliderValueToSSSIntensity(int sliderValue);
int SSSColorToSliderValue(float value);
float SliderValueToSSSColor(int sliderValue);
int BloomThresholdToSliderValue(float threshold);
float SliderValueToBloomThreshold(int sliderValue);
int DepthOfFieldFocalDistanceToSliderValue(float distance);
float SliderValueToDepthOfFieldFocalDistance(int sliderValue);
int DepthOfFieldMaxBlurDistanceToSliderValue(float distance);
float SliderValueToDepthOfFieldMaxBlurDistance(int sliderValue);
int DepthOfFieldAutoActivationDistanceToSliderValue(float distance);
float SliderValueToDepthOfFieldAutoActivationDistance(int sliderValue);
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
int FXAAQualityToSliderValue(int quality);
int SliderValueToFXAAQuality(int sliderValue);
int MotionBlurCameraQualityToSliderValue(int quality);
int SliderValueToMotionBlurCameraQuality(int sliderValue);
int MotionBlurCameraMaxBlurPixelsToSliderValue(float maxBlurPixels);
float SliderValueToMotionBlurCameraMaxBlurPixels(int sliderValue);
int MotionBlurCameraSampleCountToSliderValue(int sampleCount);
int SliderValueToMotionBlurCameraSampleCount(int sliderValue);
void ApplyGodRay();
void ApplyGodRayLightColor();
void ApplyGodRayIntensity();
void ApplyGodRayVirtualProximityStrength();
void ApplyGodRayLightPos();
int GodRayLightColorToSliderValue(float value);
float SliderValueToGodRayLightColor(int sliderValue);
int GodRayIntensityToSliderValue(float intensity);
float SliderValueToGodRayIntensity(int sliderValue);
int GodRayVirtualProximityStrengthToSliderValue(float intensity);
float SliderValueToGodRayVirtualProximityStrength(int sliderValue);
int GodRayLightPosToSliderValue(float pos);
float SliderValueToGodRayLightPos(int sliderValue);

// シーンやアセット管理系の関数群。
// 入力ショートカットや設定ダイアログから呼ばれ、描画オブジェクト実体を増減させる。
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
bool ShowOpenFileDialog(HWND hWnd, const wchar_t* filter, std::wstring& selectedPath, const wchar_t* defaultExt = L"x");
bool ShowSaveBinaryXFileDialog(HWND hWnd, const std::wstring& sourcePath, std::wstring& selectedPath);
bool ExportLoadedModelAsBinaryX(size_t modelIndex, const std::wstring& outputPath);
bool LoadXFileListFromCsv(const std::wstring& csvPath, int* loadedCount = nullptr, int* skippedCount = nullptr);
void LoadSampleSettingsFromCsv(const std::wstring& settingsCsvPath);
void ApplyAllSampleSettings();
void ApplyGBufferEnable();
bool ReloadRenderSettingsFromCsv(const std::wstring& settingsCsvPath);
void AddPointLightAtLookAt();

// Overlay / 補助描画系。
// Render の 2D 描画を利用してサンプル用の情報表示を行う。
void DrawSampleOverlay();
void UpdateDirectionalLight();
