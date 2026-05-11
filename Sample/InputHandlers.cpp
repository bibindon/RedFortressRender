#include "InputHandlers.h"

#include <cmath>
#include <cstdlib>

#include "AppState.h"
#include "SettingsDialog.h"

// このファイルはサンプル用ショートカットと移動キー割り当ての定義を持つ。
// main.cpp のメッセージ処理から呼ばれるが、
// 実際の「何を起こすか」はここへ集約して操作体系を見通しやすくしている。

namespace
{
void ToggleDebugGBufferView(const NSRender::DebugGBufferView view)
{
    // 同じデバッグビューをもう一度押したときは解除し、
    // 一つのキーで ON/OFF を切り替えられるようにする。
    if (g_debugGBufferView == view)
    {
        g_debugGBufferView = NSRender::DebugGBufferView::None;
    }
    else
    {
        g_debugGBufferView = view;
    }
    g_Render.SetDebugGBufferView(g_debugGBufferView);
}

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

void AddSimpleMeshAtLookAt(const std::wstring& filePath,
                           const float scale,
                           const bool useSSS = false,
                           const bool usePOM = false,
                           const bool usePointLight = false)
{
    // 生成位置はカメラ位置そのものではなく LookAt 先を使い、
    // 「見ている対象の近くへ置く」操作感になるようにしている。
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);

    if (useSSS)
    {
        const int renderId = g_Render.AddMeshSSS(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale, 1.0f);
        RegisterLoadedModel(L"MeshSSS", filePath, pos, scale, renderId);
    }
    else if (usePOM)
    {
        const int renderId = g_Render.AddMeshPOM(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale, 1.0f);
        RegisterLoadedModel(L"MeshPOM", filePath, pos, scale, renderId);
    }
    else if (usePointLight)
    {
        const int renderId = g_Render.AddMeshPointLight(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale);
        RegisterLoadedModel(L"MeshPointLight", filePath, pos, scale, renderId);
    }
    else
    {
        const int renderId = g_Render.AddMesh(filePath, pos, D3DXVECTOR3(0, yaw, 0.0f), scale, 1.0f);
        RegisterLoadedModel(L"Mesh", filePath, pos, scale, renderId);
    }
}

void AddNormalMappedMeshAtLookAt()
{
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const int renderId = g_Render.AddMeshNormalMapping(L"..\\..\\Sample\\res\\model2\\cubeNormal.x",
                                                       L"..\\..\\Sample\\res\\model2\\normalMap.png",
                                                       pos,
                                                       D3DXVECTOR3(0, yaw, 0.0f),
                                                       1.0f);
    RegisterLoadedModel(L"MeshNormalMap", L"..\\..\\Sample\\res\\model2\\cubeNormal.x", pos, 1.0f, renderId);
}

void AddSkinAnimMeshAtLookAt()
{
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const int renderId = g_Render.AddSkinAnimMesh(L"..\\..\\Sample\\res\\model\\wolf.x",
                                                  pos,
                                                  D3DXVECTOR3(0, yaw, 0.0f),
                                                  3.0f,
                                                  CreateDefaultAnimSetMap());
    RegisterLoadedModel(L"SkinAnimMesh", L"..\\..\\Sample\\res\\model\\wolf.x", pos, 3.0f, renderId);
}

void AddAnimMeshAtLookAt()
{
    auto pos = g_Render.GetLookAtPos();
    D3DXVECTOR3 forward = g_Render.GetCameraRotate();
    D3DXVec3Normalize(&forward, &forward);

    const float yaw = atan2f(forward.x, forward.z);
    const int renderId = g_Render.AddAnimMesh(L"..\\..\\Sample\\res\\model2\\enemyOrangeCube.x",
                                              pos,
                                              D3DXVECTOR3(0, yaw, 0.0f),
                                              1.0f,
                                              CreateDefaultAnimSetMap());
    RegisterLoadedModel(L"AnimMesh", L"..\\..\\Sample\\res\\model2\\enemyOrangeCube.x", pos, 1.0f, renderId);
}

void AddImageOrPointLight(const bool shift, const bool control)
{
    if (control)
    {
        AddPointLightAtLookAt();
        return;
    }

    if (shift)
    {
        g_imageInfoList.clear();
        return;
    }

    ImageInfo imageInfo;
    imageInfo.m_imageName = L"..\\Sample\\res\\2D_image\\cursor.png";
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
    // 押しっぱなし移動はフラグ管理なので、
    // キーを離したタイミングでは対応フラグを落とすだけにする。
    SetMovementFlag(wParam, false);
    return true;
}

bool HandleSampleKeyDown(HWND hWnd, const WPARAM wParam)
{
    // 同じキーでも Shift/Ctrl 組み合わせで役割が変わるため、
    // まず修飾キー状態を確定してから分岐している。
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    if (wParam == VK_F1)
    {
        ToggleSettingsDialog(hWnd);
        return true;
    }

    if (wParam == VK_F2)
    {
        g_bShowOverlay = !g_bShowOverlay;
        return true;
    }

    if (wParam == VK_F3)
    {
        ToggleDebugGBufferView(NSRender::DebugGBufferView::WorldPos);
        return true;
    }

    if (wParam == VK_F4)
    {
        ToggleDebugGBufferView(NSRender::DebugGBufferView::Normal);
        return true;
    }

    if (wParam == VK_F5)
    {
        ToggleDebugGBufferView(NSRender::DebugGBufferView::Depth);
        return true;
    }

    if (wParam == VK_F6)
    {
        ToggleDebugGBufferView(NSRender::DebugGBufferView::Thickness);
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
        g_windowMode = NSRender::eWindowMode::WINDOW;
        g_Render.ChangeWindowMode(NSRender::eWindowMode::WINDOW);
        RefreshSettingsDialogState();
    }
    else if (wParam == '9')
    {
        g_windowMode = NSRender::eWindowMode::BORDERLESS;
        g_Render.ChangeWindowMode(NSRender::eWindowMode::BORDERLESS);
        RefreshSettingsDialogState();
    }
    else if (wParam == '0')
    {
        g_windowMode = NSRender::eWindowMode::FULLSCREEN;
        g_Render.ChangeWindowMode(NSRender::eWindowMode::FULLSCREEN);
        RefreshSettingsDialogState();
    }

    if (wParam == 'M' && !shift && !control)
    {
        SpawnMeshMixAtLookAt(L"..\\..\\Sample\\res\\model2\\monkey.blend.x");
    }
    else if (wParam == 'M' && shift && control)
    {
        AddSimpleMeshAtLookAt(L"..\\..\\Sample\\res\\model2\\cube.x", 1.0f);
    }
    else if (wParam == 'M' && shift && !control)
    {
        AddSimpleMeshAtLookAt(L"..\\..\\Sample\\res\\model2\\cubePOM.blend.x", 1.0f, false, true);
    }
    else if (wParam == 'M' && !shift && control)
    {
        AddSimpleMeshAtLookAt(L"..\\..\\Sample\\res\\model2\\monkey.blend.x", 1.0f, true);
    }

    if (wParam == 'N' && shift && !control)
    {
        AddNormalMappedMeshAtLookAt();
    }
    else if (wParam == 'N' && !shift && !control)
    {
        AddAnimMeshAtLookAt();
    }

    if (wParam == 'K')
    {
        AddSkinAnimMeshAtLookAt();
    }

    if (wParam == 'I')
    {
        auto pos = g_Render.GetLookAtPos();
        D3DXVECTOR3 forward = g_Render.GetCameraRotate();
        D3DXVec3Normalize(&forward, &forward);
        const float yaw = atan2f(forward.x, forward.z);
        const int renderId = g_Render.AddMeshInstansing(L"..\\..\\Sample\\res\\model2\\cube.x", pos, D3DXVECTOR3(0, yaw, 0.0f), 1.0f);
        RegisterLoadedModel(L"Instancing", L"..\\..\\Sample\\res\\model2\\cube.x", pos, 1.0f, renderId);
    }

    if (wParam == 'O')
    {
        AddSimpleMeshAtLookAt(L"..\\..\\Sample\\res\\model2\\cube.x", 1.0f, false, false, true);
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

    if (wParam == 'U')
    {
        if (g_depthOfFieldMode == NSRender::DepthOfFieldMode::Disabled)
        {
            g_depthOfFieldMode = NSRender::DepthOfFieldMode::Enabled;
        }
        else if (g_depthOfFieldMode == NSRender::DepthOfFieldMode::Enabled)
        {
            g_depthOfFieldMode = NSRender::DepthOfFieldMode::AutoNear;
        }
        else
        {
            g_depthOfFieldMode = NSRender::DepthOfFieldMode::Disabled;
        }
        ApplyDepthOfFieldMode();
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
        g_bSSAO2 = !g_bSSAO2;
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
