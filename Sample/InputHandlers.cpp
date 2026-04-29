#include "InputHandlers.h"

#include <cmath>
#include <cstdlib>

#include "AppState.h"
#include "SettingsDialog.h"

namespace
{
void SetMovementFlag(const WPARAM key, const bool value)
{
    if (key == 'W')
    {
        g_bMoveForward = value;
    }
    else if (key == 'S')
    {
        g_bMoveBackward = value;
    }
    else if (key == 'A')
    {
        g_bMoveLeft = value;
    }
    else if (key == 'D')
    {
        g_bMoveRight = value;
    }
    else if (key == 'E')
    {
        g_bMoveUp = value;
    }
    else if (key == 'Q')
    {
        g_bMoveDown = value;
    }
}

void AddSimpleMeshAtCameraFront(const std::wstring& filePath,
                                const float scale,
                                const bool useSSS = false,
                                const bool usePOM = false,
                                const bool usePointLight = false)
{
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);

    if (useSSS)
    {
        g_Render.AddMeshSSS(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale, 1.0f);
        RegisterLoadedModel(L"MeshSSS", filePath, pos, scale);
    }
    else if (usePOM)
    {
        g_Render.AddMeshPOM(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale, 1.0f);
        RegisterLoadedModel(L"MeshPOM", filePath, pos, scale);
    }
    else if (usePointLight)
    {
        g_Render.AddMeshPointLight(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale);
        RegisterLoadedModel(L"MeshPointLight", filePath, pos, scale);
    }
    else
    {
        g_Render.AddMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale, 1.0f);
        RegisterLoadedModel(L"Mesh", filePath, pos, scale);
    }
}

void AddNormalMappedMeshAtCameraFront()
{
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddMeshNormalMapping(L"cubeNormalMap.x",
                                  L"normalMap.png",
                                  pos,
                                  D3DXVECTOR3(0, yaw, 0.0f),
                                  1.0f);
    RegisterLoadedModel(L"MeshNormalMap", L"cubeNormalMap.x", pos, 1.0f);
}

void AddSkinAnimMeshAtCameraFront()
{
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddSkinAnimMesh(L"res\\model\\wolf.x",
                             pos,
                             D3DXVECTOR3(0, yaw, 0.0f),
                             3.0f,
                             CreateDefaultAnimSetMap());
    RegisterLoadedModel(L"SkinAnimMesh", L"res\\model\\wolf.x", pos, 3.0f);
}

void AddAnimMeshAtCameraFront()
{
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);
    pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

    const float yaw = atan2f(forward.x, forward.z);
    g_Render.AddAnimMesh(L"enemyOrangeCube.x",
                         pos,
                         D3DXVECTOR3(0, yaw, 0.0f),
                         1.0f,
                         CreateDefaultAnimSetMap());
    RegisterLoadedModel(L"AnimMesh", L"enemyOrangeCube.x", pos, 1.0f);
}

void AddImageOrPointLight(const bool shift, const bool control)
{
    if (control)
    {
        const auto pos = g_Render.GetLookAtPos();
        g_Render.AddPointLight(pos, 1.0f, D3DXCOLOR(1.0f, 0.35f, 0.1f, 1.0f));
        return;
    }

    if (shift)
    {
        g_imageInfoList.clear();
        return;
    }

    ImageInfo imageInfo;
    imageInfo.m_imageName = L"cursor.png";
    imageInfo.m_rect.left = std::abs(std::rand()) % 1300;
    imageInfo.m_rect.top = std::abs(std::rand()) % 700;
    g_imageInfoList.push_back(imageInfo);
}

void AddOrClearText(const bool shift)
{
    if (shift)
    {
        g_textInfoList.clear();
        return;
    }

    TextInfo textInfo;
    textInfo.m_text = L"サンプルテキスト";
    textInfo.m_rect.left = std::abs(std::rand()) % 1300;
    textInfo.m_rect.top = std::abs(std::rand()) % 700;
    g_textInfoList.push_back(textInfo);
}
}

bool HandleSampleKeyUp(const WPARAM wParam)
{
    SetMovementFlag(wParam, false);
    return true;
}

bool HandleSampleKeyDown(HWND hWnd, const WPARAM wParam)
{
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (wParam == VK_F1)
    {
        ToggleSettingsDialog(hWnd);
        return true;
    }

    if (wParam == VK_ESCAPE)
    {
        if (g_bMouseLookEnabled)
        {
            DisableMouseLook();
        }
        else
        {
            EnableMouseLook(hWnd);
        }
        return true;
    }

    if (wParam == '8')
    {
        g_Render.ChangeWindowMode(NSRender::eWindowMode::WINDOW);
    }
    else if (wParam == '9')
    {
        g_Render.ChangeWindowMode(NSRender::eWindowMode::BORDERLESS);
    }
    else if (wParam == '0')
    {
        g_Render.ChangeWindowMode(NSRender::eWindowMode::FULLSCREEN);
    }

    if (wParam == 'M' && !shift && !control)
    {
        SpawnMeshMixAtCameraFront(L"..\\..\\Sample\\monkeySSAO.blend.x");
    }
    else if (wParam == 'M' && shift && control)
    {
        AddSimpleMeshAtCameraFront(L"cube.x", 1.0f);
    }
    else if (wParam == 'M' && shift && !control)
    {
        AddSimpleMeshAtCameraFront(L"cubePOM.blend.x", 1.0f, false, true);
    }
    else if (wParam == 'M' && !shift && control)
    {
        AddSimpleMeshAtCameraFront(L"monkey.blend.x", 1.0f, true);
    }

    if (wParam == 'N' && shift && !control)
    {
        AddNormalMappedMeshAtCameraFront();
    }
    else if (wParam == 'N' && !shift && !control)
    {
        AddAnimMeshAtCameraFront();
    }

    if (wParam == 'K')
    {
        AddSkinAnimMeshAtCameraFront();
    }

    if (wParam == 'I')
    {
        auto pos = g_Render.GetLookAtPos();
        D3DXVECTOR3 forward = g_Render.GetCameraRotate();
        D3DXVec3Normalize(&forward, &forward);
        pos += forward * MODEL_SPAWN_FORWARD_OFFSET;

        const float yaw = atan2f(forward.x, forward.z);
        g_Render.AddMeshInstansing(L"cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f);
        RegisterLoadedModel(L"Instancing", L"cube.x", pos, 1.0f);
    }

    if (wParam == 'O')
    {
        AddSimpleMeshAtCameraFront(L"cube.x", 1.0f, false, false, true);
    }

    if (!shift && !control)
    {
        SetMovementFlag(wParam, true);
    }

    if (wParam == VK_UP)
    {
        g_Render.RotateCamera(D3DXVECTOR3(-0.2f, 0, 0));
    }
    else if (wParam == VK_DOWN)
    {
        g_Render.RotateCamera(D3DXVECTOR3(0.2f, 0, 0));
    }
    else if (wParam == VK_LEFT)
    {
        g_Render.RotateCamera(D3DXVECTOR3(0, -0.2f, 0));
    }
    else if (wParam == VK_RIGHT)
    {
        g_Render.RotateCamera(D3DXVECTOR3(0, 0.2f, 0));
    }

    if (wParam == 'P')
    {
        AddImageOrPointLight(shift, control);
    }

    if (wParam == 'C')
    {
        AddOrClearText(shift);
    }

    if (wParam == 'S' && shift)
    {
        g_saturateLevel += SATURATE_STEP;
        ApplySaturateLevel();
        RefreshSettingsDialogState();
    }

    if (wParam == 'S' && control)
    {
        g_saturateLevel -= SATURATE_STEP;
        ApplySaturateLevel();
        RefreshSettingsDialogState();
    }

    if (wParam == 'G')
    {
        g_bGaussianFilter = !g_bGaussianFilter;
        ApplyPostEffectToggleSettings();
        RefreshSettingsDialogState();
    }

    if (wParam == 'T')
    {
        g_bSaturateFilter = !g_bSaturateFilter;
        ApplyPostEffectToggleSettings();
        RefreshSettingsDialogState();
    }

    if (wParam == 'B' && !shift)
    {
        g_bBloom = !g_bBloom;
        ApplyPostEffectToggleSettings();
        RefreshSettingsDialogState();
    }

    if (wParam == 'B' && shift)
    {
        g_bStarBurst = !g_bStarBurst;
        ApplyPostEffectToggleSettings();
        RefreshSettingsDialogState();
    }

    if (wParam == 'H')
    {
        g_bDepthBufferShadow = !g_bDepthBufferShadow;
        ApplyPostEffectToggleSettings();
        RefreshSettingsDialogState();
    }

    if (wParam == 'J')
    {
        g_bSSAO = !g_bSSAO;
        ApplyPostEffectToggleSettings();
        RefreshSettingsDialogState();
    }

    if (wParam == 'V')
    {
        g_bFog = !g_bFog;
        ApplyPostEffectToggleSettings();
        RefreshSettingsDialogState();
    }

    {
        static bool bShowFPS = true;
        if (wParam == 'F')
        {
            bShowFPS = !bShowFPS;
            g_Render.SetShowFPS(bShowFPS);
        }
    }

    if (wParam == 'R')
    {
        const auto resoList = g_Render.GetResolutionList();
        std::wstring work;
        for (auto& reso : resoList)
        {
            work += L" Resolution : ";
            work += std::to_wstring(reso.first);
            work += L" x ";
            work += std::to_wstring(reso.second);
            work += L"\n";
            OutputDebugString(work.c_str());
        }
    }

    return true;
}
