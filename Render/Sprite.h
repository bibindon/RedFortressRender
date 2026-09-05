#pragma once
#include "Common.h"

namespace NSRender
{

class Sprite : public IDeviceResettable
{
public:
    void Initialize();
    void Finalize();

    void LoadImage_(const std::wstring& filename);

    void PlaceImage(const std::wstring& filename,
                    const int X,
                    const int Y,
                    const int transparency = 255);

    void PlaceImage(const std::wstring& filename,
                    const int X,
                    const int Y,
                    const int width,
                    const int height,
                    const int transparency = 255,
                    const bool flipX = false);

    void PlaceImage(const std::wstring& filename,
                    const int X,
                    const int Y,
                    const int width,
                    const int height,
                    const RECT& sourceRect,
                    const int transparency = 255);

    void RegisterTexture(const std::wstring& key, LPDIRECT3DTEXTURE9 texture);

    void RemoveImage(const std::wstring& filename);

    SIZE GetImageSize(const std::wstring& filename);

    void Draw();

    bool HasImages() const { return !m_spriteInfoList.empty(); }

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct SpriteInfo
    {
        RECT m_rect = RECT { 0, 0, 0, 0 };
        RECT m_sourceRect = RECT { 0, 0, 0, 0 };
        std::wstring m_imageName;
        int m_transparency = 255;
        bool m_scaled = false;
        bool m_flipX = false;
        bool m_hasSourceRect = false;
    };

    LPD3DXSPRITE m_pSprite = NULL;

    std::vector<SpriteInfo> m_spriteInfoList;

    std::unordered_map<std::wstring, LPDIRECT3DTEXTURE9> m_textureMap;

};
}
