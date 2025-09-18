#pragma once

#include "Common.h"

namespace NSRender
{

//-------------------------------------------------------------------
// �t�H���g�T�C�Y�ƃt�H���g�����w�肵�ăC���X�^���X�𐶐��B
// SetMsg�ŕ������ݒ肵Draw�ŕ`��B
// �t�H���g�T�C�Y��t�H���g���͕ύX�ł��Ȃ��B
// �ύX�������ꍇ�͕ʂ̃C���X�^���X���쐬����B
// SetMsg�֐���Draw�֐����ĂԂ��тɖ���ĂԂ���
//-------------------------------------------------------------------
class Text
{
public:

    void Initialize(const std::wstring& fontName,
                    const int fontSize,
                    const UINT fontColor = D3DCOLOR_RGBA(255, 255, 255, 255));

    void SetMsgLeft(const std::wstring& msg, const int X, const int Y);

    void SetMsgCenter(const std::wstring& msg,
                      const int X,
                      const int Y,
                      const int Width,
                      const int Height);
    void Draw();
    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    struct Item
    {
        RECT m_rect { };
        std::wstring m_msg;
        bool m_bCenter = false;
    };

    LPD3DXFONT m_pFont = NULL;
    UINT m_fontColor = UINT_MAX;
    std::vector<Item> m_msgList;

};

}

