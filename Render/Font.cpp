#include "Font.h"

namespace NSRender
{

void Font::Initialize(const std::wstring& fontName,
                      const int fontSize,
                      const UINT fontColor)
{
    HRESULT hResult = E_FAIL;

    D3DXFONT_DESC desc { };
    desc.Height             = fontSize;

    // 幅は必ず0(自動)を指定する
    desc.Width              = 0;

    desc.Weight             = FW_HEAVY;
    desc.MipLevels          = 1;
    desc.Italic             = FALSE;
    desc.CharSet            = DEFAULT_CHARSET;
    desc.OutputPrecision    = OUT_TT_ONLY_PRECIS;
    desc.Quality            = CLEARTYPE_NATURAL_QUALITY;
    desc.PitchAndFamily     = FF_DONTCARE;
    wcsncpy_s(desc.FaceName, _countof(desc.FaceName), fontName.c_str(), _TRUNCATE);

    hResult = D3DXCreateFontIndirect(Common::D3DDevice(),
                                     &desc,
                                     &m_pFont);

    assert(hResult == S_OK);

    m_fontColor = fontColor;

    // デバイスロストを管理する機能にこのリソースを登録
    Common::AddDeviceLostResource(m_pFont);
}

void Font::AddText(const std::wstring& text,
                      const int X,
                      const int Y)
{
    TextInfo textInfo;

    textInfo.m_rect.left = X;
    textInfo.m_rect.top = Y;
    textInfo.m_rect.right = 0;
    textInfo.m_rect.bottom = 0;

    textInfo.m_text = text;
    textInfo.m_bCenter = false;

    textInfo.m_color = m_fontColor;

    m_textList.push_back(textInfo);
}

void Font::AddText(const std::wstring& text,
                       const int X,
                       const int Y,
                       const UINT fontColor)
{
    TextInfo textInfo;

    textInfo.m_rect.left = X;
    textInfo.m_rect.top = Y;
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
    TextInfo textInfo;

    textInfo.m_rect.left = X;
    textInfo.m_rect.top = Y;
    textInfo.m_rect.right = X + Width;
    textInfo.m_rect.bottom = Y + Height;

    textInfo.m_text = text;
    textInfo.m_bCenter = true;

    textInfo.m_color = m_fontColor;

    m_textList.push_back(textInfo);
}

void Font::AddTextCenter(const std::wstring& text,
                         const int X,
                         const int Y,
                         const int Width,
                         const int Height,
                         const UINT fontColor)
{
    TextInfo textInfo;

    textInfo.m_rect.left = X;
    textInfo.m_rect.top = Y;
    textInfo.m_rect.right = X + Width;
    textInfo.m_rect.bottom = Y + Height;

    textInfo.m_text = text;
    textInfo.m_bCenter = true;

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
                                                DT_CENTER | DT_NOCLIP,
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

void Font::Finalize()
{
    SAFE_RELEASE(m_pFont);
}

void Font::OnDeviceLost()
{
    m_pFont->OnLostDevice();
}

void Font::OnDeviceReset()
{
    m_pFont->OnResetDevice();
}

}

