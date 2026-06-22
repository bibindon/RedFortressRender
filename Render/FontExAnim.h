#pragma once

#include "FontEx.h"

namespace NSRender
{

class FontExAnim
{
public:

    void Initialize(const std::wstring& fontName,
                    const int fontSize,
                    const UINT fontColor);
    void Finalize();

    void Start(const std::wstring& text,
               const int X,
               const int Y,
               const int framesPerCharacter);
    void Start(const std::wstring& text,
               const int X,
               const int Y,
               const int framesPerCharacter,
               const UINT fontColor);

    void Update();
    void Draw();
    void Finish();

    bool IsFinished() const;
    void SetGaussianSampleSize(const int sampleSize);

private:

    size_t GetNextVisibleLength() const;

    FontEx m_font;
    std::wstring m_text;
    int m_x = 0;
    int m_y = 0;
    int m_framesPerCharacter = 1;
    int m_elapsedFrames = 0;
    size_t m_visibleLength = 0;
    UINT m_fontColor = D3DCOLOR_RGBA(255, 255, 255, 255);
    bool m_isStarted = false;
};

}
