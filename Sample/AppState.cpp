#include "AppState.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <commdlg.h>
#include <cwchar>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <cwctype>
#include <windowsx.h>

#include "SettingsDialog.h"

bool g_bClose = false;
NSRender::Render g_Render;
int g_fontId = 0;
bool g_bRecenteringMouse = false;
bool g_bMouseLookEnabled = false;
bool g_bPrevMouseClientPosValid = false;
POINT g_prevMouseClientPos { };
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
std::wstring g_selectedMixSkinAnimMeshPath;
bool g_bAnimateLight = false;
bool g_bRemoteDesktop = true;
bool g_bGaussianFilter = false;
bool g_bDepthBufferShadow = true;
bool g_bSSAO = true;
bool g_bFog = true;
bool g_bSaturateFilter = false;
bool g_bBloom = false;
bool g_bDepthOfField = false;
bool g_bStarBurst = false;
float g_fogIntensity = 2.0f;
float g_sunLightIntensity = 1.0f;
float g_shadowIntensity = 0.5f;
float g_shadowCoverage = 0.5f;
float g_shadowSaturationBoost = 0.35f;
float g_ssaoBrightness = 3.5f;
float g_ssaoSaturationBoost = 0.30f;
float g_halfLambertShadowSaturation = 1.0f;
float g_shadowDarkness = 0.3f;
float g_specularIntensity = 0.1f;
float g_specularEdge = 0.0f;
bool g_bUseSpecularEdgeOverride = true;
float g_bloomThreshold = 2.5f;
float g_dofFocalDistance = 1.0f;
float g_dofMaxBlurDistance = 8.0f;
float g_starBurstThreshold = 2.8f;
float g_modelLoadScale = 1.0f;
D3DXCOLOR g_pointLightColor = D3DXCOLOR(1.0f, 0.35f, 0.1f, 1.0f);
float g_pointLightBrightness = 1.0f;
int g_gaussianSampleSize = 101;
int g_shadowPcfTapCount = 11;
int g_shadowCompositeTapCount = 11;
int g_sunId = 0;
int g_resolutionWidth = WINDOW_SIZE_W;
int g_resolutionHeight = WINDOW_SIZE_H;
NSRender::eWindowMode g_windowMode = NSRender::eWindowMode::WINDOW;
std::vector<ImageInfo> g_imageInfoList;
std::vector<TextInfo> g_textInfoList;
std::vector<LoadedModelInfo> g_loadedModelList;
MixMeshShaderMode g_mixMeshShaderMode = MixMeshShaderMode::None;
bool g_bGodRay = false;
D3DXVECTOR3 g_godRayLightColor = D3DXVECTOR3(1.0f, 0.9f, 0.8f);
float g_godRayIntensity = 0.6f;
D3DXVECTOR3 g_godRayLightPos = D3DXVECTOR3(0.0f, 50.0f, 50.0f);
int g_godRayMarkerMeshId = -1;

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

float ClampSunLightIntensity(const float intensity)
{
    return (std::max)(SUN_LIGHT_INTENSITY_MIN, (std::min)(intensity, SUN_LIGHT_INTENSITY_MAX));
}

float ClampShadowIntensity(const float intensity)
{
    return (std::max)(SHADOW_INTENSITY_MIN, (std::min)(intensity, SHADOW_INTENSITY_MAX));
}

float ClampShadowCoverage(const float coverage)
{
    return (std::max)(SHADOW_COVERAGE_MIN, (std::min)(coverage, SHADOW_COVERAGE_MAX));
}

float ClampShadowSaturationBoost(const float boost)
{
    return (std::max)(SHADOW_SATURATION_BOOST_MIN, (std::min)(boost, SHADOW_SATURATION_BOOST_MAX));
}

float ClampSSAOBrightness(const float brightness)
{
    return (std::max)(SSAO_BRIGHTNESS_MIN, (std::min)(brightness, SSAO_BRIGHTNESS_MAX));
}

float ClampSSAOSaturationBoost(const float boost)
{
    return (std::max)(SSAO_SATURATION_BOOST_MIN, (std::min)(boost, SSAO_SATURATION_BOOST_MAX));
}

float ClampHalfLambertShadowSaturation(const float boost)
{
    return (std::max)(HALF_LAMBERT_SHADOW_SATURATION_MIN,
                      (std::min)(boost, HALF_LAMBERT_SHADOW_SATURATION_MAX));
}

float ClampShadowDarkness(const float darkness)
{
    return (std::max)(SHADOW_DARKNESS_MIN, (std::min)(darkness, SHADOW_DARKNESS_MAX));
}

float ClampSpecularIntensity(const float intensity)
{
    return (std::max)(SPECULAR_INTENSITY_MIN, (std::min)(intensity, SPECULAR_INTENSITY_MAX));
}

float ClampSpecularEdge(const float edge)
{
    return (std::max)(SPECULAR_EDGE_MIN, (std::min)(edge, SPECULAR_EDGE_MAX));
}

float ClampBloomThreshold(const float threshold)
{
    return (std::max)(BLOOM_THRESHOLD_MIN, (std::min)(threshold, BLOOM_THRESHOLD_MAX));
}

float ClampDepthOfFieldFocalDistance(const float distance)
{
    return (std::max)(DOF_FOCAL_DISTANCE_MIN, (std::min)(distance, DOF_FOCAL_DISTANCE_MAX));
}

float ClampDepthOfFieldMaxBlurDistance(const float distance)
{
    return (std::max)(DOF_MAX_BLUR_DISTANCE_MIN, (std::min)(distance, DOF_MAX_BLUR_DISTANCE_MAX));
}

float ClampStarBurstThreshold(const float threshold)
{
    return (std::max)(STARBURST_THRESHOLD_MIN, (std::min)(threshold, STARBURST_THRESHOLD_MAX));
}

float ClampModelLoadScale(const float scale)
{
    return (std::max)(MODEL_LOAD_SCALE_MIN, (std::min)(scale, MODEL_LOAD_SCALE_MAX));
}

float ClampPointLightColor(const float value)
{
    return (std::max)(POINT_LIGHT_COLOR_MIN, (std::min)(value, POINT_LIGHT_COLOR_MAX));
}

float ClampPointLightBrightness(const float brightness)
{
    return (std::max)(POINT_LIGHT_BRIGHTNESS_MIN, (std::min)(brightness, POINT_LIGHT_BRIGHTNESS_MAX));
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

int NormalizeShadowBlurTapCountLocal(const int tapCount)
{
    int normalized = (std::max)(SHADOW_BLUR_TAP_COUNT_MIN, (std::min)(tapCount, SHADOW_BLUR_TAP_COUNT_MAX));
    if ((normalized % 2) == 0)
    {
        --normalized;
    }
    return (std::max)(SHADOW_BLUR_TAP_COUNT_MIN, normalized);
}

bool IsWeekdayBusinessHours()
{
    SYSTEMTIME localTime { };
    GetLocalTime(&localTime);

    const bool isWeekday = localTime.wDayOfWeek >= 1 && localTime.wDayOfWeek <= 5;
    const bool isBusinessHours = localTime.wHour >= 9 && localTime.wHour < 18;
    return isWeekday && isBusinessHours;
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

char* DuplicateAnsiString(const char* text)
{
    if (text == nullptr)
    {
        return nullptr;
    }

    const size_t length = strlen(text);
    char* copy = NEW char[length + 1];
    strcpy_s(copy, length + 1, text);
    return copy;
}

struct ExportXFrame : public D3DXFRAME
{
    D3DXMATRIX m_combinedMatrix;
};

struct ExportXMeshContainer : public D3DXMESHCONTAINER
{
};

class ExportXHierarchyAllocator : public ID3DXAllocateHierarchy
{
public:
    STDMETHOD(CreateFrame)(LPCSTR name, LPD3DXFRAME* newFrame)
    {
        if (newFrame == nullptr)
        {
            return E_INVALIDARG;
        }

        ExportXFrame* frame = NEW ExportXFrame();
        ZeroMemory(frame, sizeof(*frame));
        frame->Name = DuplicateAnsiString(name);
        D3DXMatrixIdentity(&frame->TransformationMatrix);
        D3DXMatrixIdentity(&frame->m_combinedMatrix);

        *newFrame = frame;
        return S_OK;
    }

    STDMETHOD(CreateMeshContainer)(LPCSTR name,
                                   CONST D3DXMESHDATA* meshData,
                                   CONST D3DXMATERIAL* materials,
                                   CONST D3DXEFFECTINSTANCE*,
                                   DWORD materialCount,
                                   CONST DWORD* adjacency,
                                   LPD3DXSKININFO skinInfo,
                                   LPD3DXMESHCONTAINER* meshContainer)
    {
        if (meshData == nullptr || meshContainer == nullptr)
        {
            return E_INVALIDARG;
        }

        if (meshData->Type != D3DXMESHTYPE_MESH || meshData->pMesh == nullptr)
        {
            return E_FAIL;
        }

        ExportXMeshContainer* container = NEW ExportXMeshContainer();
        ZeroMemory(container, sizeof(*container));
        container->Name = DuplicateAnsiString(name);
        container->MeshData.Type = D3DXMESHTYPE_MESH;
        container->MeshData.pMesh = meshData->pMesh;
        container->MeshData.pMesh->AddRef();

        const DWORD faceCount = meshData->pMesh->GetNumFaces();
        if (adjacency != nullptr && faceCount > 0)
        {
            container->pAdjacency = NEW DWORD[faceCount * 3];
            memcpy(container->pAdjacency, adjacency, sizeof(DWORD) * faceCount * 3);
        }

        container->NumMaterials = (std::max)(1UL, materialCount);
        container->pMaterials = NEW D3DXMATERIAL[container->NumMaterials];
        ZeroMemory(container->pMaterials, sizeof(D3DXMATERIAL) * container->NumMaterials);

        if (materialCount > 0 && materials != nullptr)
        {
            for (DWORD i = 0; i < materialCount; ++i)
            {
                container->pMaterials[i].MatD3D = materials[i].MatD3D;
                container->pMaterials[i].pTextureFilename = DuplicateAnsiString(materials[i].pTextureFilename);
            }
        }
        else
        {
            container->pMaterials[0].MatD3D.Diffuse = D3DCOLORVALUE { 0.5f, 0.5f, 0.5f, 1.0f };
            container->pMaterials[0].MatD3D.Ambient = D3DCOLORVALUE { 0.5f, 0.5f, 0.5f, 1.0f };
        }

        if (skinInfo != nullptr)
        {
            container->pSkinInfo = skinInfo;
            container->pSkinInfo->AddRef();
        }

        *meshContainer = container;
        return S_OK;
    }

    STDMETHOD(DestroyFrame)(LPD3DXFRAME frame)
    {
        if (frame == nullptr)
        {
            return S_OK;
        }

        NSRender::SAFE_DELETE_ARRAY(frame->Name);
        NSRender::SAFE_DELETE(frame);
        return S_OK;
    }

    STDMETHOD(DestroyMeshContainer)(LPD3DXMESHCONTAINER meshContainerBase)
    {
        if (meshContainerBase == nullptr)
        {
            return S_OK;
        }

        ExportXMeshContainer* meshContainer = static_cast<ExportXMeshContainer*>(meshContainerBase);

        NSRender::SAFE_RELEASE(meshContainer->MeshData.pMesh);
        NSRender::SAFE_RELEASE(meshContainer->pSkinInfo);
        NSRender::SAFE_DELETE_ARRAY(meshContainer->Name);
        NSRender::SAFE_DELETE_ARRAY(meshContainer->pAdjacency);

        if (meshContainer->pMaterials != nullptr)
        {
            for (DWORD i = 0; i < meshContainer->NumMaterials; ++i)
            {
                NSRender::SAFE_DELETE_ARRAY(meshContainer->pMaterials[i].pTextureFilename);
            }
        }

        NSRender::SAFE_DELETE_ARRAY(meshContainer->pMaterials);
        NSRender::SAFE_DELETE(meshContainer);
        return S_OK;
    }
};

bool ResolveExistingModelPath(const std::wstring& sourcePath, std::wstring& resolvedPath)
{
    if (sourcePath.empty())
    {
        return false;
    }

    wchar_t fullPath[MAX_PATH] { };
    const DWORD length = GetFullPathNameW(sourcePath.c_str(),
                                          static_cast<DWORD>(_countof(fullPath)),
                                          fullPath,
                                          nullptr);
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

bool ExportXHierarchyBinary(const std::wstring& inputPath, const std::wstring& outputPath)
{
    ExportXHierarchyAllocator allocator;
    LPD3DXFRAME frameRoot = nullptr;
    LPD3DXANIMATIONCONTROLLER animationController = nullptr;

    const HRESULT loadResult = D3DXLoadMeshHierarchyFromXW(inputPath.c_str(),
                                                           D3DXMESH_SYSTEMMEM,
                                                           NSRender::Common::D3DDevice(),
                                                           &allocator,
                                                           nullptr,
                                                           &frameRoot,
                                                           &animationController);
    if (FAILED(loadResult))
    {
        return false;
    }

    const HRESULT saveResult = D3DXSaveMeshHierarchyToFileW(outputPath.c_str(),
                                                            D3DXF_FILEFORMAT_BINARY,
                                                            frameRoot,
                                                            animationController,
                                                            nullptr);

    D3DXFrameDestroy(frameRoot, &allocator);
    NSRender::SAFE_RELEASE(animationController);

    return SUCCEEDED(saveResult);
}

bool ExportMeshBinary(const std::wstring& inputPath, const std::wstring& outputPath)
{
    LPD3DXBUFFER adjacencyBuffer = nullptr;
    LPD3DXBUFFER materialBuffer = nullptr;
    LPD3DXBUFFER effectBuffer = nullptr;
    LPD3DXMESH mesh = nullptr;
    DWORD materialCount = 0;

    const HRESULT loadResult = D3DXLoadMeshFromXW(inputPath.c_str(),
                                                  D3DXMESH_SYSTEMMEM,
                                                  NSRender::Common::D3DDevice(),
                                                  &adjacencyBuffer,
                                                  &materialBuffer,
                                                  &effectBuffer,
                                                  &materialCount,
                                                  &mesh);

    if (FAILED(loadResult))
    {
        NSRender::SAFE_RELEASE(adjacencyBuffer);
        NSRender::SAFE_RELEASE(materialBuffer);
        NSRender::SAFE_RELEASE(effectBuffer);
        NSRender::SAFE_RELEASE(mesh);
        return false;
    }

    const DWORD* adjacency = (adjacencyBuffer != nullptr)
        ? static_cast<const DWORD*>(adjacencyBuffer->GetBufferPointer())
        : nullptr;
    const D3DXMATERIAL* materials = (materialBuffer != nullptr)
        ? static_cast<const D3DXMATERIAL*>(materialBuffer->GetBufferPointer())
        : nullptr;
    const D3DXEFFECTINSTANCE* effects = (effectBuffer != nullptr)
        ? static_cast<const D3DXEFFECTINSTANCE*>(effectBuffer->GetBufferPointer())
        : nullptr;

    const HRESULT saveResult = D3DXSaveMeshToXW(outputPath.c_str(),
                                                mesh,
                                                adjacency,
                                                materials,
                                                effects,
                                                materialCount,
                                                D3DXF_FILEFORMAT_BINARY);

    NSRender::SAFE_RELEASE(adjacencyBuffer);
    NSRender::SAFE_RELEASE(materialBuffer);
    NSRender::SAFE_RELEASE(effectBuffer);
    NSRender::SAFE_RELEASE(mesh);
    return SUCCEEDED(saveResult);
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

    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const float speed = shift ? (0.2f * 3.0f / 3.0f) : (0.2f / 3.0f);
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
    g_bPrevMouseClientPosValid = false;
    HideMouseCursor();
    SetCursor(NULL);
    if (!g_bRemoteDesktop)
    {
        RecenterMouseCursor(hWnd);
    }
}

void DisableMouseLook()
{
    if (!g_bMouseLookEnabled)
    {
        return;
    }

    g_bMouseLookEnabled = false;
    g_bRecenteringMouse = false;
    g_bPrevMouseClientPosValid = false;
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
    g_Render.SetPostEffectDepthOfField(g_bDepthOfField);
    g_Render.SetPostEffectStarBurst(g_bStarBurst);
    ApplyGodRay();
}

void ApplyFogIntensity()
{
    g_fogIntensity = ClampFogIntensity(g_fogIntensity);
    g_Render.SetPostEffectFogIntensity(g_fogIntensity);
}

void ApplySunLightIntensity()
{
    g_sunLightIntensity = ClampSunLightIntensity(g_sunLightIntensity);
    g_Render.SetLightBrightness(g_sunLightIntensity);
}

void ApplyShadowIntensity()
{
    g_shadowIntensity = ClampShadowIntensity(g_shadowIntensity);
    g_Render.SetPostEffectDepthBufferShadowIntensity(g_shadowIntensity);
}

void ApplyShadowCoverage()
{
    g_shadowCoverage = ClampShadowCoverage(g_shadowCoverage);
    g_Render.SetPostEffectDepthBufferShadowCoverage(g_shadowCoverage);
}

void ApplyShadowSaturationBoost()
{
    g_shadowSaturationBoost = ClampShadowSaturationBoost(g_shadowSaturationBoost);
    g_Render.SetPostEffectDepthBufferShadowSaturationBoost(g_shadowSaturationBoost);
}

void ApplyShadowPcfTapCount()
{
    g_shadowPcfTapCount = NormalizeShadowBlurTapCountLocal(g_shadowPcfTapCount);
    g_Render.SetPostEffectDepthBufferShadowPcfTapCount(g_shadowPcfTapCount);
}

void ApplyShadowCompositeTapCount()
{
    g_shadowCompositeTapCount = NormalizeShadowBlurTapCountLocal(g_shadowCompositeTapCount);
    g_Render.SetPostEffectDepthBufferShadowCompositeTapCount(g_shadowCompositeTapCount);
}

void ApplySSAOBrightness()
{
    g_ssaoBrightness = ClampSSAOBrightness(g_ssaoBrightness);
    g_Render.SetPostEffectSSAOBrightness(g_ssaoBrightness);
}

void ApplySSAOSaturationBoost()
{
    g_ssaoSaturationBoost = ClampSSAOSaturationBoost(g_ssaoSaturationBoost);
    g_Render.SetPostEffectSSAOSaturationBoost(g_ssaoSaturationBoost);
}

void ApplyHalfLambertShadowSaturation()
{
    g_halfLambertShadowSaturation = ClampHalfLambertShadowSaturation(g_halfLambertShadowSaturation);
    g_Render.SetMeshMixSaturateShadow(g_halfLambertShadowSaturation > 0.0f);
    g_Render.SetMeshMixSaturateShadowIntensity(g_halfLambertShadowSaturation);
}

void ApplyShadowDarkness()
{
    g_shadowDarkness = ClampShadowDarkness(g_shadowDarkness);
    g_Render.SetMeshMixShadowDarkness(g_shadowDarkness);
}

void ApplySpecularIntensity()
{
    g_specularIntensity = ClampSpecularIntensity(g_specularIntensity);
    g_Render.SetMeshMixSpecularIntensity(g_specularIntensity);
}

void ApplySpecularEdge()
{
    g_specularEdge = ClampSpecularEdge(g_specularEdge);
    g_Render.SetMeshMixSpecularEdge(g_specularEdge);
}

void ApplySpecularEdgeOverride()
{
    g_Render.SetMeshMixSpecularEdgeOverrideEnabled(g_bUseSpecularEdgeOverride);
    RefreshSettingsDialogState();
}

void ApplyBloomThreshold()
{
    g_bloomThreshold = ClampBloomThreshold(g_bloomThreshold);
    g_Render.SetPostEffectBloomThreshold(g_bloomThreshold);
}

void ApplyDepthOfFieldFocalDistance()
{
    g_dofFocalDistance = ClampDepthOfFieldFocalDistance(g_dofFocalDistance);
    g_Render.SetPostEffectDepthOfFieldFocalDistance(g_dofFocalDistance);
}

void ApplyDepthOfFieldMaxBlurDistance()
{
    g_dofMaxBlurDistance = ClampDepthOfFieldMaxBlurDistance(g_dofMaxBlurDistance);
    g_Render.SetPostEffectDepthOfFieldMaxBlurDistance(g_dofMaxBlurDistance);
}

void ApplyStarBurstThreshold()
{
    g_starBurstThreshold = ClampStarBurstThreshold(g_starBurstThreshold);
    g_Render.SetPostEffectStarBurstThreshold(g_starBurstThreshold);
}

void ApplyModelLoadScale()
{
    g_modelLoadScale = ClampModelLoadScale(g_modelLoadScale);
}

void ApplyPointLightColor()
{
    g_pointLightColor.r = ClampPointLightColor(g_pointLightColor.r);
    g_pointLightColor.g = ClampPointLightColor(g_pointLightColor.g);
    g_pointLightColor.b = ClampPointLightColor(g_pointLightColor.b);
    g_pointLightColor.a = 1.0f;
}

void ApplyPointLightBrightness()
{
    g_pointLightBrightness = ClampPointLightBrightness(g_pointLightBrightness);
}

void ApplyGaussianSampleSize()
{
    g_gaussianSampleSize = NormalizeGaussianSampleSizeLocal(g_gaussianSampleSize);
    g_Render.SetPostEffectGaussianSampleSize(g_gaussianSampleSize);
}

void ApplyResolution()
{
    if (g_resolutionWidth <= 0 || g_resolutionHeight <= 0)
    {
        return;
    }

    g_Render.ChangeResolution(g_resolutionWidth, g_resolutionHeight);
}

void ApplyWindowMode()
{
    g_Render.ChangeWindowMode(g_windowMode);
}

void InitializeRemoteDesktopDefault()
{
    g_bRemoteDesktop = IsWeekdayBusinessHours();
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

int SunLightIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampSunLightIntensity(intensity) / SUN_LIGHT_INTENSITY_STEP));
}

float SliderValueToSunLightIntensity(const int sliderValue)
{
    return ClampSunLightIntensity(static_cast<float>(sliderValue) * SUN_LIGHT_INTENSITY_STEP);
}

int ShadowIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampShadowIntensity(intensity) / SHADOW_INTENSITY_STEP));
}

float SliderValueToShadowIntensity(const int sliderValue)
{
    return ClampShadowIntensity(static_cast<float>(sliderValue) * SHADOW_INTENSITY_STEP);
}

int ShadowCoverageToSliderValue(const float coverage)
{
    return static_cast<int>(std::lround(ClampShadowCoverage(coverage) / SHADOW_COVERAGE_STEP));
}

float SliderValueToShadowCoverage(const int sliderValue)
{
    return ClampShadowCoverage(static_cast<float>(sliderValue) * SHADOW_COVERAGE_STEP);
}

int ShadowSaturationBoostToSliderValue(const float boost)
{
    return static_cast<int>(std::lround(ClampShadowSaturationBoost(boost) / SHADOW_SATURATION_BOOST_STEP));
}

float SliderValueToShadowSaturationBoost(const int sliderValue)
{
    return ClampShadowSaturationBoost(static_cast<float>(sliderValue) * SHADOW_SATURATION_BOOST_STEP);
}

int ShadowTapCountToSliderValue(const int tapCount)
{
    return ((NormalizeShadowBlurTapCountLocal(tapCount) - 1) / 2);
}

int SliderValueToShadowTapCount(const int sliderValue)
{
    return NormalizeShadowBlurTapCountLocal((sliderValue * 2) + 1);
}

int SSAOBrightnessToSliderValue(const float brightness)
{
    return static_cast<int>(std::lround((ClampSSAOBrightness(brightness) - SSAO_BRIGHTNESS_MIN) / SSAO_BRIGHTNESS_STEP));
}

float SliderValueToSSAOBrightness(const int sliderValue)
{
    return ClampSSAOBrightness(SSAO_BRIGHTNESS_MIN + static_cast<float>(sliderValue) * SSAO_BRIGHTNESS_STEP);
}

int SSAOSaturationBoostToSliderValue(const float boost)
{
    return static_cast<int>(std::lround(ClampSSAOSaturationBoost(boost) / SSAO_SATURATION_BOOST_STEP));
}

float SliderValueToSSAOSaturationBoost(const int sliderValue)
{
    return ClampSSAOSaturationBoost(static_cast<float>(sliderValue) * SSAO_SATURATION_BOOST_STEP);
}

int HalfLambertShadowSaturationToSliderValue(const float boost)
{
    return static_cast<int>(std::lround(ClampHalfLambertShadowSaturation(boost) / HALF_LAMBERT_SHADOW_SATURATION_STEP));
}

float SliderValueToHalfLambertShadowSaturation(const int sliderValue)
{
    return ClampHalfLambertShadowSaturation(static_cast<float>(sliderValue) * HALF_LAMBERT_SHADOW_SATURATION_STEP);
}

int ShadowDarknessToSliderValue(const float darkness)
{
    return static_cast<int>(std::lround(ClampShadowDarkness(darkness) / SHADOW_DARKNESS_STEP));
}

float SliderValueToShadowDarkness(const int sliderValue)
{
    return ClampShadowDarkness(static_cast<float>(sliderValue) * SHADOW_DARKNESS_STEP);
}

int SpecularIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(ClampSpecularIntensity(intensity) / SPECULAR_INTENSITY_STEP));
}

float SliderValueToSpecularIntensity(const int sliderValue)
{
    return ClampSpecularIntensity(static_cast<float>(sliderValue) * SPECULAR_INTENSITY_STEP);
}

int SpecularEdgeToSliderValue(const float edge)
{
    return static_cast<int>(std::lround(ClampSpecularEdge(edge) / SPECULAR_EDGE_STEP));
}

float SliderValueToSpecularEdge(const int sliderValue)
{
    return ClampSpecularEdge(static_cast<float>(sliderValue) * SPECULAR_EDGE_STEP);
}

int BloomThresholdToSliderValue(const float threshold)
{
    return static_cast<int>(std::lround(ClampBloomThreshold(threshold) / BLOOM_THRESHOLD_STEP));
}

float SliderValueToBloomThreshold(const int sliderValue)
{
    return ClampBloomThreshold(static_cast<float>(sliderValue) * BLOOM_THRESHOLD_STEP);
}

int DepthOfFieldFocalDistanceToSliderValue(const float distance)
{
    return static_cast<int>(std::lround((ClampDepthOfFieldFocalDistance(distance) - DOF_FOCAL_DISTANCE_MIN) / DOF_FOCAL_DISTANCE_STEP));
}

float SliderValueToDepthOfFieldFocalDistance(const int sliderValue)
{
    return ClampDepthOfFieldFocalDistance(DOF_FOCAL_DISTANCE_MIN + static_cast<float>(sliderValue) * DOF_FOCAL_DISTANCE_STEP);
}

int DepthOfFieldMaxBlurDistanceToSliderValue(const float distance)
{
    return static_cast<int>(std::lround((ClampDepthOfFieldMaxBlurDistance(distance) - DOF_MAX_BLUR_DISTANCE_MIN) / DOF_MAX_BLUR_DISTANCE_STEP));
}

float SliderValueToDepthOfFieldMaxBlurDistance(const int sliderValue)
{
    return ClampDepthOfFieldMaxBlurDistance(DOF_MAX_BLUR_DISTANCE_MIN + static_cast<float>(sliderValue) * DOF_MAX_BLUR_DISTANCE_STEP);
}

int StarBurstThresholdToSliderValue(const float threshold)
{
    return static_cast<int>(std::lround(ClampStarBurstThreshold(threshold) / STARBURST_THRESHOLD_STEP));
}

float SliderValueToStarBurstThreshold(const int sliderValue)
{
    return ClampStarBurstThreshold(static_cast<float>(sliderValue) * STARBURST_THRESHOLD_STEP);
}

int ModelLoadScaleToSliderValue(const float scale)
{
    return static_cast<int>(std::lround((ClampModelLoadScale(scale) - MODEL_LOAD_SCALE_MIN) / MODEL_LOAD_SCALE_STEP));
}

float SliderValueToModelLoadScale(const int sliderValue)
{
    return ClampModelLoadScale(MODEL_LOAD_SCALE_MIN + static_cast<float>(sliderValue) * MODEL_LOAD_SCALE_STEP);
}

int PointLightColorToSliderValue(const float value)
{
    return static_cast<int>(std::lround(ClampPointLightColor(value) / POINT_LIGHT_COLOR_STEP));
}

float SliderValueToPointLightColor(const int sliderValue)
{
    return ClampPointLightColor(static_cast<float>(sliderValue) * POINT_LIGHT_COLOR_STEP);
}

int PointLightBrightnessToSliderValue(const float brightness)
{
    return static_cast<int>(std::lround(ClampPointLightBrightness(brightness) / POINT_LIGHT_BRIGHTNESS_STEP));
}

float SliderValueToPointLightBrightness(const int sliderValue)
{
    return ClampPointLightBrightness(static_cast<float>(sliderValue) * POINT_LIGHT_BRIGHTNESS_STEP);
}

int GaussianSampleSizeToSliderValue(const int sampleSize)
{
    return (NormalizeGaussianSampleSizeLocal(sampleSize) + 1) / 2;
}

int SliderValueToGaussianSampleSize(const int sliderValue)
{
    return NormalizeGaussianSampleSizeLocal(sliderValue * 2 - 1);
}

static const std::wstring GODRAY_MARKER_PATH = L"..\\..\\Sample\\cube.x"; // 作業ディレクトリからの相対パス

void ApplyGodRay()
{
    g_Render.SetPostEffectGodRay(g_bGodRay);

    if (g_bGodRay)
    {
        if (g_godRayMarkerMeshId == -1)
        {
            g_godRayMarkerMeshId = g_Render.AddMeshMix(GODRAY_MARKER_PATH,
                                                       g_godRayLightPos + D3DXVECTOR3(0.0f, 1.0f, 0.0f),
                                                       D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                       1.0f,
                                                       1.0f,
                                                       false,
                                                       false);
        }
    }
    else
    {
        if (g_godRayMarkerMeshId != -1)
        {
            g_Render.RemoveMeshMix(g_godRayMarkerMeshId);
            g_godRayMarkerMeshId = -1;
        }
    }
}

void ApplyGodRayLightColor()
{
    g_godRayLightColor.x = (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(g_godRayLightColor.x, GODRAY_LIGHT_COLOR_MAX));
    g_godRayLightColor.y = (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(g_godRayLightColor.y, GODRAY_LIGHT_COLOR_MAX));
    g_godRayLightColor.z = (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(g_godRayLightColor.z, GODRAY_LIGHT_COLOR_MAX));
    g_Render.SetPostEffectGodRayLightColor(g_godRayLightColor);
}

void ApplyGodRayIntensity()
{
    g_godRayIntensity = (std::max)(GODRAY_INTENSITY_MIN, (std::min)(g_godRayIntensity, GODRAY_INTENSITY_MAX));
    g_Render.SetPostEffectGodRayIntensity(g_godRayIntensity);
}

void ApplyGodRayLightPos()
{
    g_Render.SetPostEffectGodRayLightPos(g_godRayLightPos);

    if (g_godRayMarkerMeshId != -1)
    {
        g_Render.SetMeshMixPos(g_godRayMarkerMeshId, g_godRayLightPos + D3DXVECTOR3(0.0f, 1.0f, 0.0f));
    }
}

int GodRayLightColorToSliderValue(const float value)
{
    return static_cast<int>(std::lround(
        (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(value, GODRAY_LIGHT_COLOR_MAX)) / GODRAY_LIGHT_COLOR_STEP));
}

float SliderValueToGodRayLightColor(const int sliderValue)
{
    return (std::max)(GODRAY_LIGHT_COLOR_MIN, (std::min)(static_cast<float>(sliderValue) * GODRAY_LIGHT_COLOR_STEP, GODRAY_LIGHT_COLOR_MAX));
}

int GodRayIntensityToSliderValue(const float intensity)
{
    return static_cast<int>(std::lround(
        (std::max)(GODRAY_INTENSITY_MIN, (std::min)(intensity, GODRAY_INTENSITY_MAX)) / GODRAY_INTENSITY_STEP));
}

float SliderValueToGodRayIntensity(const int sliderValue)
{
    return (std::max)(GODRAY_INTENSITY_MIN, (std::min)(static_cast<float>(sliderValue) * GODRAY_INTENSITY_STEP, GODRAY_INTENSITY_MAX));
}

int GodRayLightPosToSliderValue(const float pos)
{
    return static_cast<int>(std::lround(
        (std::max)(GODRAY_LIGHT_POS_MIN, (std::min)(pos, GODRAY_LIGHT_POS_MAX)) / GODRAY_LIGHT_POS_STEP));
}

float SliderValueToGodRayLightPos(const int sliderValue)
{
    return (std::max)(GODRAY_LIGHT_POS_MIN, (std::min)(static_cast<float>(sliderValue) * GODRAY_LIGHT_POS_STEP, GODRAY_LIGHT_POS_MAX));
}

void RegisterLoadedModel(const std::wstring& type,
                         const std::wstring& path,
                         const D3DXVECTOR3& pos,
                         const float scale,
                         const int renderId)
{
    LoadedModelInfo info;
    info.m_type = type;
    info.m_path = path;
    info.m_pos = pos;
    info.m_scale = scale;
    info.m_renderId = renderId;
    g_loadedModelList.push_back(info);
    RefreshSettingsDialogState();
}

bool RemoveLoadedModel(const size_t modelIndex)
{
    if (modelIndex >= g_loadedModelList.size())
    {
        return false;
    }

    const LoadedModelInfo& model = g_loadedModelList.at(modelIndex);
    bool removed = false;

    if (model.m_type == L"Mesh")
    {
        removed = g_Render.RemoveMesh(model.m_renderId);
    }
    else if (model.m_type == L"MeshMix" || model.m_type == L"MeshMixManager")
    {
        removed = g_Render.RemoveMeshMix(model.m_renderId);
    }
    else if (model.m_type == L"MeshMixSkinAnim")
    {
        removed = g_Render.RemoveMeshMixSkinAnim(model.m_renderId);
    }
    else if (model.m_type == L"AnimMesh")
    {
        removed = g_Render.RemoveAnimMesh(model.m_renderId);
    }
    else if (model.m_type == L"SkinAnimMesh")
    {
        removed = g_Render.RemoveSkinAnimMesh(model.m_renderId);
    }
    else if (model.m_type == L"MeshSSS")
    {
        removed = g_Render.RemoveMeshSSS(model.m_renderId);
    }
    else if (model.m_type == L"MeshPOM")
    {
        removed = g_Render.RemoveMeshPOM(model.m_renderId);
    }
    else if (model.m_type == L"MeshPointLight")
    {
        removed = g_Render.RemoveMeshPointLight(model.m_renderId);
    }
    else if (model.m_type == L"MeshNormalMap")
    {
        removed = g_Render.RemoveMeshNormalMapping(model.m_renderId);
    }
    else if (model.m_type == L"Instancing")
    {
        removed = g_Render.RemoveMeshInstancing(model.m_path);
    }

    if (!removed)
    {
        return false;
    }

    if (model.m_type == L"Instancing")
    {
        g_loadedModelList.erase(std::remove_if(g_loadedModelList.begin(),
                                               g_loadedModelList.end(),
                                               [&model](const LoadedModelInfo& info)
                                               {
                                                   return info.m_type == model.m_type && info.m_path == model.m_path;
                                               }),
                                g_loadedModelList.end());
    }
    else
    {
        g_loadedModelList.erase(g_loadedModelList.begin() + static_cast<std::ptrdiff_t>(modelIndex));
    }

    RefreshSettingsDialogState();
    return true;
}

void SpawnMeshAtLookAt(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const int renderId = g_Render.AddMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), g_modelLoadScale, 1.0f);
    RegisterLoadedModel(L"Mesh", filePath, pos, g_modelLoadScale, renderId);
}

void SpawnMeshMixAtLookAt(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const bool usePOM = (g_mixMeshShaderMode == MixMeshShaderMode::ParallaxOcclusionMapping);
    const bool useNormalMapping = (g_mixMeshShaderMode == MixMeshShaderMode::NormalMapping);
    const int renderId = g_Render.AddMeshMix(filePath,
                                             pos,
                                             D3DXVECTOR3(0, yaw, 0.0f),
                                             g_modelLoadScale,
                                             1.0f,
                                             usePOM,
                                             useNormalMapping);
    RegisterLoadedModel(L"MeshMixManager", filePath, pos, g_modelLoadScale, renderId);
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

void SpawnAnimMeshAtLookAt(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const int renderId = g_Render.AddAnimMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), g_modelLoadScale, CreateDefaultAnimSetMap());
    RegisterLoadedModel(L"AnimMesh", filePath, pos, g_modelLoadScale, renderId);
}

void SpawnSkinAnimMeshAtLookAt(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const int renderId = g_Render.AddSkinAnimMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), g_modelLoadScale, CreateDefaultAnimSetMap());
    RegisterLoadedModel(L"SkinAnimMesh", filePath, pos, g_modelLoadScale, renderId);
}

void SpawnMeshMixSkinAnimAtLookAt(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return;
    }

    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const bool usePOM = (g_mixMeshShaderMode == MixMeshShaderMode::ParallaxOcclusionMapping);
    const bool useNormalMapping = (g_mixMeshShaderMode == MixMeshShaderMode::NormalMapping);
    const int renderId = g_Render.AddMeshMixSkinAnim(filePath,
                                                     pos,
                                                     D3DXVECTOR3(0, yaw, 0.0f),
                                                     g_modelLoadScale,
                                                     CreateDefaultAnimSetMap(),
                                                     1.0f,
                                                     usePOM,
                                                     useNormalMapping);
    RegisterLoadedModel(L"MeshMixSkinAnim", filePath, pos, g_modelLoadScale, renderId);
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

bool ShowSaveBinaryXFileDialog(HWND hWnd, const std::wstring& sourcePath, std::wstring& selectedPath)
{
    std::wstring defaultName = sourcePath;
    const std::size_t slashPos = defaultName.find_last_of(L"\\/");
    if (slashPos != std::wstring::npos)
    {
        defaultName = defaultName.substr(slashPos + 1);
    }

    const std::size_t dotPos = defaultName.find_last_of(L'.');
    if (dotPos != std::wstring::npos)
    {
        defaultName = defaultName.substr(0, dotPos);
    }

    defaultName += L"_binary.x";
    if (defaultName.empty())
    {
        defaultName = L"export_binary.x";
    }

    wchar_t filePath[MAX_PATH] { };
    wcsncpy_s(filePath, defaultName.c_str(), _TRUNCATE);

    OPENFILENAMEW ofn { };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"X Files (*.x)\0*.x\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = static_cast<DWORD>(_countof(filePath));
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"x";

    if (!GetSaveFileNameW(&ofn))
    {
        return false;
    }

    selectedPath = filePath;
    return true;
}

bool ExportLoadedModelAsBinaryX(const size_t modelIndex, const std::wstring& outputPath)
{
    if (modelIndex >= g_loadedModelList.size() || outputPath.empty())
    {
        return false;
    }

    std::wstring inputPath;
    if (!ResolveExistingModelPath(g_loadedModelList.at(modelIndex).m_path, inputPath))
    {
        return false;
    }

    if (ExportXHierarchyBinary(inputPath, outputPath))
    {
        return true;
    }

    return ExportMeshBinary(inputPath, outputPath);
}

void AddPointLightAtLookAt()
{
    ApplyPointLightColor();
    ApplyPointLightBrightness();
    g_Render.AddPointLight(g_Render.GetLookAtPos(), g_pointLightBrightness, g_pointLightColor);
    RefreshSettingsDialogState();
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
            else if (key == L"SunLightIntensity")
            {
                g_sunLightIntensity = std::stof(value);
            }
            else if (key == L"ShadowIntensity")
            {
                g_shadowIntensity = std::stof(value);
            }
            else if (key == L"ShadowCoverage")
            {
                g_shadowCoverage = std::stof(value);
            }
            else if (key == L"SSAOBrightness")
            {
                g_ssaoBrightness = std::stof(value);
            }
            else if (key == L"ShadowBlurTapCount")
            {
                const int tapCount = std::stoi(value);
                g_shadowPcfTapCount = tapCount;
                g_shadowCompositeTapCount = tapCount;
            }
            else if (key == L"ShadowPcfTapCount")
            {
                g_shadowPcfTapCount = std::stoi(value);
            }
            else if (key == L"ShadowCompositeTapCount")
            {
                g_shadowCompositeTapCount = std::stoi(value);
            }
            else if (key == L"ShadowSaturationBoost")
            {
                g_shadowSaturationBoost = std::stof(value);
            }
            else if (key == L"SSAOSaturationBoost")
            {
                g_ssaoSaturationBoost = std::stof(value);
            }
            else if (key == L"HalfLambertShadowSaturation")
            {
                g_halfLambertShadowSaturation = std::stof(value);
            }
            else if (key == L"ShadowDarkness")
            {
                g_shadowDarkness = std::stof(value);
            }
            else if (key == L"SpecularIntensity")
            {
                g_specularIntensity = std::stof(value);
            }
            else if (key == L"SpecularEdge")
            {
                g_specularEdge = std::stof(value);
            }
            else if (key == L"SpecularEdgeOverride")
            {
                g_bUseSpecularEdgeOverride = (std::stoi(value) != 0);
            }
            else if (key == L"BloomThreshold")
            {
                g_bloomThreshold = std::stof(value);
            }
            else if (key == L"DepthOfFieldFocalDistance")
            {
                g_dofFocalDistance = std::stof(value);
            }
            else if (key == L"DepthOfFieldMaxBlurDistance")
            {
                g_dofMaxBlurDistance = std::stof(value);
            }
            else if (key == L"StarBurstThreshold")
            {
                g_starBurstThreshold = std::stof(value);
            }
            else if (key == L"ModelLoadScale")
            {
                g_modelLoadScale = std::stof(value);
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
            else if (key == L"SaturateLevel")
            {
                g_saturateLevel = std::stof(value);
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
            else if (key == L"DepthOfFieldEnable")
            {
                g_bDepthOfField = (std::stoi(value) != 0);
            }
            else if (key == L"StarBurstEnable")
            {
                g_bStarBurst = (std::stoi(value) != 0);
            }
            else if (key == L"GodRayEnable")
            {
                g_bGodRay = (std::stoi(value) != 0);
            }
            else if (key == L"GodRayLightColorR")
            {
                g_godRayLightColor.x = std::stof(value);
            }
            else if (key == L"GodRayLightColorG")
            {
                g_godRayLightColor.y = std::stof(value);
            }
            else if (key == L"GodRayLightColorB")
            {
                g_godRayLightColor.z = std::stof(value);
            }
            else if (key == L"GodRayIntensity")
            {
                g_godRayIntensity = std::stof(value);
            }
            else if (key == L"GodRayLightPosX")
            {
                g_godRayLightPos.x = std::stof(value);
            }
            else if (key == L"GodRayLightPosY")
            {
                g_godRayLightPos.y = std::stof(value);
            }
            else if (key == L"GodRayLightPosZ")
            {
                g_godRayLightPos.z = std::stof(value);
            }
        }
        catch (...)
        {
        }
    }
}

bool g_bShowOverlay = true; // グローバル変数

void DrawSampleOverlay()
{
    if (!g_bShowOverlay)
    {
        return;
    }

    std::wstring text;
    text += L"WASD : Camera move\n";
    text += L"Q/E : Camera up/down\n";
    text += L"Arrow keys : Camera rotate\n";
    text += L"Esc : Mouse look ON/OFF\n";
    text += L"F1 : Settings dialog\n";
    text += L"F2 : Toggle overlay\n"; // F2キーの説明を追加
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
    text += L"u : Depth of field ON/OFF\n";
    text += L"Shift + b : StarBurst ON/OFF\n";
    text += L"h : Depth buffer shadow ON/OFF\n";
    text += L"j : SSAO ON/OFF\n";
    text += L"v : Fog ON/OFF\n";
    text += L"Shift + f : FPS ON/OFF\n";
    g_Render.DrawText_(g_fontId, text, 10, 40);

    DrawRandomized2DContent();
}

void HandleKeyPress(WPARAM key)
{
    if (key == VK_F2)
    {
        g_bShowOverlay = !g_bShowOverlay;
    }
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
