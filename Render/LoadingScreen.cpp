#include "LoadingScreen.h"

namespace NSRender
{
namespace
{
static const std::wstring kLoadingScreenBlackTextureKey = L"__loading_screen_black__";
}

void LoadingScreen::Start()
{
    m_visible = true;
    m_fadingOut = false;
    m_animationTime = 0.0f;
    m_fadeElapsed = 0.0f;
    m_fadeAlpha = 1.0f;
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

void LoadingScreen::Draw(Sprite& sprite, Font& font)
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

    if (m_fadingOut)
    {
        return;
    }

    const int textAlpha = GetTextAlpha255();
    if (textAlpha <= 0)
    {
        return;
    }

    font.AddTextCenter(L"Loading...",
                       0,
                       0,
                       Common::BASE_W,
                       Common::BASE_H,
                       D3DCOLOR_RGBA(255, 255, 255, textAlpha));
    font.Draw();
}

bool LoadingScreen::IsVisible() const
{
    return m_visible;
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

int LoadingScreen::GetTextAlpha255() const
{
    const float blinkCycleSeconds = 1.0f;
    float cycleTime = m_animationTime;
    while (cycleTime >= blinkCycleSeconds)
    {
        cycleTime -= blinkCycleSeconds;
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
