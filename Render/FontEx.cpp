#include "FontEx.h"

namespace NSRender
{

void FontEx::Initialize(const std::wstring& fontName,
                        const int fontSize,
                        const UINT fontColor)
{
    m_fontColor = fontColor;
    m_shadowFont.Initialize(fontName, fontSize, MakeBlurColor(fontColor));
    m_mainFont.Initialize(fontName, fontSize, fontColor);
    m_postEffectFont.Initialize();
}

void FontEx::AddText(const std::wstring& text,
                     const int X,
                     const int Y)
{
    AddText(text, X, Y, m_fontColor);
}

void FontEx::AddText(const std::wstring& text,
                     const int X,
                     const int Y,
                     const UINT fontColor)
{
    const int outlineOffset = 1;
    const UINT outlineColor = MakeOutlineColor(fontColor);

    m_shadowFont.AddText(text, X, Y, MakeBlurColor(fontColor));

    const POINT mainPos = Common::ScaledPoint(X, Y);
    m_mainFont.AddTextDirect(text, mainPos.x - outlineOffset, mainPos.y - outlineOffset, outlineColor);
    m_mainFont.AddTextDirect(text, mainPos.x + outlineOffset, mainPos.y + outlineOffset, outlineColor);
    m_mainFont.AddTextDirect(text, mainPos.x, mainPos.y, fontColor);
}

void FontEx::AddTextCenter(const std::wstring& text,
                           const int X,
                           const int Y,
                           const int Width,
                           const int Height)
{
    AddTextCenter(text, X, Y, Width, Height, m_fontColor);
}

void FontEx::AddTextCenter(const std::wstring& text,
                           const int X,
                           const int Y,
                           const int Width,
                           const int Height,
                           const UINT fontColor)
{
    const int outlineOffset = 1;
    const UINT outlineColor = MakeOutlineColor(fontColor);

    m_shadowFont.AddTextCenter(text, X, Y, Width, Height, MakeBlurColor(fontColor));

    const POINT mainPos = Common::ScaledPoint(X, Y);
    D3DXVECTOR2 size = Common::ScaledSize();
    const int screenWidth = (int)(Width * size.x);
    const int screenHeight = (int)(Height * size.y);
    m_mainFont.AddTextCenterDirect(text,
                                   mainPos.x - outlineOffset,
                                   mainPos.y - outlineOffset,
                                   screenWidth,
                                   screenHeight,
                                   outlineColor);
    m_mainFont.AddTextCenterDirect(text,
                                   mainPos.x + outlineOffset,
                                   mainPos.y + outlineOffset,
                                   screenWidth,
                                   screenHeight,
                                   outlineColor);
    m_mainFont.AddTextCenterDirect(text, mainPos.x, mainPos.y, screenWidth, screenHeight, fontColor);
}

void FontEx::Draw()
{
    m_postEffectFont.BeginShadowPass();
    m_shadowFont.Draw();
    m_postEffectFont.DrawBlurredShadow();
    m_mainFont.Draw();
}

void FontEx::Finalize()
{
    m_postEffectFont.Finalize();
    m_shadowFont.Finalize();
    m_mainFont.Finalize();
}

SIZE FontEx::GetTextSize(const std::wstring& text)
{
    return m_mainFont.GetTextSize(text);
}

void FontEx::SetGaussianSampleSize(const int sampleSize)
{
    m_postEffectFont.SetGaussianSampleSize(sampleSize);
}

UINT FontEx::MakeBlurColor(const UINT color)
{
    const UINT alpha = color & 0xFF000000;
    return D3DCOLOR_ARGB(alpha >> 24, 128, 128, 128);
}

UINT FontEx::MakeOutlineColor(const UINT color)
{
    const UINT alpha = color & 0xFF000000;
    return D3DCOLOR_ARGB(alpha >> 24, 128, 128, 128);
}

}
