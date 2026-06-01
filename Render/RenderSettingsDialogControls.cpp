#include "RenderSettingsDialogInternal.h"
namespace NSRender
{
namespace RenderSettingsDialogInternal
{
void SetDefaultGuiFont(HWND hWnd)
{
    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}
void CreateSettingsStatic(HWND parent, const wchar_t* text, int x, int y, int w, int h)
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
HWND CreateSettingsCheckbox(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h)
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
HWND CreateSettingsRadio(HWND parent, int id, const wchar_t* text, int x, int y, int w, int h)
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
void CreateSettingsGroupBox(HWND parent, const wchar_t* text, int x, int y, int w, int h)
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
void CreateSettingsButton(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id)
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
void CreateSettingsEdit(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id)
{
    (void)h;
    constexpr int EDIT_HEIGHT = 14;
    HWND control = CreateWindowExW(WS_EX_CLIENTEDGE,
                                   L"EDIT",
                                   text,
                                   WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                   x,
                                   y,
                                   w,
                                   EDIT_HEIGHT,
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
        int verticalDistance = std::abs(((editTopLeft.y + editBottomRight.y) / 2) -
                                              ((sliderTopLeft.y + sliderBottomRight.y) / 2));
        int horizontalDistance = editTopLeft.x - sliderBottomRight.x;
        if (verticalDistance <= 12 && horizontalDistance >= -4)
        {
            int distance = verticalDistance * 1000 + horizontalDistance;
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
float SliderToFloat(int sliderPos, float minValue, float maxValue)
{
    float t = static_cast<float>((std::max)(0, (std::min)(100, sliderPos))) / 100.0f;
    return minValue + (maxValue - minValue) * t;
}
int SliderToInt(int sliderPos, int minValue, int maxValue)
{
    float value = SliderToFloat(sliderPos, static_cast<float>(minValue), static_cast<float>(maxValue));
    return static_cast<int>(value + 0.5f);
}

float TrackbarToFloat(HWND trackbar, float minValue, float maxValue)
{
    if (trackbar == NULL)
    {
        return minValue;
    }

    const int sliderPos = static_cast<int>(SendMessageW(trackbar, TBM_GETPOS, 0, 0));
    const int sliderMin = static_cast<int>(SendMessageW(trackbar, TBM_GETRANGEMIN, 0, 0));
    const int sliderMax = static_cast<int>(SendMessageW(trackbar, TBM_GETRANGEMAX, 0, 0));
    const int clampedPos = (std::max)(sliderMin, (std::min)(sliderMax, sliderPos));
    const float t = (sliderMax != sliderMin) ?
        (static_cast<float>(clampedPos - sliderMin) / static_cast<float>(sliderMax - sliderMin)) :
        0.0f;

    return minValue + (maxValue - minValue) * t;
}

int TrackbarToInt(HWND trackbar, int minValue, int maxValue)
{
    const float value = TrackbarToFloat(trackbar, static_cast<float>(minValue), static_cast<float>(maxValue));
    return static_cast<int>(value + 0.5f);
}

void SetEditFloat(HWND hWnd, HWND trackbar, float value, const wchar_t* format)
{
    wchar_t buffer[32] { };
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), format, value);
    const int trackbarId = GetDlgCtrlID(trackbar);
    HWND edit = GetDlgItem(hWnd, trackbarId + 10000);
    if (edit != NULL)
    {
        SetWindowTextW(edit, buffer);
        return;
    }
    SetNearestValueEditText(hWnd, trackbar, buffer);
}
void SetEditInt(HWND hWnd, HWND trackbar, int value)
{
    wchar_t buffer[32] { };
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", value);
    const int trackbarId = GetDlgCtrlID(trackbar);
    HWND edit = GetDlgItem(hWnd, trackbarId + 10000);
    if (edit != NULL)
    {
        SetWindowTextW(edit, buffer);
        return;
    }
    SetNearestValueEditText(hWnd, trackbar, buffer);
}
HWND CreateSettingsCombo(HWND parent, int id, int x, int y, int w, int h)
{
    HWND control = CreateWindowExW(0,
                                   L"COMBOBOX",
                                   L"",
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
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
                            int id,
                            int x,
                            int y,
                            int w,
                            int h,
                            int minValue,
                            int maxValue,
                            int currentValue)
{
    (void)h;
    constexpr int TRACKBAR_HEIGHT = 12;
    HWND control = CreateWindowExW(0,
                                   TRACKBAR_CLASSW,
                                   L"",
                                   WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                                   x,
                                   y,
                                   w,
                                   TRACKBAR_HEIGHT,
                                   parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandle(NULL),
                                   NULL);
    SendMessage(control, TBM_SETRANGE, TRUE, MAKELPARAM(minValue, maxValue));
    SendMessage(control, TBM_SETPOS, TRUE, currentValue);
    SendMessage(control, TBM_SETTICFREQ, (std::max)(1, (maxValue - minValue) / 10), 0);
}

void SetSettingsTrackbarRange(HWND parent, int id, int minValue, int maxValue)
{
    HWND trackbar = GetDlgItem(parent, id);
    if (trackbar == NULL)
    {
        return;
    }

    SendMessageW(trackbar, TBM_SETRANGEMIN, FALSE, minValue);
    SendMessageW(trackbar, TBM_SETRANGEMAX, TRUE, maxValue);
    SendMessageW(trackbar, TBM_SETTICFREQ, (std::max)(1, (maxValue - minValue) / 10), 0);
}
HWND CreateSettingsListView(HWND parent,
                            int id,
                            int x,
                            int y,
                            int w,
                            int h,
                            const wchar_t* const* columns,
                            const int* widths,
                            int columnCount)
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
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
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
void AddSettingsListViewRow(HWND listView, int row, const wchar_t* const* values, int valueCount)
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
std::wstring FormatResolutionLabel(int width, int height)
{
    return std::to_wstring(width) + L" x " + std::to_wstring(height);
}
bool TryParseResolutionLabel(const wchar_t* label, int& width, int& height)
{
    return swscanf_s(label, L"%d x %d", &width, &height) == 2;
}
void InitializeRenderSettingsControls(HWND hWnd, RenderSettingsDialogState* state)
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
    bool addedResolution = false;
    if (state != nullptr && state->render != nullptr)
    {
        const auto resolutionList = state->render->GetResolutionList();
        for (const auto& resolution : resolutionList)
        {
            const std::wstring label = FormatResolutionLabel(resolution.first, resolution.second);
            SendMessage(resolutionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            addedResolution = true;
        }
    }
    if (!addedResolution)
    {
        SendMessage(resolutionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1920 x 1080"));
        SendMessage(resolutionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1600 x 900"));
        SendMessage(resolutionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1280 x 720"));
    }
    SendMessage(resolutionCombo, CB_SETCURSEL, 0, 0);
    CreateSettingsStatic(hWnd, L"Quality", 306, y + 3, 60, 20);
    HWND qualityCombo = CreateSettingsCombo(hWnd, 31001, 398, y, 106, 120);
    SendMessage(qualityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"LOW"));
    SendMessage(qualityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MIDDLE"));
    SendMessage(qualityCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"HIGH"));
    SendMessage(qualityCombo, CB_SETCURSEL, 0, 0);
    y += 26;
    CreateSettingsButton(hWnd, L"Load CSV...", 158, y - 3, 92, 24, 31016);
    y += 30;
    CreateSettingsGroupBox(hWnd, L"Camera", left - 4, y, width + 8, 114);
    CreateSettingsStatic(hWnd, L"Camera Near", 22, y + 24, 100, 20);
    CreateSettingsEdit(hWnd, L"0.100", 158, y + 22, 72, 20, 41000);
    CreateSettingsStatic(hWnd, L"Camera Far", 264, y + 24, 100, 20);
    CreateSettingsEdit(hWnd, L"30000.0", 400, y + 22, 104, 20, 41001);
    CreateSettingsCheckbox(hWnd, 31002, L"Show FPS", 22, y + 50, 96, 22);
    CreateSettingsStatic(hWnd, L"H FOV", 118, y + 52, 56, 20);
    CreateSettingsTrackbar(hWnd, 31003, 174, y + 50, 238, 32, 1, 180, 90);
    CreateSettingsEdit(hWnd, L"90", 424, y + 52, 50, 20, 41003);
    CreateSettingsStatic(hWnd, L"deg", 480, y + 54, 28, 20);
    CreateSettingsButton(hWnd, L"Shake", 22, y + 78, 56, 24, 41002);
    CreateSettingsStatic(hWnd, L"Sec", 88, y + 82, 28, 20);
    CreateSettingsTrackbar(hWnd, 31004, 122, y + 76, 94, 32, 0, 100, 35);
    CreateSettingsEdit(hWnd, L"1.0", 226, y + 78, 34, 20, 41004);
    CreateSettingsStatic(hWnd, L"Power", 286, y + 82, 44, 20);
    CreateSettingsTrackbar(hWnd, 31005, 326, y + 76, 98, 32, 0, 100, 20);
    CreateSettingsEdit(hWnd, L"0.12", 436, y + 78, 68, 20, 41005);
    y += 124;
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
        CreateSettingsStatic(hWnd, colorLabels[i], 348, row + 4, 36, 18);
        CreateSettingsTrackbar(hWnd, 31200 + i, 386, row, 82, 30, 0, 100, 50);
        CreateSettingsEdit(hWnd, colorValues[i], 474, row + 2, 30, 20, 41200 + i);
        row += 18;
    }
    y += 342;
    CreateSettingsStatic(hWnd, L"MeshMixManager (x)", 24, y + 5, 138, 18);
    CreateSettingsEdit(hWnd, L"", 162, y, 124, 20, 31310);
    CreateSettingsButton(hWnd, L"Open...", 298, y - 2, 42, 24, 31311);
    CreateSettingsCheckbox(hWnd, 31300, L"HighQuality", 350, y, 104, 22);
    y += 28;
    CreateSettingsStatic(hWnd, L"Mesh Instancing (x)", 24, y + 5, 138, 18);
    CreateSettingsEdit(hWnd, L"", 162, y, 124, 20, 31320);
    CreateSettingsButton(hWnd, L"Open...", 298, y - 2, 42, 24, 31321);
    CreateSettingsCheckbox(hWnd, 31301, L"HighQuality", 350, y, 104, 22);
    y += 28;
    CreateSettingsStatic(hWnd, L"MeshMix Skin Anim (x)", 24, y + 5, 138, 18);
    CreateSettingsEdit(hWnd, L"", 162, y, 124, 20, 31330);
    CreateSettingsButton(hWnd, L"Open...", 298, y - 2, 42, 24, 31331);
    CreateSettingsButton(hWnd, L"Load XFileList...", 350, y - 2, 116, 24, 31332);
    CreateSettingsCheckbox(hWnd, 31302, L"Clip", 476, y, 52, 22);
    CreateSettingsCheckbox(hWnd, 31303, L"NoAlpha0", 534, y, 76, 22);
    y += 28;
    CreateSettingsButton(hWnd, L"Load Move...", 24, y, 116, 24, 31333);
    CreateSettingsButton(hWnd, L"Reset Move", 148, y, 88, 24, 31334);
    y += 28;
    CreateSettingsButton(hWnd, L"Open NonAnim...", 24, y, 154, 24, 31360);
    CreateSettingsButton(hWnd, L"Open AnimOnly...", 190, y, 154, 24, 31361);
    CreateSettingsButton(hWnd, L"Load Split Anim", 350, y, 158, 24, 31362);
    y += 32;
    CreateSettingsStatic(hWnd, L"Loaded Models", 24, y + 2, 120, 18);
    CreateSettingsButton(hWnd, L"Remove", 430, y - 2, 76, 22, 31350);
    y += 20;
    const wchar_t* loadedColumns[] = { L"Type", L"File", L"Scale", L"Pos" };
    int loadedWidths[] = { 72, 110, 40, 120 };
    HWND loadedList = CreateSettingsListView(hWnd, 31340, 24, y, 482, 80, loadedColumns, loadedWidths, 4);
    if (state != nullptr)
    {
        state->loadedModelsList = loadedList;
    }
    y += 104;
    CreateSettingsStatic(hWnd, L"Animation", 24, y + 2, 120, 18);
    CreateSettingsButton(hWnd, L"Play", 430, y - 2, 76, 22, 31351);
    y += 20;
    const wchar_t* animationColumns[] = { L"Name", L"File", L"Mode" };
    int animationWidths[] = { 72, 150, 54 };
    HWND animationList = CreateSettingsListView(hWnd, 31341, 24, y, 482, 100, animationColumns, animationWidths, 3);
    if (state != nullptr)
    {
        state->animationList = animationList;
    }
    y += 124;
    CreateSettingsStatic(hWnd, L"Point Lights", 24, y + 2, 120, 18);
    CreateSettingsButton(hWnd, L"Remove", 430, y - 2, 76, 22, 31412);
    y += 20;
    const wchar_t* pointLightColumns[] = { L"Pos", L"Type", L"Color", L"Bright..." };
    int pointLightWidths[] = { 88, 56, 84, 54 };
    HWND pointLightList = CreateSettingsListView(hWnd, 31342, 24, y, 482, 68, pointLightColumns, pointLightWidths, 4);
    if (state != nullptr)
    {
        state->pointLightsList = pointLightList;
        UpdatePointLightsList(state);
    }
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
    CreateSettingsGroupBox(hWnd, L"Text", 8, y, 504, 216);
    CreateSettingsStatic(hWnd, L"Text", 24, y + 24, 48, 18);
    CreateSettingsEdit(hWnd, L"サンプルテキスト", 96, y + 22, 268, 20, 32152);
    CreateSettingsCheckbox(hWnd, 32153, L"FontEx", 382, y + 20, 82, 22);
    CreateSettingsButton(hWnd, L"Add", 464, y + 18, 44, 24, 32154);
    CreateSettingsStatic(hWnd, L"X", 24, y + 52, 24, 18);
    CreateSettingsTrackbar(hWnd, 32150, 96, y + 46, 268, 30, 0, 1000, 100);
    CreateSettingsEdit(hWnd, L"0.10", 374, y + 48, 48, 20, 42150);
    CreateSettingsStatic(hWnd, L"Y", 24, y + 78, 24, 18);
    CreateSettingsTrackbar(hWnd, 32151, 96, y + 72, 268, 30, 0, 1000, 100);
    CreateSettingsEdit(hWnd, L"0.10", 374, y + 74, 48, 20, 42151);
    y += 108;
    const wchar_t* textColumns[] = { L"Text", L"X", L"Y", L"FontEx" };
    const int textWidths[] = { 270, 60, 60, 70 };
    state->settingsTextList = CreateSettingsListView(hWnd, 32155, 24, y, 482, 92, textColumns, textWidths, 4);
    y += 108;

    SetSettingsTrackbarRange(hWnd, 31004, 0, 49);
    SetSettingsTrackbarRange(hWnd, 31005, 0, 100);
    SetSettingsTrackbarRange(hWnd, 31100, 0, 50);
    SetSettingsTrackbarRange(hWnd, 31101, 0, 50);
    SetSettingsTrackbarRange(hWnd, 31102, 0, 200);
    SetSettingsTrackbarRange(hWnd, 31103, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31104, 0, 40);
    SetSettingsTrackbarRange(hWnd, 31105, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31106, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31107, 0, 120);
    SetSettingsTrackbarRange(hWnd, 31111, 0, 99);
    SetSettingsTrackbarRange(hWnd, 31124, 0, 40);
    for (int i = 0; i < 6; ++i)
    {
        SetSettingsTrackbarRange(hWnd, 31200 + i, 0, 20);
    }
    for (int i = 0; i < 3; ++i)
    {
        SetSettingsTrackbarRange(hWnd, 31400 + i, 0, 20);
    }
    SetSettingsTrackbarRange(hWnd, 31403, 0, 1000);
    SetSettingsTrackbarRange(hWnd, 31500, 0, 96);
    SetSettingsTrackbarRange(hWnd, 31501, 0, 100);
    SetSettingsTrackbarRange(hWnd, 31502, 0, 60);
    SetSettingsTrackbarRange(hWnd, 31503, 0, 100);
    SetSettingsTrackbarRange(hWnd, 31504, 0, 60);
    SetSettingsTrackbarRange(hWnd, 31505, 0, 100);
    SetSettingsTrackbarRange(hWnd, 31600, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31601, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31602, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31701, 0, 99);
    SetSettingsTrackbarRange(hWnd, 31800, 0, 199);
    SetSettingsTrackbarRange(hWnd, 31803, 0, 80);
    SetSettingsTrackbarRange(hWnd, 31804, 0, 100);
    SetSettingsTrackbarRange(hWnd, 31900, 0, 200);
    SetSettingsTrackbarRange(hWnd, 31901, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31902, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31903, 0, 20);
    SetSettingsTrackbarRange(hWnd, 31920, 0, 100);
    SetSettingsTrackbarRange(hWnd, 31921, 0, 200000);
    SetSettingsTrackbarRange(hWnd, 31922, 0, 200000);
    SetSettingsTrackbarRange(hWnd, 31923, 0, 200000);
    SetSettingsTrackbarRange(hWnd, 31924, 0, 200000);
    SetSettingsTrackbarRange(hWnd, 31940, 0, 40);
    SetSettingsTrackbarRange(hWnd, 31950, 0, 495);
    SetSettingsTrackbarRange(hWnd, 31951, 0, 495);
    SetSettingsTrackbarRange(hWnd, 31952, 0, 495);
    SetSettingsTrackbarRange(hWnd, 31953, 0, 500);
    SetSettingsTrackbarRange(hWnd, 31970, 0, 50);
    SetSettingsTrackbarRange(hWnd, 31971, 1, 100);
    SetSettingsTrackbarRange(hWnd, 31972, 0, 50);
    SetSettingsTrackbarRange(hWnd, 31980, 0, 50);
    SetSettingsTrackbarRange(hWnd, 31981, 0, 20);
    SetSettingsTrackbarRange(hWnd, 32000, 0, 20);
    SetSettingsTrackbarRange(hWnd, 32001, 0, 20);
    SetSettingsTrackbarRange(hWnd, 32002, 0, 20);
    SetSettingsTrackbarRange(hWnd, 32003, 0, 60);
    SetSettingsTrackbarRange(hWnd, 32004, 0, 100);
    SetSettingsTrackbarRange(hWnd, 32005, -200, 200);
    SetSettingsTrackbarRange(hWnd, 32006, -200, 200);
    SetSettingsTrackbarRange(hWnd, 32007, -200, 200);
    SetSettingsTrackbarRange(hWnd, 32021, 0, 100);
    SetSettingsTrackbarRange(hWnd, 32110, 1, 64);
    SetSettingsTrackbarRange(hWnd, 32111, 2, 21);
    SetSettingsTrackbarRange(hWnd, 32120, 1, 8);
    SetSettingsTrackbarRange(hWnd, 32130, 1, 21);
    SetSettingsTrackbarRange(hWnd, 32150, 0, 1000);
    SetSettingsTrackbarRange(hWnd, 32151, 0, 1000);

    CreateSettingsButton(hWnd, L"OK", 310, y, 88, 24, IDOK);
    CreateSettingsButton(hWnd, L"Cancel", 424, y, 88, 24, IDCANCEL);
}
}
}
