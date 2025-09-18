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
// SetText関数はDraw関数を呼ぶたびに毎回呼ぶこと
//-------------------------------------------------------------------
class Font
{
public:

    void Initialize(const std::wstring& fontName,
                    const int fontSize,
                    const UINT fontColor);

    void AddTextLeft(const std::wstring& text, const int X, const int Y);

    void AddTextCenter(const std::wstring& text,
                       const int X,
                       const int Y,
                       const int Width,
                       const int Height);
    void Draw();
    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

    struct TextInfo
    {
        RECT m_rect { };
        std::wstring m_text;
        bool m_bCenter = false;
    };

private:

    LPD3DXFONT m_pFont = NULL;
    UINT m_fontColor = UINT_MAX;
    std::vector<TextInfo> m_textList;

};

}

