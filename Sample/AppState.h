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
constexpr int GAUSSIAN_SAMPLE_MIN = 1;
constexpr int GAUSSIAN_SAMPLE_MAX = 101;

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

extern bool g_bClose;
extern NSRender::Render g_Render;
extern int g_fontId;
extern bool g_bRecenteringMouse;
extern bool g_bMouseLookEnabled;
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
extern bool g_bAnimateLight;
extern bool g_bGaussianFilter;
extern bool g_bDepthBufferShadow;
extern bool g_bSSAO;
extern bool g_bFog;
extern bool g_bSaturateFilter;
extern bool g_bBloom;
extern bool g_bStarBurst;
extern float g_fogIntensity;
extern int g_gaussianSampleSize;
extern int g_sunId;
extern std::vector<ImageInfo> g_imageInfoList;
extern std::vector<TextInfo> g_textInfoList;

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
void ApplyGaussianSampleSize();
int SaturateLevelToSliderValue(float level);
float SliderValueToSaturateLevel(int sliderValue);
int FogIntensityToSliderValue(float intensity);
float SliderValueToFogIntensity(int sliderValue);
int GaussianSampleSizeToSliderValue(int sampleSize);
int SliderValueToGaussianSampleSize(int sliderValue);

void SpawnMeshAtCameraFront(const std::wstring& filePath);
void SpawnMeshMixAtCameraFront(const std::wstring& filePath);
void SpawnAnimMeshAtCameraFront(const std::wstring& filePath);
void SpawnSkinAnimMeshAtCameraFront(const std::wstring& filePath);
NSRender::AnimSetMap CreateDefaultAnimSetMap();
bool ShowOpenFileDialog(HWND hWnd, const wchar_t* filter, std::wstring& selectedPath);
void LoadSampleSettingsFromCsv(const std::wstring& settingsCsvPath);

void DrawSampleOverlay();
void UpdateDirectionalLight();
