#include "RenderSettingsDialogInternal.h"
namespace NSRender
{
namespace RenderSettingsDialogInternal
{
bool IsSettingsCheckboxChecked(HWND hWnd, int id)
{
    return SendDlgItemMessage(hWnd, id, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
int GetSettingsComboSelection(HWND hWnd, int id)
{
    return static_cast<int>(SendDlgItemMessage(hWnd, id, CB_GETCURSEL, 0, 0));
}
int ComboIndexToTapCount(int index)
{
    int tapCounts[] = { 1, 3, 5, 7, 9, 11 };
    if (index < 0 || index >= static_cast<int>(sizeof(tapCounts) / sizeof(tapCounts[0])))
    {
        return 1;
    }
    return tapCounts[index];
}
int TapCountToComboIndex(int tapCount)
{
    int tapCounts[] = { 1, 3, 5, 7, 9, 11 };
    for (int i = 0; i < static_cast<int>(sizeof(tapCounts) / sizeof(tapCounts[0])); ++i)
    {
        if (tapCounts[i] == tapCount)
        {
            return i;
        }
    }
    return 0;
}
int ComboIndexToTexSizeDivisor(int index)
{
    int divisors[] = { 1, 2, 4, 8, 16 };
    if (index < 0 || index >= static_cast<int>(sizeof(divisors) / sizeof(divisors[0])))
    {
        return 1;
    }
    return divisors[index];
}
int TexSizeDivisorToComboIndex(int divisor)
{
    int divisors[] = { 1, 2, 4, 8, 16 };
    for (int i = 0; i < static_cast<int>(sizeof(divisors) / sizeof(divisors[0])); ++i)
    {
        if (divisors[i] == divisor)
        {
            return i;
        }
    }
    return 0;
}
int ComboIndexToSSAOBlurKernelSize(int index)
{
    int sizes[] = { 21, 11, 5, 3 };
    if (index < 0 || index >= static_cast<int>(sizeof(sizes) / sizeof(sizes[0])))
    {
        return 21;
    }
    return sizes[index];
}
int BlurKernelSizeToComboIndex(int kernelSize)
{
    int sizes[] = { 21, 11, 5, 3 };
    for (int i = 0; i < static_cast<int>(sizeof(sizes) / sizeof(sizes[0])); ++i)
    {
        if (sizes[i] == kernelSize)
        {
            return i;
        }
    }
    return 0;
}
int ComboIndexToSampleCount(int index)
{
    int counts[] = { 8, 16, 32, 64 };
    if (index < 0 || index >= static_cast<int>(sizeof(counts) / sizeof(counts[0])))
    {
        return 8;
    }
    return counts[index];
}
int SampleCountToComboIndex(int sampleCount)
{
    int counts[] = { 8, 16, 32, 64 };
    for (int i = 0; i < static_cast<int>(sizeof(counts) / sizeof(counts[0])); ++i)
    {
        if (counts[i] == sampleCount)
        {
            return i;
        }
    }
    return 0;
}
std::wstring ComboIndexToRenderingQuality(int index)
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
int RenderingQualityToComboIndex(const std::wstring& quality)
{
    if (quality == L"MIDDLE")
    {
        return 1;
    }
    if (quality == L"HIGH")
    {
        return 2;
    }
    return 0;
}
ParticleEffectPreset ComboIndexToParticleEffectPreset(int index)
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
    case 5:
        return ParticleEffectPreset::Explosion;
    case 6:
        return ParticleEffectPreset::Damage;
    default:
        return ParticleEffectPreset::Smoke;
    }
}
PointLightShape ComboIndexToPointLightShape(int index)
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
bool TryGetSettingsEditFloat(HWND hWnd, int id, float& value)
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
bool TryGetSettingsEditInt(HWND hWnd, int id, int& value)
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
void SetTrackbarFromFloat(HWND hWnd, int trackbarId, float value, float minValue, float maxValue)
{
    HWND trackbar = GetDlgItem(hWnd, trackbarId);
    if (trackbar == NULL)
    {
        return;
    }

    const int sliderMin = static_cast<int>(SendMessageW(trackbar, TBM_GETRANGEMIN, 0, 0));
    const int sliderMax = static_cast<int>(SendMessageW(trackbar, TBM_GETRANGEMAX, 0, 0));
    float normalized = (maxValue != minValue) ? ((value - minValue) / (maxValue - minValue)) : 0.0f;
    normalized = (std::max)(0.0f, (std::min)(1.0f, normalized));
    int sliderPos = sliderMin + static_cast<int>(normalized * static_cast<float>(sliderMax - sliderMin) + 0.5f);
    SendMessageW(trackbar, TBM_SETPOS, TRUE, sliderPos);
}
void SetTrackbarFromInt(HWND hWnd, int trackbarId, int value, int minValue, int maxValue)
{
    SetTrackbarFromFloat(hWnd, trackbarId, static_cast<float>(value), static_cast<float>(minValue), static_cast<float>(maxValue));
}
void SetSettingsCheckbox(HWND hWnd, int id, bool checked)
{
    SendDlgItemMessageW(hWnd, id, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}
void SetSettingsEditTextIfNotFocused(HWND hWnd, int id, const wchar_t* text)
{
    HWND edit = GetDlgItem(hWnd, id);
    if (edit == NULL || GetFocus() == edit)
    {
        return;
    }
    SetWindowTextW(edit, text);
}
void SetSettingsEditFloat(HWND hWnd, int id, float value, const wchar_t* format)
{
    wchar_t buffer[64] { };
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), format, value);
    SetSettingsEditTextIfNotFocused(hWnd, id, buffer);
}
void SetSettingsEditInt(HWND hWnd, int id, int value)
{
    wchar_t buffer[64] { };
    std::swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%d", value);
    SetSettingsEditTextIfNotFocused(hWnd, id, buffer);
}
void SetSettingsComboSelection(HWND hWnd, int id, int index)
{
    if (index >= 0)
    {
        SendDlgItemMessageW(hWnd, id, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }
}
void SetSettingsComboTextSelection(HWND hWnd, int id, const std::wstring& text)
{
    const LRESULT index = SendDlgItemMessageW(hWnd, id, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(text.c_str()));
    if (index != CB_ERR)
    {
        SendDlgItemMessageW(hWnd, id, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }
}
}
}
