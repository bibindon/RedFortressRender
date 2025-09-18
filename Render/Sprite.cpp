#include "Sprite.h"

namespace NSRender
{

void Sprite::Initialize()
{
}

void Sprite::Finalize()
{
    for (auto& elem : m_textureMap)
    {
        elem.second->Release();
    }
    
    m_textureMap.clear();
}

void Sprite::LoadImage_(const std::wstring& filename)
{
    if (m_textureMap.find(filename) != m_textureMap.end())
    {
        return;
    }
    
    HRESULT hResult = E_FAIL;
    LPDIRECT3DTEXTURE9 pTexture;
    hResult = D3DXCreateTextureFromFile(Common::D3DDevice(),
                                        filename.c_str(),
                                        &pTexture);

    m_textureMap[filename] = pTexture;
}

void Sprite::PlaceImage(const std::wstring& filename, const int X, const int Y, const int transparency)
{
}

void Sprite::RemoveImage(const std::wstring& filename)
{
    if (m_textureMap.find(filename) == m_textureMap.end())
    {
        return;
    }

    m_textureMap[filename]->Release();
    m_textureMap.erase(filename);
}

void Sprite::Draw()
{
    m_pSprite->Begin(0);

    for (auto& elem : m_spriteInfoList)
    {
        D3DXVECTOR3 pos((float)elem.m_rect.left, (float)elem.m_rect.top, 0);
        m_pSprite->Draw(m_textureMap.at(elem.m_imageName),
                        NULL,
                        NULL,
                        &pos,
                        0xFFFFFFFF);
    }

    m_pSprite->End();
}

void Sprite::OnDeviceLost()
{
    m_pSprite->OnLostDevice();
}

void Sprite::OnDeviceReset()
{
    m_pSprite->OnResetDevice();
}

}
