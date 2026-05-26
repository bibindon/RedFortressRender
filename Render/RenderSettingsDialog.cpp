#pragma comment(lib, "comctl32.lib")

#include "RenderSettingsDialog.h"

#include <algorithm>
#include <commctrl.h>
#include <vector>

#include "Render.h"

namespace NSRender
{
namespace
{
constexpr const wchar_t* RENDER_SETTINGS_DIALOG_CLASS_NAME = L"NSRenderSettingsDialog";
constexpr int RENDER_SETTINGS_CONTENT_HEIGHT = 1220;

struct RenderSettingsDialogState
{
    Render* render = nullptr;
    int scrollPos = 0;
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

void CreateSettingsButton(HWND parent, const wchar_t* text, const int x, const int y, const int w, const int h)
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
                                   NULL,
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
}

void CreateSettingsEdit(HWND parent, const wchar_t* text, const int x, const int y, const int w, const int h)
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
                                   NULL,
                                   GetModuleHandle(NULL),
                                   NULL);
    SetDefaultGuiFont(control);
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
    CreateSettingsEdit(hWnd, L"0.100", 158, y + 22, 72, 20);
    CreateSettingsStatic(hWnd, L"Camera Far", 264, y + 24, 100, 20);
    CreateSettingsEdit(hWnd, L"30000.0", 400, y + 22, 104, 20);
    CreateSettingsButton(hWnd, L"Shake", 22, y + 50, 56, 24);
    CreateSettingsStatic(hWnd, L"Sec", 88, y + 54, 28, 20);
    CreateSettingsTrackbar(hWnd, 31004, 122, y + 48, 94, 32, 0, 100, 35);
    CreateSettingsEdit(hWnd, L"1.0", 226, y + 50, 34, 20);
    CreateSettingsStatic(hWnd, L"Power", 286, y + 54, 44, 20);
    CreateSettingsTrackbar(hWnd, 31005, 326, y + 48, 98, 32, 0, 100, 20);
    CreateSettingsEdit(hWnd, L"0.12", 436, y + 50, 68, 20);

    y += 98;
    CreateSettingsGroupBox(hWnd, L"GBuffer", left - 4, y, width + 8, 62);
    CreateSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_GBUFFER_ENABLE, L"GBuffer", 22, y + 18, 110, 22);
    CreateSettingsStatic(hWnd, L"GBuffer Near", 22, y + 42, 100, 18);
    CreateSettingsEdit(hWnd, L"0.100", 158, y + 38, 72, 20);
    CreateSettingsStatic(hWnd, L"GBuffer Far", 264, y + 42, 100, 18);
    CreateSettingsEdit(hWnd, L"30.0", 400, y + 38, 104, 20);

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
        CreateSettingsEdit(hWnd, leftValues[i], 296, row + 2, 40, 20);
        row += 22;
    }

    CreateSettingsCheckbox(hWnd, 31120, L"SSS", 22, y + 184, 72, 22);
    CreateSettingsCheckbox(hWnd, 31121, L"Treat Texture As White", 348, y + 128, 154, 22);
    CreateSettingsCheckbox(hWnd, 31122, L"Use Override", 348, y + 150, 132, 22);
    CreateSettingsCheckbox(hWnd, 31123, L"Use Override", 348, y + 172, 132, 22);
    CreateSettingsStatic(hWnd, L"Fresne", 348, y + 202, 52, 18);
    CreateSettingsTrackbar(hWnd, 31124, 386, y + 194, 82, 30, 0, 100, 8);
    CreateSettingsEdit(hWnd, L"0.08", 474, y + 198, 30, 20);

    row = y + 28;
    const wchar_t* colorLabels[] = { L"Sun R", L"Sun G", L"Sun B", L"Amb R", L"Amb G", L"Amb B" };
    const wchar_t* colorValues[] = { L"1.00", L"1.00", L"1.00", L"0.20", L"0.20", L"0.20" };
    for (int i = 0; i < 6; ++i)
    {
        CreateSettingsStatic(hWnd, colorLabels[i], 348, row + 4, 54, 18);
        CreateSettingsTrackbar(hWnd, 31200 + i, 438, row, 30, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, colorValues[i], 474, row + 2, 30, 20);
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
        CreateSettingsEdit(hWnd, pointValues[i], 298, y + 2, 40, 20);
        y += 20;
    }

    CreateSettingsStatic(hWnd, L"PointLight Type", 352, y - 74, 96, 18);
    HWND pointTypeCombo = CreateSettingsCombo(hWnd, 31410, 446, y - 78, 66, 120);
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Point"));
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Line"));
    SendMessage(pointTypeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Square"));
    SendMessage(pointTypeCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsButton(hWnd, L"Add", 382, y + 12, 54, 24);
}

bool IsSettingsCheckboxChecked(HWND hWnd, const int id)
{
    return SendDlgItemMessage(hWnd, id, BM_GETCHECK, 0, 0) == BST_CHECKED;
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
        else if (id == IDC_RENDER_SETTINGS_SSAO_ENABLE)
        {
            render->SetPostEffectSSAO(IsSettingsCheckboxChecked(hWnd, id));
        }
        else if (id == IDC_RENDER_SETTINGS_FOG_ENABLE)
        {
            render->SetPostEffectFog(IsSettingsCheckboxChecked(hWnd, id));
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
    scrollInfo.nMax = RENDER_SETTINGS_CONTENT_HEIGHT;
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
    const int maxScrollPos = (std::max)(0, RENDER_SETTINGS_CONTENT_HEIGHT - clientHeight);
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
