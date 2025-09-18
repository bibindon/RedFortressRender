#pragma once
#include "Common.h"

namespace NSRender
{

//-------------------------------------------------------------------
// �X�v���C�g���R����Ďg���ƌ��d�ɂȂ�A�Ƃ�����肪����B
// �X�v���C�g�͕`�悷��Ƃ��Ƀe�N�X�`�����w�肷�邱�Ƃ��ł���̂�
// ��̃X�v���C�g�ő�R�̃e�N�X�`����\���ł���B
// ��肪�N����܂ł́A�X�v���C�g1���������g���悤�ɂ���
// ���t���[���AAddImage�֐������s���Ȃ��ƊG���\�����ꑱ���Ȃ�
// 
// TODO �𑜓x�ɂ���ēK�؂ȉ𑜓x�̉摜���\�������悤�ɂ�����
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

