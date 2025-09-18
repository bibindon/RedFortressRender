#pragma once

#include "Common.h"

namespace NSRender
{

//-------------------------------------------------------------------
// �t�H���g�T�C�Y�ƃt�H���g�����w�肵�ăC���X�^���X�𐶐��B
// SetText�ŕ������ݒ肵Draw�ŕ`��B
// SetText�𕡐�����s���Ă���Draw�֐������s�����ꍇ�A
// �����̕����񂪕\�������B
// �t�H���g�T�C�Y��t�H���g���͕ύX�ł��Ȃ��B
// �ύX�������ꍇ�͕ʂ̃C���X�^���X���쐬����B
// SetText�֐���Draw�֐����ĂԂ��тɖ���ĂԂ��ƁB
// �t�H���g�̐F�͍ŏ��Ɏw�肷�邱�Ƃ��ł��邵�A����w�肷�邱�Ƃ��ł���B
// ����w�肵�Ȃ��ꍇ�A�ŏ��Ɏw�肵���F���g����
//-------------------------------------------------------------------
class Font
{
public:

    void Initialize(const std::wstring& fontName,
                    const int fontSize,
                    const UINT fontColor);

    void AddText(const std::wstring& text,
                     const int X,
                     const int Y);

    void AddText(const std::wstring& text,
                     const int X,
                     const int Y,
                     const UINT fontColor);

    void AddTextCenter(const std::wstring& text,
                       const int X,
                       const int Y,
                       const int Width,
                       const int Height);

    void AddTextCenter(const std::wstring& text,
                       const int X,
                       const int Y,
                       const int Width,
                       const int Height,
                       const UINT fontColor);

    void Draw();
    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

    struct TextInfo
    {
        RECT m_rect { };
        std::wstring m_text;
        bool m_bCenter = false;
        UINT m_color = D3DCOLOR_RGBA(255, 255, 255, 255);
    };

private:

    LPD3DXFONT m_pFont = NULL;
    UINT m_fontColor = UINT_MAX;
    std::vector<TextInfo> m_textList;

};

}

