#pragma once

#include "Common.h"
#include "Font.h"
#include "Sprite.h"

namespace NSRender
{

class LoadingScreen
{
public:
    void Start();
    void End();
    void Update(float deltaSeconds);
    void Draw(Sprite& sprite, Font& loadingFont, Font& titleFont);
    bool IsVisible() const;
    void SetTitle(const std::wstring& title);
    void SetProgress(int progress);
    int GetProgress() const;
    void SetShowTitle(bool show);
    bool IsShowTitle() const;

private:
    void EnsureBlackTexture(Sprite& sprite);
    void EnsureWhitePointTexture(Sprite& sprite);
    void DrawWhitePoint(Sprite& sprite);
    std::wstring BuildDisplayTitle() const;
    int GetFadeAlpha255() const;
    int GetBlinkAlpha255(float phaseSeconds) const;

    bool m_visible = false;
    bool m_fadingOut = false;
    bool m_blackTextureRegistered = false;
    bool m_whitePointTextureRegistered = false;
    float m_animationTime = 0.0f;
    float m_fadeElapsed = 0.0f;
    float m_fadeDuration = 0.5f;
    float m_fadeAlpha = 0.0f;
    std::wstring m_title = L"ホシガール";
    int m_progress = 0;
    bool m_showTitle = true;
    LPDIRECT3DTEXTURE9 m_blackTexture = NULL;
    LPDIRECT3DTEXTURE9 m_whitePointTexture = NULL;
};

}
