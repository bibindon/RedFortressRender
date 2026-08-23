#include "Font.h"

namespace NSRender
{

void Font::Initialize(const std::wstring& fontName,
                      const int fontSize,
                      const UINT fontColor)
{
    HRESULT hResult = E_FAIL;

    m_fontName = fontName;
    m_fontSizeOrigin = fontSize;
    m_fontColor = fontColor;

    CreateResource();

    // デバイスロストを管理する機能にこのリソースを登録
    Common::AddDeviceLostResource(this);
}

void Font::CreateResource()
{
    HRESULT hResult = E_FAIL;

    // モニターの解像度に合わせてフォントサイズを調節する。
    // 1920x1080だったら1600x900の1.2倍なので1.2倍のフォントサイズを指定する
    // 1920x900だったら、横は1.2倍、縦は1倍である。この場合は小さい方の1倍を採用する
    float scaleW = (float)Common::ScreenW() / Common::BASE_W;
    float scaleH = (float)Common::ScreenH() / Common::BASE_H;
    float scaleMin = (std::min)(scaleW, scaleH);

    m_fontSizeScaled = (int)(m_fontSizeOrigin * scaleMin);

    D3DXFONT_DESC desc { };
    desc.Height             = m_fontSizeScaled;

    // 幅は必ず0(自動)を指定する
    desc.Width              = 0;

    desc.Weight             = FW_THIN;
    desc.MipLevels          = 1;
    desc.Italic             = FALSE;
    desc.CharSet            = DEFAULT_CHARSET;
    desc.OutputPrecision    = OUT_TT_ONLY_PRECIS;
    desc.Quality            = CLEARTYPE_NATURAL_QUALITY;
    desc.PitchAndFamily     = FF_DONTCARE;
    wcsncpy_s(desc.FaceName, _countof(desc.FaceName), m_fontName.c_str(), _TRUNCATE);

    hResult = D3DXCreateFontIndirect(Common::D3DDevice(),
                                     &desc,
                                     &m_pFont);

    assert(hResult == S_OK);
}

void Font::AddText(const std::wstring& text,
                      const int X,
                      const int Y)
{
    const POINT pt = Common::ScaledPoint(X, Y);
    AddTextDirect(text, pt.x, pt.y);
}

void Font::AddText(const std::wstring& text,
                       const int X,
                       const int Y,
                       const UINT fontColor)
{
    const POINT pt = Common::ScaledPoint(X, Y);
    AddTextDirect(text, pt.x, pt.y, fontColor);
}

void Font::AddTextDirect(const std::wstring& text,
                         const int screenX,
                         const int screenY)
{
    TextInfo textInfo;

    textInfo.m_rect.left = screenX;
    textInfo.m_rect.top = screenY;
    textInfo.m_rect.right = 0;
    textInfo.m_rect.bottom = 0;

    textInfo.m_text = text;
    textInfo.m_bCenter = false;
    textInfo.m_color = m_fontColor;

    m_textList.push_back(textInfo);
}

void Font::AddTextDirect(const std::wstring& text,
                         const int screenX,
                         const int screenY,
                         const UINT fontColor)
{
    TextInfo textInfo;

    textInfo.m_rect.left = screenX;
    textInfo.m_rect.top = screenY;
    textInfo.m_rect.right = 0;
    textInfo.m_rect.bottom = 0;

    textInfo.m_text = text;
    textInfo.m_bCenter = false;
    textInfo.m_color = fontColor;

    m_textList.push_back(textInfo);
}

void Font::AddTextCenter(const std::wstring& text,
                        const int X,
                        const int Y,
                        const int Width,
                        const int Height)
{
    const POINT pt = Common::ScaledPoint(X, Y);
    D3DXVECTOR2 size = Common::ScaledSize();
    const int screenWidth = (int)(Width * size.x);
    const int screenHeight = (int)(Height * size.y);
    AddTextCenterDirect(text, pt.x, pt.y, screenWidth, screenHeight);
}

void Font::AddTextCenter(const std::wstring& text,
                         const int X,
                         const int Y,
                         const int Width,
                         const int Height,
                         const UINT fontColor)
{
    const POINT pt = Common::ScaledPoint(X, Y);
    D3DXVECTOR2 size = Common::ScaledSize();
    const int screenWidth = (int)(Width * size.x);
    const int screenHeight = (int)(Height * size.y);
    AddTextCenterDirect(text, pt.x, pt.y, screenWidth, screenHeight, fontColor);
}

void Font::AddTextCenterDirect(const std::wstring& text,
                               const int screenX,
                               const int screenY,
                               const int screenWidth,
                               const int screenHeight)
{
    TextInfo textInfo;

    textInfo.m_rect.left = screenX;
    textInfo.m_rect.top = screenY;
    textInfo.m_rect.right = screenX + screenWidth;
    textInfo.m_rect.bottom = screenY + screenHeight;

    textInfo.m_text = text;
    textInfo.m_bCenter = true;
    textInfo.m_color = m_fontColor;

    m_textList.push_back(textInfo);
}

void Font::AddTextCenterDirect(const std::wstring& text,
                               const int screenX,
                               const int screenY,
                               const int screenWidth,
                               const int screenHeight,
                               const UINT fontColor)
{
    TextInfo textInfo;

    textInfo.m_rect.left = screenX;
    textInfo.m_rect.top = screenY;
    textInfo.m_rect.right = screenX + screenWidth;
    textInfo.m_rect.bottom = screenY + screenHeight;

    textInfo.m_text = text;
    textInfo.m_bCenter = true;
    textInfo.m_color = fontColor;

    m_textList.push_back(textInfo);
}

void Font::AddTextRight(const std::wstring& text,
                        const int X,
                        const int Y,
                        const int Width,
                        const int Height,
                        const UINT fontColor)
{
    const POINT pt = Common::ScaledPoint(X, Y);
    D3DXVECTOR2 size = Common::ScaledSize();
    const int screenWidth = (int)(Width * size.x);
    const int screenHeight = (int)(Height * size.y);
    AddTextRightDirect(text, pt.x, pt.y, screenWidth, screenHeight, fontColor);
}

void Font::AddTextRightDirect(const std::wstring& text,
                              const int screenX,
                              const int screenY,
                              const int screenWidth,
                              const int screenHeight,
                              const UINT fontColor)
{
    TextInfo textInfo;

    textInfo.m_rect.left = screenX;
    textInfo.m_rect.top = screenY;
    textInfo.m_rect.right = screenX + screenWidth;
    textInfo.m_rect.bottom = screenY + screenHeight;

    textInfo.m_text = text;
    textInfo.m_bRight = true;
    textInfo.m_color = fontColor;

    m_textList.push_back(textInfo);
}

void Font::Draw()
{
    for (auto& textInfo : m_textList)
    {
        if (textInfo.m_bCenter)
        {
            // DrawTextの戻り値は文字数である。
            // そのため、hResultの中身が整数でもエラーが起きているわけではない。
            HRESULT hResult = m_pFont->DrawText(NULL,
                                                textInfo.m_text.c_str(),

                                                // -1 = 長さ自動
                                                -1,

                                                &textInfo.m_rect,
                                                DT_CENTER | DT_VCENTER | DT_NOCLIP,
                                                textInfo.m_color);

            assert(hResult >= 0);
        }
        else if (textInfo.m_bRight)
        {
            HRESULT hResult = m_pFont->DrawText(NULL,
                                                textInfo.m_text.c_str(),
                                                -1,
                                                &textInfo.m_rect,
                                                DT_RIGHT | DT_NOCLIP,
                                                textInfo.m_color);

            assert(hResult >= 0);
        }
        else
        {
            // DrawTextの戻り値は文字数である。
            // そのため、hResultの中身が整数でもエラーが起きているわけではない。
            HRESULT hResult = m_pFont->DrawText(NULL,
                                                textInfo.m_text.c_str(),

                                                // -1 = 長さ自動
                                                -1,

                                                &textInfo.m_rect,
                                                DT_LEFT | DT_NOCLIP,
                                                textInfo.m_color);

            assert(hResult >= 0);
        }

    }

    m_textList.clear();
}

SIZE Font::GetTextSize(const std::wstring& text)
{
    SIZE size = { 0, 0 };
    if (m_pFont == NULL)
    {
        return size;
    }

    RECT rect = { 0, 0, 0, 0 };
    m_pFont->DrawText(NULL, text.c_str(), -1, &rect, DT_CALCRECT | DT_LEFT | DT_NOCLIP, 0);
    size.cx = rect.right - rect.left;
    size.cy = rect.bottom - rect.top;
    return size;
}

void Font::Finalize()
{
    SAFE_RELEASE(m_pFont);
    Common::RemoveDeviceLostResource(this);
}

void Font::OnDeviceLost()
{
    SAFE_RELEASE(m_pFont);
}

void Font::OnDeviceReset()
{
    CreateResource();
}

}

