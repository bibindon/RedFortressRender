#include "Sprite.h"

namespace NSRender
{

void Sprite::Initialize()
{
    HRESULT hResult = E_FAIL;
    hResult = D3DXCreateSprite(Common::D3DDevice(), &m_pSprite);

    assert(hResult == S_OK);
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
    SpriteInfo spriteInfo;

    spriteInfo.m_rect.left = X;
    spriteInfo.m_rect.top = Y;
    spriteInfo.m_imageName = filename;
    spriteInfo.m_transparency = transparency;

    m_spriteInfoList.push_back(spriteInfo);
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
    m_pSprite->Begin(D3DXSPRITE_ALPHABLEND);

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

    m_spriteInfoList.clear();
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
