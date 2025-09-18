#include "Text.h"

namespace NSRender
{

void Text::Initialize(const std::wstring& fontName,
                      const int fontSize,
                      const UINT fontColor)
{
    HRESULT hResult = E_FAIL;

    D3DXFONT_DESC desc { };
    desc.Height             = fontSize;

    // ���͕K��0(����)���w�肷��
    desc.Width              = 0;

    desc.Weight             = FW_HEAVY;
    desc.MipLevels          = 1;
    desc.Italic             = FALSE;
    desc.CharSet            = DEFAULT_CHARSET;
    desc.OutputPrecision    = OUT_TT_ONLY_PRECIS;
    desc.Quality            = CLEARTYPE_NATURAL_QUALITY;
    desc.PitchAndFamily     = FF_DONTCARE;
    wcsncpy_s(desc.FaceName, _countof(desc.FaceName), fontName.c_str(), _TRUNCATE);

    hResult = D3DXCreateFontIndirect(Common::D3DDevice(),
                                     &desc,
                                     &m_pFont);

    assert(hResult == S_OK);

    m_fontColor = fontColor;
}

void Text::SetMsgLeft(const std::wstring& msg,
                      const int X,
                      const int Y)
{
    Item item;

    item.m_rect.left = X;
    item.m_rect.top = Y;
    item.m_rect.right = 0;
    item.m_rect.bottom = 0;

    item.m_msg = msg;
    item.m_bCenter = false;

    m_msgList.push_back(item);
}

void Text::SetMsgCenter(const std::wstring& msg,
                        const int X,
                        const int Y,
                        const int Width,
                        const int Height)
{
    Item item;

    item.m_rect.left = X;
    item.m_rect.top = Y;
    item.m_rect.right = X + Width;
    item.m_rect.bottom = Y + Height;

    item.m_msg = msg;
    item.m_bCenter = true;

    m_msgList.push_back(item);
}

void Text::Draw()
{
    for (auto& item : m_msgList)
    {
        if (item.m_bCenter)
        {
            // DrawText�̖߂�l�͕������ł���B
            // ���̂��߁AhResult�̒��g�������ł��G���[���N���Ă���킯�ł͂Ȃ��B
            HRESULT hResult = m_pFont->DrawText(NULL,
                                                item.m_msg.c_str(),

                                                // -1 = ��������
                                                -1,

                                                &item.m_rect,
                                                DT_CENTER | DT_NOCLIP,
                                                m_fontColor);

            assert(hResult >= 0);
        }
        else
        {
            // DrawText�̖߂�l�͕������ł���B
            // ���̂��߁AhResult�̒��g�������ł��G���[���N���Ă���킯�ł͂Ȃ��B
            HRESULT hResult = m_pFont->DrawText(NULL,
                                                item.m_msg.c_str(),

                                                // -1 = ��������
                                                -1,

                                                &item.m_rect,
                                                DT_LEFT | DT_NOCLIP,
                                                m_fontColor);

            assert(hResult >= 0);
        }

    }

    m_msgList.clear();
}

void Text::Finalize()
{
    SAFE_RELEASE(m_pFont);
}

void Text::OnDeviceLost()
{
    m_pFont->OnLostDevice();
}

void Text::OnDeviceReset()
{
    m_pFont->OnResetDevice();
}

}

