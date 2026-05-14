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
    m_shadowFont.AddText(text, X, Y, MakeBlurColor(fontColor));
    m_mainFont.AddText(text, X, Y, fontColor);
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
    m_shadowFont.AddTextCenter(text, X, Y, Width, Height, MakeBlurColor(fontColor));
    m_mainFont.AddTextCenter(text, X, Y, Width, Height, fontColor);
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

void FontEx::SetGaussianSampleSize(const int sampleSize)
{
    m_postEffectFont.SetGaussianSampleSize(sampleSize);
}

UINT FontEx::MakeBlurColor(const UINT color)
{
    const UINT alpha = color & 0xFF000000;
    return D3DCOLOR_ARGB(alpha >> 24, 128, 128, 128);
}

}
