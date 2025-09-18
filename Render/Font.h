#pragma once

#include "Common.h"

namespace NSRender
{

//-------------------------------------------------------------------
// フォントサイズとフォント名を指定してインスタンスを生成。
// SetTextで文字列を設定しDrawで描画。
// SetTextを複数回実行してからDraw関数を実行した場合、
// 複数の文字列が表示される。
// フォントサイズやフォント名は変更できない。
// 変更したい場合は別のインスタンスを作成する。
// SetText関数はDraw関数を呼ぶたびに毎回呼ぶこと。
// フォントの色は最初に指定することもできるし、毎回指定することもできる。
// 毎回指定しない場合、最初に指定した色が使われる
//-------------------------------------------------------------------
class Font
{
public:

    void Initialize(const std::wstring& fontName,
                    const int fontSize,
                    const UINT fontColor);

    void AddText(const std::wstring& text,
                     const int X,
                     const int Y);

    void AddText(const std::wstring& text,
                     const int X,
                     const int Y,
                     const UINT fontColor);

    void AddTextCenter(const std::wstring& text,
                       const int X,
                       const int Y,
                       const int Width,
                       const int Height);

    void AddTextCenter(const std::wstring& text,
                       const int X,
                       const int Y,
                       const int Width,
                       const int Height,
                       const UINT fontColor);

    void Draw();
    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

    struct TextInfo
    {
        RECT m_rect { };
        std::wstring m_text;
        bool m_bCenter = false;
        UINT m_color = D3DCOLOR_RGBA(255, 255, 255, 255);
    };

private:

    LPD3DXFONT m_pFont = NULL;
    UINT m_fontColor = UINT_MAX;
    std::vector<TextInfo> m_textList;

};

}

