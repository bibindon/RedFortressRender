#pragma comment(lib, "comctl32.lib")

#include "RenderSettingsDialog.h"

#include <algorithm>
#include <commctrl.h>
#include <commdlg.h>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <vector>

#include "Render.h"

namespace NSRender
{
namespace
{
constexpr const wchar_t* RENDER_SETTINGS_DIALOG_CLASS_NAME = L"NSRenderSettingsDialog";
constexpr int RENDER_SETTINGS_CONTENT_BOTTOM_MARGIN = 24;

struct RenderSettingsDialogState
{
    Render* render = nullptr;
    int scrollPos = 0;
    int contentHeight = 0;
    D3DXCOLOR fogColor = D3DXCOLOR(0.72f, 0.78f, 0.86f, 1.0f);
    D3DXVECTOR3 godRayColor = D3DXVECTOR3(1.0f, 0.9f, 0.8f);
    D3DXVECTOR3 godRayPos = D3DXVECTOR3(1000.0f, 100.0f, 1000.0f);
    D3DXCOLOR pointLightColor = D3DXCOLOR(1.0f, 0.35f, 0.10f, 1.0f);
    float pointLightBrightness = 1.0f;
    PointLightShape pointLightShape = PointLightShape::Point;
    float cameraNearPlane = 0.1f;
    float cameraFarPlane = 30000.0f;
    float cameraShakeDuration = 1.0f;
    float cameraShakeIntensity = 0.12f;
    float gBufferNearPlane = 0.1f;
    float gBufferFarPlane = 30.0f;
    ParticleEffectPreset particleEffectPreset = ParticleEffectPreset::Smoke;
    std::wstring pbrMeshPath;
    std::wstring pbrEnvMapPath;
    std::wstring maskedGaussianMaskPath;
    struct ChildPlacement
    {
        HWND hWnd = NULL;
        RECT rect { };
    };
    std::vector<ChildPlacement> childPlacements;
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
};

void SetDefaultGuiFont(HWND hWnd)
{
    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void CreateSettingsStatic(HWND parent, const wchar_t* text, const int x, const int y, const int w, const int h)
{
    HWND control = CreateWindowExW(0,
                                   L"STATIC",
                                   text,
                                   WS_CHILD | WS_VISIBLE,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   NULL,
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
}

HWND CreateSettingsCheckbox(HWND parent, const int id, const wchar_t* text, const int x, const int y, const int w, const int h)
{
    HWND control = CreateWindowExW(0,
                                   L"BUTTON",
                                   text,
                                   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
    return control;
}

HWND CreateSettingsRadio(HWND parent, const int id, const wchar_t* text, const int x, const int y, const int w, const int h)
{
    HWND control = CreateWindowExW(0,
                                   L"BUTTON",
                                   text,
                                   WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
    return control;
}

void CreateSettingsGroupBox(HWND parent, const wchar_t* text, const int x, const int y, const int w, const int h)
{
    HWND control = CreateWindowExW(0,
                                   L"BUTTON",
                                   text,
                                   WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   NULL,
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
}

void CreateSettingsButton(HWND parent, const wchar_t* text, const int x, const int y, const int w, const int h, const int id = 0)
{
    HWND control = CreateWindowExW(0,
                                   L"BUTTON",
                                   text,
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
}

void CreateSettingsEdit(HWND parent, const wchar_t* text, const int x, const int y, const int w, const int h, const int id = 0)
{
    HWND control = CreateWindowExW(WS_EX_CLIENTEDGE,
                                   L"EDIT",
                                   text,
                                   WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
}

void SetNearestValueEditText(HWND hWnd, HWND trackbar, const wchar_t* text)
{
    RECT sliderRect { };
    GetWindowRect(trackbar, &sliderRect);
    POINT sliderTopLeft { sliderRect.left, sliderRect.top };
    POINT sliderBottomRight { sliderRect.right, sliderRect.bottom };
    ScreenToClient(hWnd, &sliderTopLeft);
    ScreenToClient(hWnd, &sliderBottomRight);

    HWND bestEdit = NULL;
    int bestDistance = 1000000;
    for (HWND child = GetWindow(hWnd, GW_CHILD); child != NULL; child = GetWindow(child, GW_HWNDNEXT))
    {
        wchar_t className[16] { };
        GetClassNameW(child, className, static_cast<int>(sizeof(className) / sizeof(className[0])));
        if (std::wcscmp(className, L"Edit") != 0)
        {
            continue;
        }

        RECT editRect { };
        GetWindowRect(child, &editRect);
        POINT editTopLeft { editRect.left, editRect.top };
        POINT editBottomRight { editRect.right, editRect.bottom };
        ScreenToClient(hWnd, &editTopLeft);
        ScreenToClient(hWnd, &editBottomRight);

        const int verticalDistance = std::abs(((editTopLeft.y + editBottomRight.y) / 2) -
                                              ((sliderTopLeft.y + sliderBottomRight.y) / 2));
        const int horizontalDistance = editTopLeft.x - sliderBottomRight.x;
        if (verticalDistance <= 12 && horizontalDistance >= -4)
        {
            const int distance = verticalDistance * 1000 + horizontalDistance;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestEdit = child;
            }
        }
    }

    if (bestEdit != NULL)
    {
        SetWindowTextW(bestEdit, text);
    }
}

float SliderToFloat(const int sliderPos, const float minValue, const float maxValue)
{
    const float t = static_cast<float>((std::max)(0, (std::min)(100, sliderPos))) / 100.0f;
    return minValue + (maxValue - minValue) * t;
}

int SliderToInt(const int sliderPos, const int minValue, const int maxValue)
{
    const float value = SliderToFloat(sliderPos, static_cast<float>(minValue), static_cast<float>(maxValue));
    return static_cast<int>(value + 0.5f);
}

void SetEditFloat(HWND hWnd, HWND trackbar, const float value, const wchar_t* format = L"%.2f")
{
    wchar_t buffer[32] { };
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), format, value);
    SetNearestValueEditText(hWnd, trackbar, buffer);
}

void SetEditInt(HWND hWnd, HWND trackbar, const int value)
{
    wchar_t buffer[32] { };
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", value);
    SetNearestValueEditText(hWnd, trackbar, buffer);
}

HWND CreateSettingsCombo(HWND parent, const int id, const int x, const int y, const int w, const int h)
{
    HWND control = CreateWindowExW(0,
                                   L"COMBOBOX",
                                   L"",
                                   WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
    return control;
}

void CreateSettingsTrackbar(HWND parent,
                            const int id,
                            const int x,
                            const int y,
                            const int w,
                            const int h,
                            const int minValue,
                            const int maxValue,
                            const int currentValue)
{
    HWND control = CreateWindowExW(0,
                                   TRACKBAR_CLASSW,
                                   L"",
                                   WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandle(NULL),
                                   NULL);
    SendMessage(control, TBM_SETRANGE, TRUE, MAKELPARAM(minValue, maxValue));
    SendMessage(control, TBM_SETPOS, TRUE, currentValue);
    SendMessage(control, TBM_SETTICFREQ, 50, 0);
}

HWND CreateSettingsListView(HWND parent,
                            const int x,
                            const int y,
                            const int w,
                            const int h,
                            const wchar_t* const* columns,
                            const int* widths,
                            const int columnCount)
{
    HWND control = CreateWindowExW(WS_EX_CLIENTEDGE,
                                   WC_LISTVIEWW,
                                   L"",
                                   WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                   x,
                                   y,
                                   w,
                                   h,
                                   parent,
                                   NULL,
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
    ListView_SetExtendedListViewStyle(control, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    for (int i = 0; i < columnCount; ++i)
    {
        LVCOLUMNW column { };
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<LPWSTR>(columns[i]);
        column.cx = widths[i];
        ListView_InsertColumn(control, i, &column);
    }

    return control;
}

void AddSettingsListViewRow(HWND listView, const int row, const wchar_t* const* values, const int valueCount)
{
    LVITEMW item { };
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = 0;
    item.pszText = const_cast<LPWSTR>(values[0]);
    ListView_InsertItem(listView, &item);

    for (int i = 1; i < valueCount; ++i)
    {
        ListView_SetItemText(listView, row, i, const_cast<LPWSTR>(values[i]));
    }
}

void InitializeRenderSettingsControls(HWND hWnd)
{
    INITCOMMONCONTROLSEX icc { };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    constexpr int left = 10;
    constexpr int width = 494;
    int y = 12;

    CreateSettingsStatic(hWnd, L"Window Mode", left, y + 3, 120, 20);
    HWND windowRadio = CreateSettingsRadio(hWnd, IDC_RENDER_SETTINGS_WINDOW_MODE_WINDOW, L"Window", 158, y, 72, 22);
    CreateSettingsRadio(hWnd, IDC_RENDER_SETTINGS_WINDOW_MODE_BORDERLESS, L"Borderless", 232, y, 88, 22);
    CreateSettingsRadio(hWnd, IDC_RENDER_SETTINGS_WINDOW_MODE_FULLSCREEN, L"Fullscreen", 328, y, 96, 22);
    SendMessage(windowRadio, BM_SETCHECK, BST_CHECKED, 0);

    y += 28;
    CreateSettingsStatic(hWnd, L"Resolution", left, y + 3, 120, 20);
    HWND resolutionCombo = CreateSettingsCombo(hWnd, 31000, 158, y, 128, 120);
    SendMessage(resolutionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1920 x 1080"));
    SendMessage(resolutionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1600 x 900"));
    SendMessage(resolutionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1280 x 720"));
    SendMessage(resolutionCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsStatic(hWnd, L"Quality", 306, y + 3, 60, 20);
    HWND qualityCombo = CreateSettingsCombo(hWnd, 31001, 398, y, 106, 120);
    SendMessage(qualityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"LOW"));
    SendMessage(qualityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MIDDLE"));
    SendMessage(qualityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"HIGH"));
    SendMessage(qualityCombo, CB_SETCURSEL, 0, 0);

    y += 26;
    CreateSettingsCheckbox(hWnd, 31002, L"Remote Desktop", 158, y, 124, 22);
    CreateSettingsButton(hWnd, L"Load CSV...", 286, y - 3, 92, 24);

    y += 24;
    CreateSettingsCheckbox(hWnd, 31003, L"Move x100", 158, y, 124, 22);

    y += 30;
    CreateSettingsGroupBox(hWnd, L"Camera", left - 4, y, width + 8, 88);
    CreateSettingsStatic(hWnd, L"Camera Near", 22, y + 24, 100, 20);
    CreateSettingsEdit(hWnd, L"0.100", 158, y + 22, 72, 20, 41000);
    CreateSettingsStatic(hWnd, L"Camera Far", 264, y + 24, 100, 20);
    CreateSettingsEdit(hWnd, L"30000.0", 400, y + 22, 104, 20, 41001);
    CreateSettingsButton(hWnd, L"Shake", 22, y + 50, 56, 24, 41002);
    CreateSettingsStatic(hWnd, L"Sec", 88, y + 54, 28, 20);
    CreateSettingsTrackbar(hWnd, 31004, 122, y + 48, 94, 32, 0, 100, 35);
    CreateSettingsEdit(hWnd, L"1.0", 226, y + 50, 34, 20, 41004);
    CreateSettingsStatic(hWnd, L"Power", 286, y + 54, 44, 20);
    CreateSettingsTrackbar(hWnd, 31005, 326, y + 48, 98, 32, 0, 100, 20);
    CreateSettingsEdit(hWnd, L"0.12", 436, y + 50, 68, 20, 41005);

    y += 98;
    CreateSettingsGroupBox(hWnd, L"GBuffer", left - 4, y, width + 8, 62);
    CreateSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_GBUFFER_ENABLE, L"GBuffer", 22, y + 18, 110, 22);
    CreateSettingsStatic(hWnd, L"GBuffer Near", 22, y + 42, 100, 18);
    CreateSettingsEdit(hWnd, L"0.100", 158, y + 38, 72, 20, 41010);
    CreateSettingsStatic(hWnd, L"GBuffer Far", 264, y + 42, 100, 18);
    CreateSettingsEdit(hWnd, L"30.0", 400, y + 38, 104, 20, 41011);

    y += 72;
    CreateSettingsGroupBox(hWnd, L"Post Effects", left - 4, y, width + 8, 90);
    CreateSettingsCheckbox(hWnd, 31006, L"ZShadow", 22, y + 18, 88, 22);
    CreateSettingsCheckbox(hWnd, 31007, L"SSGI", 118, y + 18, 70, 22);
    CreateSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_SSAO_ENABLE, L"SSAO", 188, y + 18, 72, 22);
    CreateSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_FOG_ENABLE, L"Fog", 264, y + 18, 62, 22);
    CreateSettingsCheckbox(hWnd, 31008, L"Height Fog", 344, y + 18, 104, 22);
    CreateSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_SATURATE_ENABLE, L"Saturate Filter", 22, y + 40, 116, 22);
    CreateSettingsRadio(hWnd, 31009, L"DOF Off", 160, y + 40, 72, 22);
    CreateSettingsRadio(hWnd, 31010, L"DOF On", 234, y + 40, 72, 22);
    CreateSettingsRadio(hWnd, 31011, L"DOF Aut", 306, y + 40, 76, 22);
    SendDlgItemMessage(hWnd, 31009, BM_SETCHECK, BST_CHECKED, 0);
    CreateSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_BLOOM_ENABLE, L"Bloom", 384, y + 40, 72, 22);
    CreateSettingsCheckbox(hWnd, 31012, L"StarBurst", 456, y + 40, 92, 22);
    CreateSettingsCheckbox(hWnd, 31013, L"GodRay", 22, y + 62, 88, 22);
    CreateSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_GAUSSIAN_ENABLE, L"Gaussian blur", 118, y + 62, 124, 22);
    CreateSettingsCheckbox(hWnd, 31014, L"Halo", 360, y + 62, 72, 22);

    y += 100;
    CreateSettingsGroupBox(hWnd, L"Common", left - 4, y, width + 8, 38);
    CreateSettingsCheckbox(hWnd, 31015, L"Animate Light", 22, y + 16, 124, 22);

    y += 48;
    CreateSettingsGroupBox(hWnd, L"Phong", left - 4, y, width + 8, 330);
    int row = y + 28;
    const wchar_t* leftLabels[] = {
        L"Sun Light", L"Ambient", L"Lambert Sat", L"Lambert Darkness", L"Specular Intensity",
        L"Specular Edge", L"EnvMap Blend", L"SSS Int", L"SSS R", L"SSS G", L"SSS B", L"Model Load Scale"
    };
    const wchar_t* leftValues[] = {
        L"1.0", L"1.0", L"2.00", L"0.30", L"0.10", L"0.00", L"1.00", L"1.00", L"1.00", L"1.00", L"0.50", L"1.0"
    };
    for (int i = 0; i < 12; ++i)
    {
        CreateSettingsStatic(hWnd, leftLabels[i], 22, row + 4, 136, 18);
        CreateSettingsTrackbar(hWnd, 31100 + i, 164, row, 124, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, leftValues[i], 296, row + 2, 40, 20, 41100 + i);
        row += 22;
    }

    CreateSettingsCheckbox(hWnd, 31120, L"SSS", 22, y + 184, 72, 22);
    CreateSettingsCheckbox(hWnd, 31121, L"Treat Texture As White", 348, y + 128, 154, 22);
    CreateSettingsCheckbox(hWnd, 31122, L"Use Override", 348, y + 150, 132, 22);
    CreateSettingsCheckbox(hWnd, 31123, L"Use Override", 348, y + 172, 132, 22);
    CreateSettingsStatic(hWnd, L"Fresne", 348, y + 202, 52, 18);
    CreateSettingsTrackbar(hWnd, 31124, 386, y + 194, 82, 30, 0, 100, 8);
    CreateSettingsEdit(hWnd, L"0.08", 474, y + 198, 30, 20, 41124);

    row = y + 28;
    const wchar_t* colorLabels[] = { L"Sun R", L"Sun G", L"Sun B", L"Amb R", L"Amb G", L"Amb B" };
    const wchar_t* colorValues[] = { L"1.00", L"1.00", L"1.00", L"0.20", L"0.20", L"0.20" };
    for (int i = 0; i < 6; ++i)
    {
        CreateSettingsStatic(hWnd, colorLabels[i], 348, row + 4, 54, 18);
        CreateSettingsTrackbar(hWnd, 31200 + i, 438, row, 30, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, colorValues[i], 474, row + 2, 30, 20, 41200 + i);
        row += 18;
    }

    y += 342;
    CreateSettingsStatic(hWnd, L"MeshMixManager (x)", 24, y + 5, 138, 18);
    CreateSettingsEdit(hWnd, L"", 162, y, 124, 20);
    CreateSettingsButton(hWnd, L"Open...", 298, y - 2, 42, 24);
    CreateSettingsCheckbox(hWnd, 31300, L"HighQuality", 350, y, 104, 22);

    y += 28;
    CreateSettingsStatic(hWnd, L"Mesh Instancing (x)", 24, y + 5, 138, 18);
    CreateSettingsEdit(hWnd, L"", 162, y, 124, 20);
    CreateSettingsButton(hWnd, L"Open...", 298, y - 2, 42, 24);
    CreateSettingsCheckbox(hWnd, 31301, L"HighQuality", 350, y, 104, 22);

    y += 28;
    CreateSettingsStatic(hWnd, L"MeshMix Skin Anim (x)", 24, y + 5, 138, 18);
    CreateSettingsEdit(hWnd, L"", 162, y, 124, 20);
    CreateSettingsButton(hWnd, L"Open...", 298, y - 2, 42, 24);
    CreateSettingsButton(hWnd, L"Load XFileList...", 350, y - 2, 116, 24);
    CreateSettingsCheckbox(hWnd, 31302, L"Clip", 476, y, 52, 22);

    y += 28;
    CreateSettingsButton(hWnd, L"Open NonAnim...", 24, y, 154, 24);
    CreateSettingsButton(hWnd, L"Open AnimOnly...", 190, y, 154, 24);
    CreateSettingsButton(hWnd, L"Load Split Anim", 350, y, 158, 24);

    y += 32;
    CreateSettingsStatic(hWnd, L"Loaded Models", 24, y + 2, 120, 18);
    y += 20;
    const wchar_t* loadedColumns[] = { L"Type", L"File", L"Scale", L"Pos" };
    const int loadedWidths[] = { 72, 110, 40, 120 };
    HWND loadedList = CreateSettingsListView(hWnd, 24, y, 482, 80, loadedColumns, loadedWidths, 4);
    const wchar_t* loadedRow0[] = { L"MeshMixM...", L"..\\..\\Sample\\res\\...", L"1.0", L"(0.0, 0.0, 0.0)" };
    const wchar_t* loadedRow1[] = { L"MeshMixM...", L"..\\..\\Sample\\res\\...", L"1.0", L"(0.0, 0.0, 0.0)" };
    AddSettingsListViewRow(loadedList, 0, loadedRow0, 4);
    AddSettingsListViewRow(loadedList, 1, loadedRow1, 4);

    y += 104;
    CreateSettingsStatic(hWnd, L"Animation", 24, y + 2, 120, 18);
    y += 20;
    const wchar_t* animationColumns[] = { L"Name", L"File", L"Mode" };
    const int animationWidths[] = { 72, 150, 54 };
    CreateSettingsListView(hWnd, 24, y, 482, 100, animationColumns, animationWidths, 3);

    y += 124;
    CreateSettingsStatic(hWnd, L"Point Lights", 24, y + 2, 120, 18);
    y += 20;
    const wchar_t* pointLightColumns[] = { L"Pos", L"Type", L"Color", L"Bright..." };
    const int pointLightWidths[] = { 88, 56, 84, 54 };
    CreateSettingsListView(hWnd, 24, y, 482, 68, pointLightColumns, pointLightWidths, 4);

    y += 78;
    const wchar_t* pointLabels[] = { L"PointLight R", L"PointLight G", L"PointLight B", L"PointLight Power" };
    const wchar_t* pointValues[] = { L"1.00", L"0.35", L"0.10", L"1.00" };
    for (int i = 0; i < 4; ++i)
    {
        CreateSettingsStatic(hWnd, pointLabels[i], 24, y + 4, 132, 18);
        CreateSettingsTrackbar(hWnd, 31400 + i, 168, y, 122, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, pointValues[i], 298, y + 2, 40, 20, 41400 + i);
        y += 20;
    }

    CreateSettingsStatic(hWnd, L"PointLight Type", 352, y - 74, 96, 18);
    HWND pointTypeCombo = CreateSettingsCombo(hWnd, 31410, 446, y - 78, 66, 120);
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Point"));
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Line"));
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Square"));
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Cube"));
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sphere"));
    SendMessage(pointTypeCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsButton(hWnd, L"Add", 382, y + 12, 54, 24, 31411);

    y += 62;
    CreateSettingsGroupBox(hWnd, L"PBR", 8, y, 504, 178);
    CreateSettingsStatic(hWnd, L"MeshPBR (x)", 24, y + 24, 132, 18);
    CreateSettingsEdit(hWnd, L"", 162, y + 20, 124, 20, 32210);
    CreateSettingsButton(hWnd, L"Open...", 298, y + 18, 42, 24, 32211);
    CreateSettingsStatic(hWnd, L"EnvMap Image", 24, y + 46, 132, 18);
    CreateSettingsEdit(hWnd, L"", 162, y + 42, 124, 20, 32212);
    CreateSettingsButton(hWnd, L"Open...", 298, y + 40, 42, 24, 32213);

    const wchar_t* pbrLabels[] = { L"PBR Roughness", L"PBR Metallic", L"Env Refl Int", L"Env Max Mip", L"Env Diffuse", L"Env Diff Mip" };
    const wchar_t* pbrValues[] = { L"0.850", L"0.000", L"0.050", L"5.000", L"0.800", L"3.000" };
    int pbrY = y + 68;
    for (int i = 0; i < 6; ++i)
    {
        CreateSettingsStatic(hWnd, pbrLabels[i], 24, pbrY + 4, 132, 18);
        CreateSettingsTrackbar(hWnd, 31500 + i, 168, pbrY, 122, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, pbrValues[i], 298, pbrY + 2, 40, 20, 41500 + i);
        pbrY += 20;
    }

    y += 190;
    CreateSettingsGroupBox(hWnd, L"ZShadow", 8, y, 504, 126);
    const wchar_t* zShadowLabels[] = { L"ZShadow Intensity", L"ZShadow Saturation", L"ZShadow Range" };
    const wchar_t* zShadowValues[] = { L"0.25", L"0.50", L"0.05" };
    int zY = y + 28;
    for (int i = 0; i < 3; ++i)
    {
        CreateSettingsStatic(hWnd, zShadowLabels[i], 24, zY + 4, 142, 18);
        CreateSettingsTrackbar(hWnd, 31600 + i, 168, zY, 122, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, zShadowValues[i], 298, zY + 2, 40, 20, 41600 + i);
        zY += 20;
    }
    CreateSettingsStatic(hWnd, L"ZShadow PCF Taps", 24, zY + 4, 142, 18);
    HWND pcfCombo = CreateSettingsCombo(hWnd, 31610, 266, zY, 72, 120);
    SendMessage(pcfCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1"));
    SendMessage(pcfCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"3"));
    SendMessage(pcfCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"5"));
    SendMessage(pcfCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"7"));
    SendMessage(pcfCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"9"));
    SendMessage(pcfCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"11"));
    SendMessage(pcfCombo, CB_SETCURSEL, 0, 0);
    zY += 20;
    CreateSettingsStatic(hWnd, L"ZShadow Composite Taps", 24, zY + 4, 176, 18);
    HWND compositeCombo = CreateSettingsCombo(hWnd, 31611, 266, zY, 72, 120);
    SendMessage(compositeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1"));
    SendMessage(compositeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"3"));
    SendMessage(compositeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"5"));
    SendMessage(compositeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"7"));
    SendMessage(compositeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"9"));
    SendMessage(compositeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"11"));
    SendMessage(compositeCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsStatic(hWnd, L"ZShadowTexSize", 266, zY + 28, 96, 18);
    HWND zTexSizeCombo = CreateSettingsCombo(hWnd, 31612, 366, zY + 24, 94, 120);
    SendMessage(zTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/1"));
    SendMessage(zTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/2"));
    SendMessage(zTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/4"));
    SendMessage(zTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/8"));
    SendMessage(zTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/16"));
    SendMessage(zTexSizeCombo, CB_SETCURSEL, 0, 0);

    y += 140;
    CreateSettingsGroupBox(hWnd, L"SSGI", 8, y, 504, 104);
    CreateSettingsStatic(hWnd, L"SSGI Sample Count", 24, y + 24, 138, 18);
    HWND ssgiSampleCombo = CreateSettingsCombo(hWnd, 31700, 266, y + 20, 72, 120);
    SendMessage(ssgiSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"8"));
    SendMessage(ssgiSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"16"));
    SendMessage(ssgiSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"32"));
    SendMessage(ssgiSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"64"));
    SendMessage(ssgiSampleCombo, CB_SETCURSEL, 1, 0);
    CreateSettingsStatic(hWnd, L"Indirect light", 24, y + 46, 138, 18);
    CreateSettingsEdit(hWnd, L"1.00", 266, y + 42, 72, 20, 41710);
    CreateSettingsStatic(hWnd, L"Indirect light max", 24, y + 68, 138, 18);
    CreateSettingsEdit(hWnd, L"1.00", 266, y + 64, 72, 20, 41711);
    CreateSettingsStatic(hWnd, L"SSGI Dist Scale", 24, y + 90, 138, 18);
    CreateSettingsTrackbar(hWnd, 31701, 168, y + 84, 122, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"1.00", 298, y + 86, 40, 20, 41701);
    CreateSettingsCheckbox(hWnd, 31702, L"SSGI Blur", 350, y + 20, 100, 22);
    CreateSettingsStatic(hWnd, L"SSGI Blur Size", 350, y + 46, 92, 18);
    HWND ssgiBlurSizeCombo = CreateSettingsCombo(hWnd, 31703, 442, y + 42, 64, 120);
    SendMessage(ssgiBlurSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"21x21"));
    SendMessage(ssgiBlurSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"11x11"));
    SendMessage(ssgiBlurSizeCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsCheckbox(hWnd, 31704, L"Separable Blur", 350, y + 68, 130, 22);

    y += 114;
    CreateSettingsGroupBox(hWnd, L"SSAO", 8, y, 504, 198);
    CreateSettingsStatic(hWnd, L"SSAO Sample Radius", 24, y + 24, 142, 18);
    CreateSettingsTrackbar(hWnd, 31800, 168, y + 18, 122, 30, 0, 100, 15);
    CreateSettingsEdit(hWnd, L"1.00", 298, y + 20, 40, 20, 41800);
    CreateSettingsStatic(hWnd, L"SSAO TexSize", 350, y + 24, 92, 18);
    HWND ssaoTexSizeCombo = CreateSettingsCombo(hWnd, 31801, 442, y + 20, 64, 120);
    SendMessage(ssaoTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/1"));
    SendMessage(ssaoTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/2"));
    SendMessage(ssaoTexSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1/4"));
    SendMessage(ssaoTexSizeCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsStatic(hWnd, L"SSAO Blur Size", 350, y + 54, 92, 18);
    HWND ssaoBlurSizeCombo = CreateSettingsCombo(hWnd, 31802, 442, y + 50, 64, 120);
    SendMessage(ssaoBlurSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"21x21"));
    SendMessage(ssaoBlurSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"11x11"));
    SendMessage(ssaoBlurSizeCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsStatic(hWnd, L"SSAO Shadow Strength", 24, y + 74, 142, 18);
    CreateSettingsTrackbar(hWnd, 31803, 168, y + 68, 122, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"1.00", 298, y + 70, 40, 20, 41803);
    CreateSettingsStatic(hWnd, L"SSAO Shadow Saturation", 24, y + 96, 154, 18);
    CreateSettingsTrackbar(hWnd, 31804, 168, y + 90, 122, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"0.30", 298, y + 92, 40, 20, 41804);
    CreateSettingsStatic(hWnd, L"SSAO Sample Count", 24, y + 138, 142, 18);
    HWND ssaoSampleCombo = CreateSettingsCombo(hWnd, 31805, 266, y + 134, 72, 120);
    SendMessage(ssaoSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"8"));
    SendMessage(ssaoSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"16"));
    SendMessage(ssaoSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"32"));
    SendMessage(ssaoSampleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"64"));
    SendMessage(ssaoSampleCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsCheckbox(hWnd, 31806, L"SSAO 3x3 Gauss", 350, y + 76, 144, 22);
    CreateSettingsCheckbox(hWnd, 31807, L"Separable Blur", 350, y + 98, 130, 22);
    CreateSettingsCheckbox(hWnd, 31808, L"SSAO Blur", 350, y + 120, 120, 22);
    CreateSettingsCheckbox(hWnd, 31809, L"Depth Scaled Dist", 350, y + 142, 146, 22);
    CreateSettingsCheckbox(hWnd, 31810, L"Random Dir", 350, y + 164, 110, 22);
    CreateSettingsCheckbox(hWnd, 31811, L"Clamp 50% Dark", 350, y + 186, 142, 22);

    y += 210;
    CreateSettingsGroupBox(hWnd, L"FOG", 8, y, 504, 86);
    const wchar_t* fogLabels[] = { L"Fog Density", L"Fog Color R", L"Fog Color G", L"Fog Color B" };
    const wchar_t* fogValues[] = { L"1.0", L"0.72", L"0.78", L"0.86" };
    int fogY = y + 18;
    for (int i = 0; i < 4; ++i)
    {
        CreateSettingsStatic(hWnd, fogLabels[i], 24, fogY + 4, 138, 18);
        CreateSettingsTrackbar(hWnd, 31900 + i, 168, fogY, 122, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, fogValues[i], 298, fogY + 2, 40, 20, 41900 + i);
        fogY += 16;
    }

    y += 90;
    CreateSettingsGroupBox(hWnd, L"HeightFOG", 8, y, 504, 110);
    const wchar_t* heightFogLabels[] = {
        L"HeightFog Density",
        L"HeightFog Start",
        L"HeightFog Max",
        L"HeightFog Dist Start",
        L"HeightFog Dist Max"
    };
    const wchar_t* heightFogValues[] = { L"0.30", L"0.0", L"-5.0", L"0.0", L"20.0" };
    int heightFogY = y + 18;
    for (int i = 0; i < 5; ++i)
    {
        CreateSettingsStatic(hWnd, heightFogLabels[i], 24, heightFogY + 4, 138, 18);
        CreateSettingsTrackbar(hWnd, 31920 + i, 168, heightFogY, 122, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, heightFogValues[i], 298, heightFogY + 2, 40, 20, 41920 + i);
        heightFogY += 18;
    }

    y += 124;
    CreateSettingsGroupBox(hWnd, L"Saturate", 8, y, 504, 50);
    CreateSettingsStatic(hWnd, L"Saturation", 24, y + 22, 54, 18);
    CreateSettingsEdit(hWnd, L"0.7", 78, y + 18, 80, 20, 41940);
    CreateSettingsTrackbar(hWnd, 31940, 174, y + 14, 96, 30, 0, 100, 30);
    CreateSettingsButton(hWnd, L"-", 286, y + 16, 20, 22);
    CreateSettingsButton(hWnd, L"+", 308, y + 16, 20, 22);
    CreateSettingsButton(hWnd, L"Reset", 330, y + 16, 40, 22);

    y += 64;
    CreateSettingsGroupBox(hWnd, L"DOF", 8, y, 504, 118);
    const wchar_t* dofLabels[] = { L"DOF Focal Dist", L"DOF Max Blur Dist", L"DOF Auto Dist", L"DOF Start Near" };
    const wchar_t* dofValues[] = { L"3.0", L"8.0", L"3.0", L"1.0" };
    int dofY = y + 42;
    for (int i = 0; i < 4; ++i)
    {
        CreateSettingsStatic(hWnd, dofLabels[i], 24, dofY + 4, 138, 18);
        CreateSettingsTrackbar(hWnd, 31950 + i, 168, dofY, 122, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, dofValues[i], 298, dofY + 2, 40, 20, 41950 + i);
        dofY += 18;
    }

    y += 132;
    CreateSettingsGroupBox(hWnd, L"Bloom", 8, y, 504, 58);
    CreateSettingsStatic(hWnd, L"Bloom Threshold", 24, y + 22, 138, 18);
    CreateSettingsTrackbar(hWnd, 31970, 168, y + 16, 122, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"1.0", 298, y + 18, 40, 20, 41970);
    CreateSettingsStatic(hWnd, L"Sum", 350, y + 22, 34, 18);
    CreateSettingsTrackbar(hWnd, 31971, 386, y + 16, 58, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"1", 458, y + 18, 46, 20, 41971);
    CreateSettingsStatic(hWnd, L"Halo Threshold", 24, y + 42, 138, 18);
    CreateSettingsTrackbar(hWnd, 31972, 168, y + 36, 122, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"1.0", 298, y + 38, 40, 20, 41972);

    y += 70;
    CreateSettingsGroupBox(hWnd, L"StarBurst", 8, y, 504, 68);
    CreateSettingsStatic(hWnd, L"StarBurst Threshold", 24, y + 22, 138, 18);
    CreateSettingsTrackbar(hWnd, 31980, 168, y + 16, 122, 30, 0, 100, 30);
    CreateSettingsEdit(hWnd, L"0.6", 298, y + 18, 40, 20, 41980);
    CreateSettingsStatic(hWnd, L"StarBurst Dist Fade", 24, y + 44, 138, 18);
    CreateSettingsTrackbar(hWnd, 31981, 168, y + 38, 122, 30, 0, 100, 5);
    CreateSettingsEdit(hWnd, L"0.00", 298, y + 40, 40, 20, 41981);

    y += 82;
    CreateSettingsGroupBox(hWnd, L"GodRay", 8, y, 504, 158);
    const wchar_t* godRayLabels[] = {
        L"GodRay R",
        L"GodRay G",
        L"GodRay B",
        L"GodRay Intensity",
        L"GodRay Virtual Prox",
        L"GodRay Pos X",
        L"GodRay Pos Y",
        L"GodRay Pos Z"
    };
    const wchar_t* godRayValues[] = { L"1.00", L"0.90", L"0.80", L"0.10", L"1.50", L"1000.0", L"100.0", L"1000.0" };
    int godRayY = y + 20;
    for (int i = 0; i < 8; ++i)
    {
        CreateSettingsStatic(hWnd, godRayLabels[i], 24, godRayY + 4, 138, 18);
        CreateSettingsTrackbar(hWnd, 32000 + i, 168, godRayY, 122, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, godRayValues[i], 298, godRayY + 2, 40, 20, 42000 + i);
        godRayY += 16;
    }

    y += 170;
    CreateSettingsGroupBox(hWnd, L"Gaussian", 8, y, 504, 68);
    CreateSettingsCheckbox(hWnd, 32020, L"Gaussian", 408, y + 16, 96, 22);
    CreateSettingsStatic(hWnd, L"Blur Strength", 24, y + 46, 138, 18);
    CreateSettingsTrackbar(hWnd, 32021, 168, y + 40, 292, 30, 0, 100, 96);
    CreateSettingsEdit(hWnd, L"1.00", 476, y + 42, 30, 20, 42021);

    y += 80;
    CreateSettingsGroupBox(hWnd, L"Masked Gaussian", 8, y, 504, 78);
    CreateSettingsEdit(hWnd, L"", 162, y + 24, 242, 20, 32220);
    CreateSettingsButton(hWnd, L"Open...", 416, y + 22, 90, 24, 32221);
    CreateSettingsCheckbox(hWnd, 32030, L"Masked G", 408, y + 48, 96, 22);

    y += 90;
    CreateSettingsGroupBox(hWnd, L"PostEffectAA", 8, y, 504, 58);
    CreateSettingsCheckbox(hWnd, 32100, L"PAA", 366, y + 18, 64, 22);
    CreateSettingsCheckbox(hWnd, 32101, L"TAA", 438, y + 18, 64, 22);
    CreateSettingsStatic(hWnd, L"TAA Weight", 384, y + 40, 88, 18);
    CreateSettingsEdit(hWnd, L"0.85", 474, y + 36, 34, 20, 42101);

    y += 70;
    CreateSettingsGroupBox(hWnd, L"Motion Blur", 8, y, 84 + 420, 82);
    CreateSettingsStatic(hWnd, L"Max Blur Px", 24, y + 22, 138, 18);
    CreateSettingsTrackbar(hWnd, 32110, 168, y + 16, 296, 30, 0, 100, 78);
    CreateSettingsEdit(hWnd, L"24", 478, y + 18, 30, 20, 42110);
    CreateSettingsStatic(hWnd, L"Sample Count", 24, y + 44, 138, 18);
    CreateSettingsTrackbar(hWnd, 32111, 168, y + 38, 296, 30, 0, 100, 58);
    CreateSettingsEdit(hWnd, L"13", 478, y + 40, 30, 20, 42111);
    CreateSettingsCheckbox(hWnd, 32112, L"Motion Blur", 372, y + 60, 118, 22);

    y += 96;
    CreateSettingsGroupBox(hWnd, L"FXAA", 8, y, 504, 58);
    CreateSettingsStatic(hWnd, L"FXAA Quality", 24, y + 22, 138, 18);
    CreateSettingsTrackbar(hWnd, 32120, 168, y + 16, 296, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"4", 478, y + 18, 30, 20, 42120);

    y += 64;
    CreateSettingsStatic(hWnd, L"FontEx Blur Size", 16, y + 6, 140, 18);
    CreateSettingsTrackbar(hWnd, 32130, 168, y, 122, 30, 0, 100, 50);
    CreateSettingsEdit(hWnd, L"21", 300, y + 2, 40, 20, 42130);

    y += 34;
    CreateSettingsStatic(hWnd, L"Particle", 16, y + 6, 76, 18);
    HWND particleCombo = CreateSettingsCombo(hWnd, 32140, 96, y + 2, 242, 120);
    SendMessage(particleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Smoke"));
    SendMessage(particleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Fire"));
    SendMessage(particleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Dust"));
    SendMessage(particleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Fog"));
    SendMessage(particleCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Rain"));
    SendMessage(particleCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsButton(hWnd, L"Place At LookAt", 352, y, 154, 24, 32141);

    y += 30;
    CreateSettingsButton(hWnd, L"OK", 310, y, 88, 24, IDOK);
    CreateSettingsButton(hWnd, L"Cancel", 424, y, 88, 24, IDCANCEL);
}

bool IsSettingsCheckboxChecked(HWND hWnd, const int id)
{
    return SendDlgItemMessage(hWnd, id, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

int GetSettingsComboSelection(HWND hWnd, const int id)
{
    return static_cast<int>(SendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0));
}

int ComboIndexToTapCount(const int index)
{
    const int tapCounts[] = { 1, 3, 5, 7, 9, 11 };
    if (index < 0 || index >= static_cast<int>(sizeof(tapCounts) / sizeof(tapCounts[0])))
    {
        return 1;
    }
    return tapCounts[index];
}

int ComboIndexToTexSizeDivisor(const int index)
{
    const int divisors[] = { 1, 2, 4, 8, 16 };
    if (index < 0 || index >= static_cast<int>(sizeof(divisors) / sizeof(divisors[0])))
    {
        return 1;
    }
    return divisors[index];
}

int ComboIndexToSSAOBlurKernelSize(const int index)
{
    const int sizes[] = { 21, 11, 5, 3 };
    if (index < 0 || index >= static_cast<int>(sizeof(sizes) / sizeof(sizes[0])))
    {
        return 21;
    }
    return sizes[index];
}

int ComboIndexToSampleCount(const int index)
{
    const int counts[] = { 8, 16, 32, 64 };
    if (index < 0 || index >= static_cast<int>(sizeof(counts) / sizeof(counts[0])))
    {
        return 8;
    }
    return counts[index];
}

std::wstring ComboIndexToRenderingQuality(const int index)
{
    if (index == 1)
    {
        return L"MIDDLE";
    }
    if (index == 2)
    {
        return L"HIGH";
    }
    return L"LOW";
}

ParticleEffectPreset ComboIndexToParticleEffectPreset(const int index)
{
    switch (index)
    {
    case 1:
        return ParticleEffectPreset::Fire;
    case 2:
        return ParticleEffectPreset::Dust;
    case 3:
        return ParticleEffectPreset::Fog;
    case 4:
        return ParticleEffectPreset::Rain;
    default:
        return ParticleEffectPreset::Smoke;
    }
}

PointLightShape ComboIndexToPointLightShape(const int index)
{
    switch (index)
    {
    case 1:
        return PointLightShape::Line;
    case 2:
        return PointLightShape::Square;
    case 3:
        return PointLightShape::Cube;
    case 4:
        return PointLightShape::Sphere;
    default:
        return PointLightShape::Point;
    }
}

bool ShowSettingsOpenFileDialog(HWND owner, const wchar_t* filter, std::wstring& path)
{
    wchar_t fileName[MAX_PATH] { };
    if (!path.empty())
    {
        wcsncpy_s(fileName, path.c_str(), _TRUNCATE);
    }

    OPENFILENAMEW ofn { };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(fileName) / sizeof(fileName[0]));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn))
    {
        return false;
    }

    path = fileName;
    return true;
}

bool TryGetSettingsEditFloat(HWND hWnd, const int id, float& value)
{
    wchar_t buffer[64] { };
    GetDlgItemTextW(hWnd, id, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    try
    {
        value = std::stof(buffer);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool TryGetSettingsEditInt(HWND hWnd, const int id, int& value)
{
    wchar_t buffer[64] { };
    GetDlgItemTextW(hWnd, id, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
    try
    {
        value = std::stoi(buffer);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void SetTrackbarFromFloat(HWND hWnd, const int trackbarId, const float value, const float minValue, const float maxValue)
{
    const float normalized = (maxValue != minValue) ? ((value - minValue) / (maxValue - minValue)) : 0.0f;
    const int sliderPos = (std::max)(0, (std::min)(100, static_cast<int>(normalized * 100.0f + 0.5f)));
    SendDlgItemMessageW(hWnd, trackbarId, TBM_SETPOS, TRUE, sliderPos);
}

void SetTrackbarFromInt(HWND hWnd, const int trackbarId, const int value, const int minValue, const int maxValue)
{
    SetTrackbarFromFloat(hWnd, trackbarId, static_cast<float>(value), static_cast<float>(minValue), static_cast<float>(maxValue));
}

BOOL CALLBACK CaptureRenderSettingsChildPlacementProc(HWND child, LPARAM lParam)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(lParam);
    if (state == nullptr)
    {
        return TRUE;
    }

    RECT rect { };
    GetWindowRect(child, &rect);

    POINT topLeft { rect.left, rect.top };
    POINT bottomRight { rect.right, rect.bottom };
    HWND parent = GetParent(child);
    ScreenToClient(parent, &topLeft);
    ScreenToClient(parent, &bottomRight);

    RenderSettingsDialogState::ChildPlacement placement;
    placement.hWnd = child;
    placement.rect.left = topLeft.x;
    placement.rect.top = topLeft.y;
    placement.rect.right = bottomRight.x;
    placement.rect.bottom = bottomRight.y;
    state->childPlacements.push_back(placement);
    return TRUE;
}

void CaptureRenderSettingsChildPlacements(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }

    state->childPlacements.clear();
    EnumChildWindows(hWnd, CaptureRenderSettingsChildPlacementProc, reinterpret_cast<LPARAM>(state));

    int maxBottom = 0;
    for (const auto& placement : state->childPlacements)
    {
        maxBottom = (std::max)(maxBottom, static_cast<int>(placement.rect.bottom));
    }
    state->contentHeight = maxBottom + RENDER_SETTINGS_CONTENT_BOTTOM_MARGIN;
}

void ApplyRenderSettingsChildPositions(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }

    for (const auto& placement : state->childPlacements)
    {
        const int width = placement.rect.right - placement.rect.left;
        const int height = placement.rect.bottom - placement.rect.top;
        SetWindowPos(placement.hWnd,
                     NULL,
                     placement.rect.left,
                     placement.rect.top - state->scrollPos,
                     width,
                     height,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

bool HandleRenderSettingsEditCommit(HWND hWnd, const int id)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Render* render = (state != nullptr) ? state->render : nullptr;
    if (render == nullptr)
    {
        return false;
    }

    float floatValue = 0.0f;
    int intValue = 0;
    if (id >= 41100 && id <= 41107 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        const int index = id - 41100;
        if (index == 0)
        {
            render->SetLightBrightness(floatValue);
            SetTrackbarFromFloat(hWnd, 31100, floatValue, 0.0f, 5.0f);
        }
        else if (index == 1)
        {
            render->SetAmbientLightBrightness(floatValue);
            SetTrackbarFromFloat(hWnd, 31101, floatValue, 0.0f, 5.0f);
        }
        else if (index == 2)
        {
            render->SetMeshMixSaturateShadowIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31102, floatValue, 0.0f, 4.0f);
        }
        else if (index == 3)
        {
            render->SetMeshMixShadowDarkness(floatValue);
            SetTrackbarFromFloat(hWnd, 31103, floatValue, 0.0f, 1.0f);
        }
        else if (index == 4)
        {
            render->SetMeshMixSpecularIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31104, floatValue, 0.0f, 1.0f);
        }
        else if (index == 5)
        {
            render->SetMeshMixSpecularEdge(floatValue);
            SetTrackbarFromFloat(hWnd, 31105, floatValue, 0.0f, 1.0f);
        }
        else if (index == 6)
        {
            render->SetMeshMixEnvMapBlend(floatValue);
            SetTrackbarFromFloat(hWnd, 31106, floatValue, 0.0f, 1.0f);
        }
        else if (index == 7)
        {
            render->SetMeshMixSSSIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31107, floatValue, 0.0f, 2.0f);
        }
        return true;
    }

    if (id == 41124 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetMeshMixFresnelIntensity(floatValue);
        SetTrackbarFromFloat(hWnd, 31124, floatValue, 0.0f, 1.0f);
        return true;
    }

    if (id >= 41000 && id <= 41011 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41000)
        {
            state->cameraNearPlane = floatValue;
            render->SetCameraClipPlanes(state->cameraNearPlane, state->cameraFarPlane);
        }
        else if (id == 41001)
        {
            state->cameraFarPlane = floatValue;
            render->SetCameraClipPlanes(state->cameraNearPlane, state->cameraFarPlane);
        }
        else if (id == 41004)
        {
            state->cameraShakeDuration = floatValue;
            render->SetCameraShakeDuration(state->cameraShakeDuration);
            SetTrackbarFromFloat(hWnd, 31004, state->cameraShakeDuration, 0.0f, 5.0f);
        }
        else if (id == 41005)
        {
            state->cameraShakeIntensity = floatValue;
            render->SetCameraShakeIntensity(state->cameraShakeIntensity);
            SetTrackbarFromFloat(hWnd, 31005, state->cameraShakeIntensity, 0.0f, 1.0f);
        }
        else if (id == 41010)
        {
            state->gBufferNearPlane = floatValue;
            render->SetGBufferClipPlanes(state->gBufferNearPlane, state->gBufferFarPlane);
        }
        else if (id == 41011)
        {
            state->gBufferFarPlane = floatValue;
            render->SetGBufferClipPlanes(state->gBufferNearPlane, state->gBufferFarPlane);
        }
        return true;
    }

    if (id >= 41400 && id <= 41403 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41400) state->pointLightColor.r = floatValue;
        if (id == 41401) state->pointLightColor.g = floatValue;
        if (id == 41402) state->pointLightColor.b = floatValue;
        if (id == 41403) state->pointLightBrightness = floatValue;
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, (id == 41403) ? 4.0f : 1.0f);
        return true;
    }

    if (id >= 41500 && id <= 41505 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        const int index = id - 41500;
        if (index == 0) render->SetMeshPBRRoughness(floatValue);
        if (index == 1) render->SetMeshPBRMetallic(floatValue);
        if (index == 2) render->SetMeshPBREnvReflectionIntensity(floatValue);
        if (index == 3) render->SetMeshPBREnvMaxMipLevel(floatValue);
        if (index == 4) render->SetMeshPBREnvDiffuseIntensity(floatValue);
        if (index == 5) render->SetMeshPBREnvDiffuseMipLevel(floatValue);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, (index == 3 || index == 5) ? 10.0f : 1.0f);
        return true;
    }

    if (id >= 41600 && id <= 41602 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41600) render->SetPostEffectDepthBufferShadowIntensity(floatValue);
        if (id == 41601) render->SetPostEffectDepthBufferShadowSaturationBoost(floatValue);
        if (id == 41602) render->SetPostEffectDepthBufferShadowCoverage(floatValue);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 1.0f);
        return true;
    }

    if (id == 41710 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSSGIIndirectLightStrength(floatValue);
        return true;
    }
    if (id == 41711 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSSGIIndirectLightMaxContribution(floatValue);
        return true;
    }
    if (id == 41701 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSSGISampleRadius(floatValue);
        SetTrackbarFromFloat(hWnd, 31701, floatValue, 0.1f, 10.0f);
        return true;
    }

    if ((id == 41800 || id == 41803 || id == 41804) && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41800) render->SetPostEffectSSAOSampleRadius(floatValue);
        if (id == 41803) render->SetPostEffectSSAOShadowStrength(floatValue);
        if (id == 41804) render->SetPostEffectSSAOSaturationBoost(floatValue);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, (id == 41800) ? 0.1f : 0.0f, (id == 41800) ? 10.0f : 2.0f);
        return true;
    }

    if (id >= 41900 && id <= 41903 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41900)
        {
            render->SetPostEffectFogIntensity(floatValue);
            SetTrackbarFromFloat(hWnd, 31900, floatValue, 0.0f, 2.0f);
        }
        else
        {
            if (id == 41901) state->fogColor.r = floatValue;
            if (id == 41902) state->fogColor.g = floatValue;
            if (id == 41903) state->fogColor.b = floatValue;
            render->SetPostEffectFogColor(state->fogColor);
            SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 1.0f);
        }
        return true;
    }

    if (id >= 41920 && id <= 41924 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41920) render->SetPostEffectHeightFogIntensity(floatValue);
        if (id == 41921) render->SetPostEffectHeightFogStart(floatValue);
        if (id == 41922) render->SetPostEffectHeightFogMax(floatValue);
        if (id == 41923) render->SetPostEffectHeightFogDistanceStart(floatValue);
        if (id == 41924) render->SetPostEffectHeightFogDistanceMax(floatValue);
        if (id == 41920) SetTrackbarFromFloat(hWnd, 31920, floatValue, 0.0f, 2.0f);
        if (id == 41921 || id == 41922) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, -20.0f, 20.0f);
        if (id == 41923 || id == 41924) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 100.0f);
        return true;
    }

    if (id == 41940 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectSaturate(floatValue);
        SetTrackbarFromFloat(hWnd, 31940, floatValue, 0.0f, 2.0f);
        return true;
    }

    if (id >= 41950 && id <= 41953 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41950) render->SetPostEffectDepthOfFieldFocalDistance(floatValue);
        if (id == 41951) render->SetPostEffectDepthOfFieldMaxBlurDistance(floatValue);
        if (id == 41952) render->SetPostEffectDepthOfFieldAutoActivationDistance(floatValue);
        if (id == 41953) render->SetPostEffectDepthOfFieldStartNear(floatValue);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, (id == 41951) ? 32.0f : 20.0f);
        return true;
    }

    if ((id >= 41970 && id <= 41972 || id >= 41980 && id <= 41981) && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 41970) render->SetPostEffectBloomThreshold(floatValue);
        if (id == 41971) render->SetPostEffectBloomWeightSum(floatValue);
        if (id == 41972) render->SetPostEffectHaloThreshold(floatValue);
        if (id == 41980) render->SetPostEffectStarBurstThreshold(floatValue);
        if (id == 41981) render->SetPostEffectStarBurstDistanceFade(floatValue);
        SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, (id == 41971) ? 4.0f : ((id == 41981) ? 1.0f : 5.0f));
        return true;
    }

    if (id >= 42000 && id <= 42007 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        if (id == 42000) state->godRayColor.x = floatValue;
        if (id == 42001) state->godRayColor.y = floatValue;
        if (id == 42002) state->godRayColor.z = floatValue;
        if (id >= 42000 && id <= 42002) render->SetPostEffectGodRayLightColor(state->godRayColor);
        if (id == 42003) render->SetPostEffectGodRayIntensity(floatValue);
        if (id == 42004) render->SetPostEffectGodRayVirtualProximityStrength(floatValue);
        if (id == 42005) state->godRayPos.x = floatValue;
        if (id == 42006) state->godRayPos.y = floatValue;
        if (id == 42007) state->godRayPos.z = floatValue;
        if (id >= 42005 && id <= 42007) render->SetPostEffectGodRayLightPos(state->godRayPos);
        if (id <= 42003) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, 0.0f, 1.0f);
        if (id == 42004) SetTrackbarFromFloat(hWnd, 32004, floatValue, 0.0f, 4.0f);
        if (id >= 42005) SetTrackbarFromFloat(hWnd, id - 10000, floatValue, -2000.0f, 2000.0f);
        return true;
    }

    if (id == 42021 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectGaussianStrength(floatValue);
        SetTrackbarFromFloat(hWnd, 32021, floatValue, 0.0f, 1.0f);
        return true;
    }

    if (id == 42101 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectTAAHistoryWeight(floatValue);
        return true;
    }
    if (id == 42110 && TryGetSettingsEditFloat(hWnd, id, floatValue))
    {
        render->SetPostEffectMotionBlurCameraMaxBlurPixels(floatValue);
        SetTrackbarFromFloat(hWnd, 32110, floatValue, 1.0f, 64.0f);
        return true;
    }
    if (id == 42111 && TryGetSettingsEditInt(hWnd, id, intValue))
    {
        render->SetPostEffectMotionBlurCameraSampleCount(intValue);
        SetTrackbarFromInt(hWnd, 32111, intValue, 1, 32);
        return true;
    }
    if (id == 42120 && TryGetSettingsEditInt(hWnd, id, intValue))
    {
        render->SetPostEffectFXAAQuality(intValue);
        SetTrackbarFromInt(hWnd, 32120, intValue, 1, 8);
        return true;
    }
    if (id == 42130 && TryGetSettingsEditInt(hWnd, id, intValue))
    {
        render->SetPostEffectFontSampleSize(intValue);
        SetTrackbarFromInt(hWnd, 32130, intValue, 1, 31);
        return true;
    }

    return false;
}

void HandleRenderSettingsCommand(HWND hWnd, const WPARAM wParam)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Render* render = (state != nullptr) ? state->render : nullptr;
    if (render == nullptr)
    {
        return;
    }

    const int id = LOWORD(wParam);
    const int notifyCode = HIWORD(wParam);

    if (notifyCode == EN_KILLFOCUS && HandleRenderSettingsEditCommit(hWnd, id))
    {
        return;
    }

    if (notifyCode == BN_CLICKED)
    {
        if (id == IDC_RENDER_SETTINGS_SATURATE_ENABLE)
        {
            render->SetPostEffectSaturateEnable(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_GBUFFER_ENABLE)
        {
            render->SetGBufferEnable(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_GAUSSIAN_ENABLE)
        {
            render->SetPostEffectGaussianFilter(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_BLOOM_ENABLE)
        {
            render->SetPostEffectBloom(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31006)
        {
            render->SetPostEffectDepthBufferShadow(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31007)
        {
            render->SetPostEffectSSGI(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_SSAO_ENABLE)
        {
            render->SetPostEffectSSAO(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_FOG_ENABLE)
        {
            render->SetPostEffectFog(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31008)
        {
            render->SetPostEffectHeightFog(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31009)
        {
            render->SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Disabled);
        }
        else if (id == 31010)
        {
            render->SetPostEffectDepthOfFieldMode(DepthOfFieldMode::Enabled);
        }
        else if (id == 31011)
        {
            render->SetPostEffectDepthOfFieldMode(DepthOfFieldMode::AutoNear);
        }
        else if (id == 31012)
        {
            render->SetPostEffectStarBurst(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31013)
        {
            render->SetPostEffectGodRay(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31014)
        {
            render->SetPostEffectHalo(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31120)
        {
            render->SetMeshMixSSS(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31121)
        {
            render->SetPhongTreatTextureAsWhite(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31122)
        {
            render->SetMeshMixSpecularIntensityOverrideEnabled(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31123)
        {
            render->SetMeshMixSpecularEdgeOverrideEnabled(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31702)
        {
            render->SetPostEffectSSGIBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31704)
        {
            render->SetPostEffectSSGISeparableBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31806)
        {
            render->SetPostEffectSSAOCompositeGaussian3x3(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31807)
        {
            render->SetPostEffectSSAOSeparableBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31808)
        {
            render->SetPostEffectSSAOBlur(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31809)
        {
            render->SetPostEffectSSAODepthScaledSampleDistance(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31810)
        {
            render->SetPostEffectSSAORandomSamplingDirection(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 31811)
        {
            render->SetPostEffectSSAOMaxDarknessClamp(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32020)
        {
            render->SetPostEffectGaussianFilter(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32030)
        {
            render->SetPostEffectMaskedGaussianFilter(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32100)
        {
            render->SetPostEffectAA(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32101)
        {
            render->SetPostEffectTAA(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32112)
        {
            render->SetPostEffectMotionBlurCamera(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == 32141)
        {
            render->PlaceParticleEffect(state->particleEffectPreset, render->GetLookAtPos());
        }
        else if (id == 31411)
        {
            render->AddPointLight(render->GetLookAtPos(),
                                  state->pointLightBrightness,
                                  state->pointLightColor,
                                  state->pointLightShape);
        }
        else if (id == 41002)
        {
            render->SetCameraShakeDuration(state->cameraShakeDuration);
            render->SetCameraShakeIntensity(state->cameraShakeIntensity);
            render->TriggerCameraShake();
        }
        else if (id == 32211)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"PBR Mesh Files (*.x;*.blend.x)\0*.x;*.blend.x\0All Files (*.*)\0*.*\0",
                                           state->pbrMeshPath))
            {
                SetDlgItemTextW(hWnd, 32210, state->pbrMeshPath.c_str());
            }
        }
        else if (id == 32213)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"Cube Environment Map Files (*.dds)\0*.dds\0All Files (*.*)\0*.*\0",
                                           state->pbrEnvMapPath))
            {
                SetDlgItemTextW(hWnd, 32212, state->pbrEnvMapPath.c_str());
                render->SetMeshPBREnvMapTexturePath(state->pbrEnvMapPath);
            }
        }
        else if (id == 32221)
        {
            if (ShowSettingsOpenFileDialog(hWnd,
                                           L"Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.dds;*.tga)\0*.png;*.jpg;*.jpeg;*.bmp;*.dds;*.tga\0All Files (*.*)\0*.*\0",
                                           state->maskedGaussianMaskPath))
            {
                SetDlgItemTextW(hWnd, 32220, state->maskedGaussianMaskPath.c_str());
                render->SetPostEffectMaskedGaussianMaskPath(state->maskedGaussianMaskPath);
            }
        }
        else if (id == IDOK || id == IDCANCEL)
        {
            ShowWindow(hWnd, SW_HIDE);
        }
        else if (id == IDC_RENDER_SETTINGS_WINDOW_MODE_WINDOW)
        {
            render->ChangeWindowMode(eWindowMode::WINDOW);
        }
        else if (id == IDC_RENDER_SETTINGS_WINDOW_MODE_BORDERLESS)
        {
            render->ChangeWindowMode(eWindowMode::BORDERLESS);
        }
        else if (id == IDC_RENDER_SETTINGS_WINDOW_MODE_FULLSCREEN)
        {
            render->ChangeWindowMode(eWindowMode::FULLSCREEN);
        }
        return;
    }

    if (notifyCode == CBN_SELCHANGE)
    {
        if (id == 31610)
        {
            render->SetPostEffectDepthBufferShadowPcfTapCount(ComboIndexToTapCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31001)
        {
            render->SetRenderQuality(ComboIndexToRenderingQuality(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31611)
        {
            render->SetPostEffectDepthBufferShadowCompositeTapCount(ComboIndexToTapCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31612)
        {
            render->SetPostEffectDepthBufferShadowTexSizeDivisor(ComboIndexToTexSizeDivisor(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31700)
        {
            render->SetPostEffectSSGISampleCount(ComboIndexToSampleCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31703)
        {
            render->SetPostEffectSSGIBlurKernelSize(ComboIndexToSSAOBlurKernelSize(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31801)
        {
            render->SetPostEffectSSAOTexSizeDivisor(ComboIndexToTexSizeDivisor(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31802)
        {
            render->SetPostEffectSSAOBlurKernelSize(ComboIndexToSSAOBlurKernelSize(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 31805)
        {
            render->SetPostEffectSSAOSampleCount(ComboIndexToSampleCount(GetSettingsComboSelection(hWnd, id)));
        }
        else if (id == 32140)
        {
            state->particleEffectPreset = ComboIndexToParticleEffectPreset(GetSettingsComboSelection(hWnd, id));
        }
        else if (id == 31410)
        {
            state->pointLightShape = ComboIndexToPointLightShape(GetSettingsComboSelection(hWnd, id));
        }
        return;
    }
}

void UpdateRenderSettingsScrollBar(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }

    RECT clientRect { };
    GetClientRect(hWnd, &clientRect);

    SCROLLINFO scrollInfo { };
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = (std::max)(0, state->contentHeight - 1);
    scrollInfo.nPage = clientRect.bottom - clientRect.top;
    scrollInfo.nPos = state->scrollPos;
    SetScrollInfo(hWnd, SB_VERT, &scrollInfo, TRUE);
}

void ScrollRenderSettingsTo(HWND hWnd, const int newScrollPos)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }

    RECT clientRect { };
    GetClientRect(hWnd, &clientRect);
    const int clientHeight = static_cast<int>(clientRect.bottom - clientRect.top);
    const int maxScrollPos = (std::max)(0, state->contentHeight - clientHeight);
    const int clampedPos = (std::max)(0, (std::min)(newScrollPos, maxScrollPos));
    const int delta = state->scrollPos - clampedPos;
    if (delta == 0)
    {
        return;
    }

    state->scrollPos = clampedPos;
    ApplyRenderSettingsChildPositions(hWnd);
    UpdateRenderSettingsScrollBar(hWnd);
}

void HandleRenderSettingsVScroll(HWND hWnd, const WPARAM wParam)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (state == nullptr)
    {
        return;
    }

    SCROLLINFO scrollInfo { };
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_ALL;
    GetScrollInfo(hWnd, SB_VERT, &scrollInfo);

    int newPos = state->scrollPos;
    switch (LOWORD(wParam))
    {
    case SB_LINEUP:
        newPos -= 24;
        break;
    case SB_LINEDOWN:
        newPos += 24;
        break;
    case SB_PAGEUP:
        newPos -= static_cast<int>(scrollInfo.nPage);
        break;
    case SB_PAGEDOWN:
        newPos += static_cast<int>(scrollInfo.nPage);
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newPos = scrollInfo.nTrackPos;
        break;
    default:
        break;
    }

    ScrollRenderSettingsTo(hWnd, newPos);
}

void HandleRenderSettingsHScroll(HWND hWnd, const LPARAM lParam)
{
    HWND trackbar = reinterpret_cast<HWND>(lParam);
    if (trackbar == NULL)
    {
        return;
    }

    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Render* render = (state != nullptr) ? state->render : nullptr;
    if (render == nullptr)
    {
        return;
    }

    const int id = GetDlgCtrlID(trackbar);
    const int pos = static_cast<int>(SendMessage(trackbar, TBM_GETPOS, 0, 0));
    switch (id)
    {
    case 31004:
        state->cameraShakeDuration = SliderToFloat(pos, 0.0f, 5.0f);
        render->SetCameraShakeDuration(state->cameraShakeDuration);
        SetEditFloat(hWnd, trackbar, state->cameraShakeDuration, L"%.1f");
        break;
    case 31005:
        state->cameraShakeIntensity = SliderToFloat(pos, 0.0f, 1.0f);
        render->SetCameraShakeIntensity(state->cameraShakeIntensity);
        SetEditFloat(hWnd, trackbar, state->cameraShakeIntensity, L"%.2f");
        break;
    case IDC_RENDER_SETTINGS_SATURATE_LEVEL:
    case 31940:
    {
        const float value = SliderToFloat(pos, 0.0f, 2.0f);
        render->SetPostEffectSaturate(value);
        SetEditFloat(hWnd, trackbar, value, L"%.1f");
        break;
    }
    case 31100:
        render->SetLightBrightness(SliderToFloat(pos, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 5.0f), L"%.1f");
        break;
    case 31101:
        render->SetAmbientLightBrightness(SliderToFloat(pos, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 5.0f), L"%.1f");
        break;
    case 31102:
        render->SetMeshMixSaturateShadowIntensity(SliderToFloat(pos, 0.0f, 4.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 4.0f));
        break;
    case 31103:
        render->SetMeshMixShadowDarkness(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31104:
        render->SetMeshMixSpecularIntensity(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31105:
        render->SetMeshMixSpecularEdge(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31106:
        render->SetMeshMixEnvMapBlend(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31107:
        render->SetMeshMixSSSIntensity(SliderToFloat(pos, 0.0f, 2.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 2.0f));
        break;
    case 31124:
        render->SetMeshMixFresnelIntensity(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31400:
    case 31401:
    case 31402:
    {
        const float value = SliderToFloat(pos, 0.0f, 1.0f);
        if (id == 31400) state->pointLightColor.r = value;
        if (id == 31401) state->pointLightColor.g = value;
        if (id == 31402) state->pointLightColor.b = value;
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 31403:
        state->pointLightBrightness = SliderToFloat(pos, 0.0f, 4.0f);
        SetEditFloat(hWnd, trackbar, state->pointLightBrightness);
        break;
    case 31500:
        render->SetMeshPBRRoughness(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f), L"%.3f");
        break;
    case 31501:
        render->SetMeshPBRMetallic(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f), L"%.3f");
        break;
    case 31502:
        render->SetMeshPBREnvReflectionIntensity(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f), L"%.3f");
        break;
    case 31503:
        render->SetMeshPBREnvMaxMipLevel(SliderToFloat(pos, 0.0f, 10.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 10.0f), L"%.3f");
        break;
    case 31504:
        render->SetMeshPBREnvDiffuseIntensity(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f), L"%.3f");
        break;
    case 31505:
        render->SetMeshPBREnvDiffuseMipLevel(SliderToFloat(pos, 0.0f, 10.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 10.0f), L"%.3f");
        break;
    case 31600:
        render->SetPostEffectDepthBufferShadowIntensity(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31601:
        render->SetPostEffectDepthBufferShadowSaturationBoost(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31602:
        render->SetPostEffectDepthBufferShadowCoverage(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 31701:
        render->SetPostEffectSSGISampleRadius(SliderToFloat(pos, 0.1f, 10.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.1f, 10.0f));
        break;
    case 31800:
        render->SetPostEffectSSAOSampleRadius(SliderToFloat(pos, 0.1f, 10.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.1f, 10.0f));
        break;
    case 31803:
        render->SetPostEffectSSAOShadowStrength(SliderToFloat(pos, 0.0f, 2.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 2.0f));
        break;
    case 31804:
        render->SetPostEffectSSAOSaturationBoost(SliderToFloat(pos, 0.0f, 2.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 2.0f));
        break;
    case 31900:
        render->SetPostEffectFogIntensity(SliderToFloat(pos, 0.0f, 2.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 2.0f), L"%.1f");
        break;
    case 31901:
    case 31902:
    case 31903:
    {
        const float value = SliderToFloat(pos, 0.0f, 1.0f);
        if (id == 31901) state->fogColor.r = value;
        if (id == 31902) state->fogColor.g = value;
        if (id == 31903) state->fogColor.b = value;
        render->SetPostEffectFogColor(state->fogColor);
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 31920:
        render->SetPostEffectHeightFogIntensity(SliderToFloat(pos, 0.0f, 2.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 2.0f));
        break;
    case 31921:
        render->SetPostEffectHeightFogStart(SliderToFloat(pos, -20.0f, 20.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, -20.0f, 20.0f), L"%.1f");
        break;
    case 31922:
        render->SetPostEffectHeightFogMax(SliderToFloat(pos, -20.0f, 20.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, -20.0f, 20.0f), L"%.1f");
        break;
    case 31923:
        render->SetPostEffectHeightFogDistanceStart(SliderToFloat(pos, 0.0f, 100.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 100.0f), L"%.1f");
        break;
    case 31924:
        render->SetPostEffectHeightFogDistanceMax(SliderToFloat(pos, 0.0f, 100.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 100.0f), L"%.1f");
        break;
    case 31950:
        render->SetPostEffectDepthOfFieldFocalDistance(SliderToFloat(pos, 0.0f, 20.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 20.0f), L"%.1f");
        break;
    case 31951:
        render->SetPostEffectDepthOfFieldMaxBlurDistance(SliderToFloat(pos, 0.0f, 32.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 32.0f), L"%.1f");
        break;
    case 31952:
        render->SetPostEffectDepthOfFieldAutoActivationDistance(SliderToFloat(pos, 0.0f, 20.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 20.0f), L"%.1f");
        break;
    case 31953:
        render->SetPostEffectDepthOfFieldStartNear(SliderToFloat(pos, 0.0f, 20.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 20.0f), L"%.1f");
        break;
    case 31970:
        render->SetPostEffectBloomThreshold(SliderToFloat(pos, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 5.0f), L"%.1f");
        break;
    case 31971:
        render->SetPostEffectBloomWeightSum(SliderToFloat(pos, 0.0f, 4.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 4.0f), L"%.1f");
        break;
    case 31972:
        render->SetPostEffectHaloThreshold(SliderToFloat(pos, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 5.0f), L"%.1f");
        break;
    case 31980:
        render->SetPostEffectStarBurstThreshold(SliderToFloat(pos, 0.0f, 5.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 5.0f), L"%.1f");
        break;
    case 31981:
        render->SetPostEffectStarBurstDistanceFade(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 32000:
    case 32001:
    case 32002:
    {
        const float value = SliderToFloat(pos, 0.0f, 1.0f);
        if (id == 32000) state->godRayColor.x = value;
        if (id == 32001) state->godRayColor.y = value;
        if (id == 32002) state->godRayColor.z = value;
        render->SetPostEffectGodRayLightColor(state->godRayColor);
        SetEditFloat(hWnd, trackbar, value);
        break;
    }
    case 32003:
        render->SetPostEffectGodRayIntensity(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 32004:
        render->SetPostEffectGodRayVirtualProximityStrength(SliderToFloat(pos, 0.0f, 4.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 4.0f));
        break;
    case 32005:
    case 32006:
    case 32007:
    {
        const float value = SliderToFloat(pos, -2000.0f, 2000.0f);
        if (id == 32005) state->godRayPos.x = value;
        if (id == 32006) state->godRayPos.y = value;
        if (id == 32007) state->godRayPos.z = value;
        render->SetPostEffectGodRayLightPos(state->godRayPos);
        SetEditFloat(hWnd, trackbar, value, L"%.1f");
        break;
    }
    case 32021:
        render->SetPostEffectGaussianStrength(SliderToFloat(pos, 0.0f, 1.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 0.0f, 1.0f));
        break;
    case 32110:
        render->SetPostEffectMotionBlurCameraMaxBlurPixels(SliderToFloat(pos, 1.0f, 64.0f));
        SetEditFloat(hWnd, trackbar, SliderToFloat(pos, 1.0f, 64.0f), L"%.0f");
        break;
    case 32111:
    {
        const int sampleCount = SliderToInt(pos, 1, 32);
        render->SetPostEffectMotionBlurCameraSampleCount(sampleCount);
        SetEditInt(hWnd, trackbar, sampleCount);
        break;
    }
    case 32120:
    {
        const int quality = SliderToInt(pos, 1, 8);
        render->SetPostEffectFXAAQuality(quality);
        SetEditInt(hWnd, trackbar, quality);
        break;
    }
    case 32130:
    {
        const int sampleSize = SliderToInt(pos, 1, 31) | 1;
        render->SetPostEffectFontSampleSize(sampleSize);
        SetEditInt(hWnd, trackbar, sampleSize);
        break;
    }
    default:
        break;
    }
}

LRESULT CALLBACK RenderSettingsDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCCREATE:
    {
        const CREATESTRUCT* createStruct = reinterpret_cast<const CREATESTRUCT*>(lParam);
        RenderSettingsDialogState* state = new RenderSettingsDialogState;
        state->render = reinterpret_cast<Render*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        return TRUE;
    }
    case WM_CREATE:
    {
        InitializeRenderSettingsControls(hWnd);
        CaptureRenderSettingsChildPlacements(hWnd);
        UpdateRenderSettingsScrollBar(hWnd);
        return 0;
    }
    case WM_SIZE:
    {
        ApplyRenderSettingsChildPositions(hWnd);
        UpdateRenderSettingsScrollBar(hWnd);
        return 0;
    }
    case WM_COMMAND:
    {
        HandleRenderSettingsCommand(hWnd, wParam);
        return 0;
    }
    case WM_VSCROLL:
    {
        HandleRenderSettingsVScroll(hWnd, wParam);
        return 0;
    }
    case WM_HSCROLL:
    {
        HandleRenderSettingsHScroll(hWnd, lParam);
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        const short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (state != nullptr)
        {
            ScrollRenderSettingsTo(hWnd, state->scrollPos - ((wheelDelta / WHEEL_DELTA) * 72));
        }
        return 0;
    }
    case WM_CLOSE:
    {
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }
    case WM_NCDESTROY:
    {
        RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        delete state;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
        break;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool EnsureRenderSettingsDialogClass(HINSTANCE hInstance)
{
    WNDCLASSEXW existingClass { };
    existingClass.cbSize = sizeof(existingClass);
    if (GetClassInfoExW(hInstance, RENDER_SETTINGS_DIALOG_CLASS_NAME, &existingClass))
    {
        return true;
    }

    WNDCLASSEXW wc { };
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = RenderSettingsDialogProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = RENDER_SETTINGS_DIALOG_CLASS_NAME;

    return RegisterClassExW(&wc) != 0;
}

void MoveWindowNearParent(HWND hWnd, HWND parent)
{
    if (parent == NULL)
    {
        return;
    }

    RECT parentRect { };
    RECT windowRect { };
    if (!GetWindowRect(parent, &parentRect) || !GetWindowRect(hWnd, &windowRect))
    {
        return;
    }

    const int windowW = windowRect.right - windowRect.left;
    const int windowH = windowRect.bottom - windowRect.top;
    const int gap = 8;

    RECT workArea { };
    HMONITOR monitor = MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo { };
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(monitor, &monitorInfo))
    {
        workArea = monitorInfo.rcWork;
    }
    else
    {
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    }

    int x = parentRect.right + gap;
    int y = parentRect.top;

    if (x + windowW > workArea.right)
    {
        x = parentRect.left + gap;
    }

    if (y + windowH > workArea.bottom)
    {
        y = workArea.bottom - windowH;
    }

    x = (std::max)(static_cast<int>(workArea.left),
                   (std::min)(x, static_cast<int>(workArea.right) - windowW));
    y = (std::max)(static_cast<int>(workArea.top),
                   (std::min)(y, static_cast<int>(workArea.bottom) - windowH));

    SetWindowPos(hWnd,
                 NULL,
                 x,
                 y,
                 windowW,
                 windowH,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}
}

void RenderSettingsDialog::Show(HWND parent, Render* render, const bool activateDialog)
{
    if (parent == NULL || render == nullptr)
    {
        return;
    }

    if (m_hWnd != NULL && !IsWindow(m_hWnd))
    {
        m_hWnd = NULL;
    }

    if (m_hWnd == NULL)
    {
        HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL));
        if (!EnsureRenderSettingsDialogClass(hInstance))
        {
            return;
        }

        RECT rect { 0, 0, 520, 780 };
        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL;
        const DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME;
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        m_hWnd = CreateWindowExW(exStyle,
                                 RENDER_SETTINGS_DIALOG_CLASS_NAME,
                                 L"Settings",
                                 style,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 rect.right - rect.left,
                                 rect.bottom - rect.top,
                                 parent,
                                 NULL,
                                 hInstance,
                                 render);
        if (m_hWnd == NULL)
        {
            return;
        }

        MoveWindowNearParent(m_hWnd, parent);
    }

    ShowWindow(m_hWnd, SW_SHOWNORMAL);
    if (activateDialog)
    {
        SetForegroundWindow(m_hWnd);
        SetFocus(m_hWnd);
    }
    else
    {
        SetForegroundWindow(parent);
        SetFocus(parent);
    }
}

void RenderSettingsDialog::Toggle(HWND parent, Render* render)
{
    if (m_hWnd != NULL && IsWindow(m_hWnd) && IsWindowVisible(m_hWnd))
    {
        ShowWindow(m_hWnd, SW_HIDE);
        SetForegroundWindow(parent);
        SetFocus(parent);
        return;
    }

    Show(parent, render, true);
}

void RenderSettingsDialog::Finalize()
{
    if (m_hWnd != NULL)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

}
