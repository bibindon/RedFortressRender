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
    void Draw(Sprite& sprite, Font& font);
    bool IsVisible() const;

private:
    void EnsureBlackTexture(Sprite& sprite);
    int GetFadeAlpha255() const;
    int GetTextAlpha255() const;

    bool m_visible = false;
    bool m_fadingOut = false;
    bool m_blackTextureRegistered = false;
    float m_animationTime = 0.0f;
    float m_fadeElapsed = 0.0f;
    float m_fadeDuration = 0.5f;
    float m_fadeAlpha = 0.0f;
    LPDIRECT3DTEXTURE9 m_blackTexture = NULL;
};

}
