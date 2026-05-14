#pragma once

#include "Common.h"
#include "Font.h"
#include "PostEffectFont.h"

namespace NSRender
{

class FontEx
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

    void SetGaussianSampleSize(const int sampleSize);

private:

    static UINT MakeBlurColor(const UINT color);

    Font m_shadowFont;
    Font m_mainFont;
    PostEffectFont m_postEffectFont;
    UINT m_fontColor = D3DCOLOR_RGBA(255, 255, 255, 255);
};

}
