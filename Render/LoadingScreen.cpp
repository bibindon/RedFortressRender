#include "LoadingScreen.h"

#include <cmath>

namespace NSRender
{
namespace
{
static const std::wstring kLoadingScreenBlackTextureKey = L"__loading_screen_black__";
static const std::wstring kLoadingScreenWhitePointTextureKey = L"__loading_screen_white_point__";
}

void LoadingScreen::Start()
{
    m_visible = true;
    m_fadingOut = false;
    m_animationTime = 0.0f;
    m_fadeElapsed = 0.0f;
    m_fadeAlpha = 1.0f;
    m_progress = 0;
}

void LoadingScreen::End()
{
    if (!m_visible)
    {
        return;
    }

    m_fadingOut = true;
    m_fadeElapsed = 0.0f;
    m_fadeDuration = 0.5f;
}

void LoadingScreen::Update(const float deltaSeconds)
{
    if (!m_visible)
    {
        return;
    }

    float clampedDelta = deltaSeconds;
    if (clampedDelta < 0.0f)
    {
        clampedDelta = 0.0f;
    }

    m_animationTime += clampedDelta;

    if (!m_fadingOut)
    {
        return;
    }

    m_fadeElapsed += clampedDelta;
    if (m_fadeElapsed >= m_fadeDuration)
    {
        m_fadeAlpha = 0.0f;
        m_visible = false;
        m_fadingOut = false;
        return;
    }

    if (m_fadeDuration <= 0.0f)
    {
        m_fadeAlpha = 0.0f;
        m_visible = false;
        m_fadingOut = false;
        return;
    }

    const float t = m_fadeElapsed / m_fadeDuration;
    m_fadeAlpha = 1.0f - t;
    if (m_fadeAlpha < 0.0f)
    {
        m_fadeAlpha = 0.0f;
    }
}

void LoadingScreen::Draw(Sprite& sprite, Font& loadingFont, FontEx& titleFont)
{
    if (!m_visible)
    {
        return;
    }

    const int backgroundAlpha = GetFadeAlpha255();
    if (backgroundAlpha <= 0)
    {
        return;
    }

    EnsureBlackTexture(sprite);
    if (m_blackTexture != NULL)
    {
        sprite.PlaceImage(kLoadingScreenBlackTextureKey,
                          0,
                          0,
                          Common::BASE_W,
                          Common::BASE_H,
                          backgroundAlpha);
        sprite.Draw();
    }

    if (m_showTitle)
    {
        const std::wstring displayTitle = BuildDisplayTitle();
        titleFont.AddTextCenter(displayTitle,
                                0,
                                220,
                                Common::BASE_W,
                                100,
                                D3DCOLOR_RGBA(255, 255, 255, 255));
        titleFont.Draw();

        const std::size_t totalChars = m_title.size();
        if (totalChars > 0 && m_progress < 100)
        {
            const SIZE titleSize = titleFont.GetTextSize(displayTitle);
            int visibleCount = m_progress * static_cast<int>(totalChars) / 100 + 1;
            if (visibleCount > static_cast<int>(totalChars))
            {
                visibleCount = static_cast<int>(totalChars);
            }

            const float visibleRatio = static_cast<float>(visibleCount) / static_cast<float>(totalChars);
            const int titleLeft = (Common::BASE_W - titleSize.cx) / 2;
            const int maskLeft = titleLeft + static_cast<int>(titleSize.cx * visibleRatio);
            int maskWidth = Common::BASE_W - maskLeft;
            if (maskWidth < 0)
            {
                maskWidth = 0;
            }

            if (maskWidth > 0 && m_blackTexture != NULL)
            {
                sprite.PlaceImage(kLoadingScreenBlackTextureKey,
                                  maskLeft,
                                  220,
                                  maskWidth,
                                  100,
                                  backgroundAlpha);
                sprite.Draw();
            }
        }
    }

    const int textAlpha = GetBlinkAlpha255(0.0f);
    if (textAlpha > 0)
    {
        loadingFont.AddTextCenter(L"Loading...",
                                  0,
                                  100,
                                  Common::BASE_W,
                                  Common::BASE_H,
                                  D3DCOLOR_RGBA(255, 255, 255, textAlpha));
    }
    loadingFont.Draw();

    DrawWhitePoint(sprite);
}

bool LoadingScreen::IsVisible() const
{
    return m_visible;
}

void LoadingScreen::SetTitle(const std::wstring& title)
{
    m_title = title;
}

void LoadingScreen::SetProgress(const int progress)
{
    if (progress < 0)
    {
        m_progress = 0;
        return;
    }
    if (progress > 100)
    {
        m_progress = 100;
        return;
    }
    m_progress = progress;
}

int LoadingScreen::GetProgress() const
{
    return m_progress;
}

void LoadingScreen::SetShowTitle(const bool show)
{
    m_showTitle = show;
}

bool LoadingScreen::IsShowTitle() const
{
    return m_showTitle;
}

std::wstring LoadingScreen::BuildDisplayTitle() const
{
    std::wstring displayTitle;
    for (std::size_t i = 0; i < m_title.size(); ++i)
    {
        if (i > 0)
        {
            displayTitle += L"  ";
        }
        displayTitle += m_title[i];
    }
    return displayTitle;
}

void LoadingScreen::EnsureBlackTexture(Sprite& sprite)
{
    if (m_blackTextureRegistered)
    {
        return;
    }

    HRESULT hr = Common::D3DDevice()->CreateTexture(
        2,
        2,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &m_blackTexture,
        NULL);
    if (FAILED(hr) || m_blackTexture == NULL)
    {
        return;
    }

    D3DLOCKED_RECT lockedRect;
    hr = m_blackTexture->LockRect(0, &lockedRect, NULL, 0);
    if (SUCCEEDED(hr))
    {
        DWORD* pixels = static_cast<DWORD*>(lockedRect.pBits);
        for (int i = 0; i < 4; ++i)
        {
            pixels[i] = D3DCOLOR_ARGB(255, 0, 0, 0);
        }
        m_blackTexture->UnlockRect(0);
    }

    sprite.RegisterTexture(kLoadingScreenBlackTextureKey, m_blackTexture);
    m_blackTextureRegistered = true;
}

void LoadingScreen::EnsureWhitePointTexture(Sprite& sprite)
{
    if (m_whitePointTextureRegistered)
    {
        return;
    }

    HRESULT hr = Common::D3DDevice()->CreateTexture(
        16,
        16,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &m_whitePointTexture,
        NULL);
    if (FAILED(hr) || m_whitePointTexture == NULL)
    {
        return;
    }

    D3DLOCKED_RECT lockedRect;
    hr = m_whitePointTexture->LockRect(0, &lockedRect, NULL, 0);
    if (SUCCEEDED(hr))
    {
        const float center = 7.5f;
        const float coreRadius = 0.50f;
        const float blurSigma = 2.0f;
        const float coreRadiusSquared = coreRadius * coreRadius;
        const float blurScale = 2.0f * blurSigma * blurSigma;
        for (int y = 0; y < 16; ++y)
        {
            BYTE* row = static_cast<BYTE*>(lockedRect.pBits) + lockedRect.Pitch * y;
            DWORD* pixels = reinterpret_cast<DWORD*>(row);
            for (int x = 0; x < 16; ++x)
            {
                const float dx = static_cast<float>(x) - center;
                const float dy = static_cast<float>(y) - center;
                const float distanceSquared = dx * dx + dy * dy;
                int alpha = 0;
                if (distanceSquared <= coreRadiusSquared)
                {
                    alpha = 255;
                }
                else
                {
                    const float distance = std::sqrt(distanceSquared);
                    const float blurDistance = distance - coreRadius;
                    const float gaussian = std::exp(-(blurDistance * blurDistance) / blurScale);
                    alpha = static_cast<int>(gaussian * 255.0f);
                    if (alpha < 0)
                    {
                        alpha = 0;
                    }
                    if (alpha > 255)
                    {
                        alpha = 255;
                    }
                }
                DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);
                pixels[x] = color;
            }
        }
        m_whitePointTexture->UnlockRect(0);
    }

    sprite.RegisterTexture(kLoadingScreenWhitePointTextureKey, m_whitePointTexture);
    m_whitePointTextureRegistered = true;
}

void LoadingScreen::DrawWhitePoint(Sprite& sprite)
{
    EnsureWhitePointTexture(sprite);
    if (m_whitePointTexture == NULL)
    {
        return;
    }

    const int alpha = GetBlinkAlpha255(1.33f);
    const float orbitCycleSeconds = 5.4f;
    const float radians = (m_animationTime / orbitCycleSeconds) * D3DX_PI * 2.0f;
    const float radiusX = 130.0f;
    const float radiusY = 60.0f;
    const float centerX = static_cast<float>(Common::BASE_W) * 0.5f;
    const float centerY = (static_cast<float>(Common::BASE_H) * 0.5f) + 100.0f;
    const int x = static_cast<int>(centerX + std::cos(radians) * radiusX) - 8;
    const int y = static_cast<int>(centerY + std::sin(radians) * radiusY) - 8;

    if (alpha > 0)
    {
        sprite.PlaceImage(kLoadingScreenWhitePointTextureKey, x, y, alpha);
    }

    const int alpha2 = GetBlinkAlpha255(2.66f);
    const float orbitCycleSeconds2 = 7.4f;
    const float radians2 = (m_animationTime / orbitCycleSeconds2) * D3DX_PI * 2.0f;
    const float radiusX2 = 80.0f;
    const float radiusY2 = 120.0f;
    const int x2 = static_cast<int>(centerX + std::cos(radians2) * radiusX2) - 12;
    const int y2 = static_cast<int>(centerY + std::sin(radians2) * radiusY2) - 12;

    if (alpha2 > 0)
    {
        sprite.PlaceImage(kLoadingScreenWhitePointTextureKey, x2, y2, 24, 24, alpha2);
    }
    sprite.Draw();
}

int LoadingScreen::GetFadeAlpha255() const
{
    float clamped = m_fadeAlpha;
    if (clamped < 0.0f)
    {
        clamped = 0.0f;
    }
    if (clamped > 1.0f)
    {
        clamped = 1.0f;
    }
    return static_cast<int>(clamped * 255.0f);
}

int LoadingScreen::GetBlinkAlpha255(const float phaseSeconds) const
{
    const float blinkCycleSeconds = 4.0f;
    float cycleTime = m_animationTime + phaseSeconds;
    while (cycleTime >= blinkCycleSeconds)
    {
        cycleTime -= blinkCycleSeconds;
    }
    while (cycleTime < 0.0f)
    {
        cycleTime += blinkCycleSeconds;
    }

    float blinkAlpha = 0.0f;
    const float halfCycleSeconds = blinkCycleSeconds * 0.5f;
    if (cycleTime < halfCycleSeconds)
    {
        blinkAlpha = cycleTime / halfCycleSeconds;
    }
    else
    {
        blinkAlpha = 1.0f - ((cycleTime - halfCycleSeconds) / halfCycleSeconds);
    }

    if (blinkAlpha < 0.0f)
    {
        blinkAlpha = 0.0f;
    }
    if (blinkAlpha > 1.0f)
    {
        blinkAlpha = 1.0f;
    }

    return static_cast<int>(blinkAlpha * 255.0f);
}

}
