#include "AppState.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <commdlg.h>
#include <cwchar>
#include <cstdlib>
#include <fstream>
#include <cwctype>
#include <windowsx.h>

bool g_bClose = false;
NSRender::Render g_Render;
int g_fontId = 0;
bool g_bRecenteringMouse = false;
bool g_bMouseLookEnabled = false;
bool g_bMoveForward = false;
bool g_bMoveBackward = false;
bool g_bMoveLeft = false;
bool g_bMoveRight = false;
bool g_bMoveUp = false;
bool g_bMoveDown = false;
float g_saturateLevel = 1.0f;
HWND g_hSettingsDialog = NULL;
std::wstring g_selectedMixMeshPath;
std::wstring g_selectedMeshPath;
std::wstring g_selectedAnimMeshPath;
std::wstring g_selectedSkinAnimMeshPath;
bool g_bAnimateLight = false;
bool g_bGaussianFilter = false;
bool g_bDepthBufferShadow = true;
bool g_bSSAO = true;
bool g_bFog = true;
bool g_bSaturateFilter = false;
bool g_bBloom = false;
bool g_bStarBurst = false;
float g_fogIntensity = 2.0f;
int g_gaussianSampleSize = 101;
int g_sunId = 0;
std::vector<ImageInfo> g_imageInfoList;
std::vector<TextInfo> g_textInfoList;

namespace
{
float ClampSaturateLevel(const float level)
{
    return (std::max)(SATURATE_MIN, (std::min)(level, SATURATE_MAX));
}

float ClampFogIntensity(const float intensity)
{
    return (std::max)(FOG_INTENSITY_MIN, (std::min)(intensity, FOG_INTENSITY_MAX));
}

std::wstring Trim(const std::wstring& text)
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

int NormalizeGaussianSampleSizeLocal(const int sampleSize)
{
    int normalized = (std::max)(GAUSSIAN_SAMPLE_MIN, (std::min)(sampleSize, GAUSSIAN_SAMPLE_MAX));
    if ((normalized % 2) == 0)
    {
        --normalized;
    }
    return (std::max)(GAUSSIAN_SAMPLE_MIN, normalized);
}

void DrawRandomized2DContent()
{
    for (auto& elem : g_textInfoList)
    {
        g_Render.DrawText_(g_fontId,
                           elem.m_text,
                           elem.m_rect.left,
                           elem.m_rect.top,
                           0xFFAA88FF);
    }

    for (auto& elem : g_imageInfoList)
    {
        g_Render.DrawImage(elem.m_imageName, elem.m_rect.left, elem.m_rect.top);
    }
}
}

void UpdateCameraMoveByKeyboard()
{
    if (!g_bMoveForward && !g_bMoveBackward && !g_bMoveLeft && !g_bMoveRight && !g_bMoveUp && !g_bMoveDown)
    {
        return;
    }

    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    D3DXVECTOR3 worldUp(0, 1, 0);
    D3DXVECTOR3 right;
    D3DXVec3Cross(&right, &worldUp, &forward);
    D3DXVec3Normalize(&right, &right);

    D3DXVECTOR3 move(0.0f, 0.0f, 0.0f);

    if (g_bMoveForward)
    {
        move += forward;
    }
    if (g_bMoveBackward)
    {
        move -= forward;
    }
    if (g_bMoveRight)
    {
        move += right;
    }
    if (g_bMoveLeft)
    {
        move -= right;
    }
    if (g_bMoveUp)
    {
        move += worldUp;
    }
    if (g_bMoveDown)
    {
        move -= worldUp;
    }

    if (D3DXVec3LengthSq(&move) <= 0.0f)
    {
        return;
    }

    D3DXVec3Normalize(&move, &move);

    const float speed = 0.2f;
    g_Render.MoveCamera(move * speed);
}

void MoveCameraAwayFromLookAtByWheel(const short wheelDelta)
{
    if (wheelDelta == 0)
    {
        return;
    }

    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float notchCount = static_cast<float>(-wheelDelta) / WHEEL_DELTA;
    const D3DXVECTOR3 lookAt = g_Render.GetLookAtPos();
    const D3DXVECTOR3 eye = g_Render.GetCameraPos();
    const D3DXVECTOR3 newEye = eye - forward * MOUSE_WHEEL_CAMERA_DISTANCE * notchCount;

    g_Render.SetCamera(newEye, lookAt);
}

POINT GetClientCenter(HWND hWnd)
{
    RECT clientRect { };
    GetClientRect(hWnd, &clientRect);

    POINT center { };
    center.x = (clientRect.right - clientRect.left) / 2;
    center.y = (clientRect.bottom - clientRect.top) / 2;
    return center;
}

void RecenterMouseCursor(HWND hWnd)
{
    POINT center = GetClientCenter(hWnd);
    ClientToScreen(hWnd, &center);

    if (SetCursorPos(center.x, center.y))
    {
        g_bRecenteringMouse = true;
    }
}

void HideMouseCursor()
{
    while (ShowCursor(FALSE) >= 0)
    {
    }
}

void ShowMouseCursor()
{
    while (ShowCursor(TRUE) < 0)
    {
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void EnableMouseLook(HWND hWnd)
{
    if (g_bMouseLookEnabled)
    {
        return;
    }

    g_bMouseLookEnabled = true;
    HideMouseCursor();
    SetCursor(NULL);
    RecenterMouseCursor(hWnd);
}

void DisableMouseLook()
{
    if (!g_bMouseLookEnabled)
    {
        return;
    }

    g_bMouseLookEnabled = false;
    g_bRecenteringMouse = false;
    ShowMouseCursor();
}

void ApplySaturateLevel()
{
    g_saturateLevel = ClampSaturateLevel(g_saturateLevel);
    g_Render.SetPostEffectSaturate(g_saturateLevel);
}

void ApplyPostEffectToggleSettings()
{
    g_Render.SetPostEffectDepthBufferShadow(g_bDepthBufferShadow);
    g_Render.SetPostEffectSSAO(g_bSSAO);
    g_Render.SetPostEffectFog(g_bFog);
    g_Render.SetPostEffectSaturateEnable(g_bSaturateFilter);
    g_Render.SetPostEffectGaussianFilter(g_bGaussianFilter);
    g_Render.SetPostEffectBloom(g_bBloom);
    g_Render.SetPostEffectStarBurst(g_bStarBurst);
}

void ApplyFogIntensity()
{
    g_fogIntensity = ClampFogIntensity(g_fogIntensity);
    g_Render.SetPostEffectFogIntensity(g_fogIntensity);
}

void ApplyGaussianSampleSize()
{
    g_gaussianSampleSize = NormalizeGaussianSampleSizeLocal(g_gaussianSampleSize);
    g_Render.SetPostEffectGaussianSampleSize(g_gaussianSampleSize);
}

int SaturateLevelToSliderValue(const float level)
{
    return static_cast<int>(std::lround(ClampSaturateLevel(level) / SATURATE_STEP));
}

float SliderValueToSaturateLevel(const int sliderValue)
{
    return ClampSaturateLevel(static_cast<float>(sliderValue) * SATURATE_STEP);
}

int FogIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampFogIntensity(intensity) / FOG_INTENSITY_STEP));
}

float SliderValueToFogIntensity(const int sliderValue)
{
    return ClampFogIntensity(static_cast<float>(sliderValue) * FOG_INTENSITY_STEP);
}

int GaussianSampleSizeToSliderValue(const int sampleSize)
{
    return (NormalizeGaussianSampleSizeLocal(sampleSize) + 1) / 2;
}

int SliderValueToGaussianSampleSize(const int sliderValue)
{
    return NormalizeGaussianSampleSizeLocal(sliderValue * 2 - 1);
}

void SpawnMeshAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, 1.0f);
}

void SpawnMeshMixAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddMeshMix(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, 1.0f);
}

NSRender::AnimSetMap CreateDefaultAnimSetMap()
{
    NSRender::AnimSetMap animMap;
    NSRender::AnimSetting animSetting;
    animSetting.m_startPos = 0.f;
    animSetting.m_duration = 1.f;
    animSetting.m_loop = true;
    animSetting.m_stopEnd = false;
    animMap[L"0_Idle"] = animSetting;
    return animMap;
}

void SpawnAnimMeshAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddAnimMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, CreateDefaultAnimSetMap());
}

void SpawnSkinAnimMeshAtCameraFront(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddSkinAnimMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f, CreateDefaultAnimSetMap());
}

bool ShowOpenFileDialog(HWND hWnd, const wchar_t* filter, std::wstring& selectedPath)
{
    wchar_t filePath[MAX_PATH] { };

    OPENFILENAMEW ofn { };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = static_cast<DWORD>(_countof(filePath));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"x";

    if (!GetOpenFileNameW(&ofn))
    {
        return false;
    }

    selectedPath = filePath;
    return true;
}

void LoadSampleSettingsFromCsv(const std::wstring& settingsCsvPath)
{
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
        if (key.empty() || value.empty())
        {
            continue;
        }

        try
        {
            if (key == L"GaussianSampleSize")
            {
                g_gaussianSampleSize = std::stoi(value);
            }
            else if (key == L"FogIntensity")
            {
                g_fogIntensity = std::stof(value);
            }
            else if (key == L"DepthBufferShadowEnable")
            {
                g_bDepthBufferShadow = (std::stoi(value) != 0);
            }
            else if (key == L"SSAOEnable")
            {
                g_bSSAO = (std::stoi(value) != 0);
            }
            else if (key == L"FogEnable")
            {
                g_bFog = (std::stoi(value) != 0);
            }
            else if (key == L"SaturateEnable")
            {
                g_bSaturateFilter = (std::stoi(value) != 0);
            }
            else if (key == L"GaussianEnable")
            {
                g_bGaussianFilter = (std::stoi(value) != 0);
            }
            else if (key == L"BloomEnable")
            {
                g_bBloom = (std::stoi(value) != 0);
            }
            else if (key == L"StarBurstEnable")
            {
                g_bStarBurst = (std::stoi(value) != 0);
            }
        }
        catch (...)
        {
        }
    }
}

void DrawSampleOverlay()
{
    std::wstring text;
    text += L"WASD : Camera move\n";
    text += L"Q/E : Camera up/down\n";
    text += L"Arrow keys : Camera rotate\n";
    text += L"Esc : Mouse look ON/OFF\n";
    text += L"\n";
    text += L"8 : Window mode\n";
    text += L"9 : Borderless mode\n";
    text += L"0 : Fullscreen mode\n";
    text += L"\n";
    text += L"c : Add text\n";
    text += L"Shift + c : Clear text\n";
    text += L"p : Add image\n";
    text += L"Shift + p : Clear image\n";
    text += L"Ctrl + p : Add point light\n";
    text += L"\n";
    text += L"m : Add MixMesh\n";
    text += L"Shift + m : Add POM mesh\n";
    text += L"Ctrl + m : Add SSS mesh\n";
    text += L"\n";
    text += L"n : Add anim mesh\n";
    text += L"k : Add skin anim mesh\n";
    text += L"i : Add instancing mesh\n";
    text += L"o : Add point-light mesh\n";
    text += L"Shift + n : Add normal-mapped mesh\n";
    text += L"\n";
    text += L"Shift + s : Saturation up\n";
    text += L"Ctrl + s : Saturation down\n";
    text += L"t : Saturation filter ON/OFF\n";
    text += L"g : Gaussian filter ON/OFF\n";
    text += L"b : Bloom ON/OFF\n";
    text += L"Shift + b : StarBurst ON/OFF\n";
    text += L"h : Depth buffer shadow ON/OFF\n";
    text += L"j : SSAO ON/OFF\n";
    text += L"v : Fog ON/OFF\n";
    text += L"Shift + f : FPS ON/OFF\n";
    g_Render.DrawText_(g_fontId, text, 10, 40);

    DrawRandomized2DContent();
}

void UpdateDirectionalLight()
{
    static float work_f = 0.0f;
    D3DXVECTOR3 lightDir(0.0f, 0.0f, 0.0f);

    if (g_bAnimateLight)
    {
        work_f += 0.02f;
        lightDir.x = sinf(work_f);
        lightDir.z = cosf(work_f);
        lightDir.y = sinf(work_f * 2);
    }
    else
    {
        lightDir = D3DXVECTOR3(1.0f, 1.0f, -1.0f);
    }

    g_Render.SetLightDir(lightDir);
}
