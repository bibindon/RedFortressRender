#pragma once

#include "Common.h"

namespace NSRender
{

//-------------------------------------------------------------------
// フォントサイズとフォント名を指定してインスタンスを生成。
// SetMsgで文字列を設定しDrawで描画。
// フォントサイズやフォント名は変更できない。
// 変更したい場合は別のインスタンスを作成する。
// SetMsg関数はDraw関数を呼ぶたびに毎回呼ぶこと
//-------------------------------------------------------------------
class Text
{
public:

    void Initialize(const std::wstring& fontName,
                    const int fontSize,
                    const UINT fontColor = D3DCOLOR_RGBA(255, 255, 255, 255));

    void SetMsgLeft(const std::wstring& msg, const int X, const int Y);

    void SetMsgCenter(const std::wstring& msg,
                      const int X,
                      const int Y,
                      const int Width,
                      const int Height);
    void Draw();
    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct Item
    {
        RECT m_rect { };
        std::wstring m_msg;
        bool m_bCenter = false;
    };

    LPD3DXFONT m_pFont = NULL;
    UINT m_fontColor = UINT_MAX;
    std::vector<Item> m_msgList;

};

}

