#pragma once
#include "Common.h"

namespace NSRender
{

//-------------------------------------------------------------------
// スプライトを沢山作って使うと激重になる、という問題がある。
// スプライトは描画するときにテクスチャを指定することができるので
// 一つのスプライトで沢山のテクスチャを表示できる。
// 問題が起きるまでは、スプライト1枚だけを使うようにする
// 毎フレーム、AddImage関数を実行しないと絵が表示され続けない
// 
// TODO 解像度によって適切な解像度の画像が表示されるようにしたい
//-------------------------------------------------------------------
class Sprite
{
public:
    void Initialize();
    void Finalize();

    void LoadImage_(const std::wstring& filename);

    void PlaceImage(const std::wstring& filename,
                    const int X,
                    const int Y,
                    const int transparency = 255);

    void RemoveImage(const std::wstring& filename);

    void Draw();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct SpriteInfo
    {
        RECT m_rect { };
        std::wstring m_imageName;
        int m_transparency = 255;
    };

    LPD3DXSPRITE m_pSprite = NULL;

    std::vector<SpriteInfo> m_spriteInfoList;

};
}

