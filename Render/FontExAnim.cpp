#include "FontExAnim.h"

namespace NSRender
{

void FontExAnim::Initialize(const std::wstring& fontName,
                            const int fontSize,
                            const UINT fontColor)
{
    m_fontColor = fontColor;
    m_font.Initialize(fontName, fontSize, fontColor);
}

void FontExAnim::Finalize()
{
    m_font.Finalize();
    m_text.clear();
    m_visibleLength = 0;
    m_isStarted = false;
}

void FontExAnim::Start(const std::wstring& text,
                       const int X,
                       const int Y,
                       const int framesPerCharacter)
{
    Start(text, X, Y, framesPerCharacter, m_fontColor);
}

void FontExAnim::Start(const std::wstring& text,
                       const int X,
                       const int Y,
                       const int framesPerCharacter,
                       const UINT fontColor)
{
    m_text = text;
    m_x = X;
    m_y = Y;
    m_framesPerCharacter = framesPerCharacter;
    if (m_framesPerCharacter < 1)
    {
        m_framesPerCharacter = 1;
    }
    m_elapsedFrames = 0;
    m_visibleLength = 0;
    m_fontColor = fontColor;
    m_isStarted = true;

    if (!m_text.empty())
    {
        m_visibleLength = GetNextVisibleLength();
    }
}

void FontExAnim::Update()
{
    if (!m_isStarted || IsFinished())
    {
        return;
    }

    ++m_elapsedFrames;
    if (m_elapsedFrames < m_framesPerCharacter)
    {
        return;
    }

    m_elapsedFrames = 0;
    m_visibleLength = GetNextVisibleLength();
}

void FontExAnim::Draw()
{
    if (!m_isStarted || m_visibleLength == 0)
    {
        return;
    }

    m_font.AddText(m_text.substr(0, m_visibleLength), m_x, m_y, m_fontColor);
    m_font.Draw();
}

void FontExAnim::Finish()
{
    if (!m_isStarted)
    {
        return;
    }

    m_visibleLength = m_text.length();
    m_elapsedFrames = 0;
}

bool FontExAnim::IsFinished() const
{
    if (!m_isStarted)
    {
        return true;
    }

    return m_visibleLength >= m_text.length();
}

void FontExAnim::SetGaussianSampleSize(const int sampleSize)
{
    m_font.SetGaussianSampleSize(sampleSize);
}

size_t FontExAnim::GetNextVisibleLength() const
{
    if (m_visibleLength >= m_text.length())
    {
        return m_text.length();
    }

    size_t nextLength = m_visibleLength + 1;
    const wchar_t firstCodeUnit = m_text.at(m_visibleLength);
    const bool isHighSurrogate = firstCodeUnit >= 0xD800 && firstCodeUnit <= 0xDBFF;
    if (isHighSurrogate && nextLength < m_text.length())
    {
        const wchar_t secondCodeUnit = m_text.at(nextLength);
        const bool isLowSurrogate = secondCodeUnit >= 0xDC00 && secondCodeUnit <= 0xDFFF;
        if (isLowSurrogate)
        {
            ++nextLength;
        }
    }

    return nextLength;
}

}
