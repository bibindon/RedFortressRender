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
    spriteInfo.m_scaled = false;

    m_spriteInfoList.push_back(spriteInfo);
}

void Sprite::PlaceImage(const std::wstring& filename,
                         const int X,
                         const int Y,
                         const int width,
                         const int height,
                         const int transparency)
{
    SpriteInfo spriteInfo;

    spriteInfo.m_rect.left = X;
    spriteInfo.m_rect.top = Y;
    spriteInfo.m_rect.right = width;
    spriteInfo.m_rect.bottom = height;
    spriteInfo.m_imageName = filename;
    spriteInfo.m_transparency = transparency;
    spriteInfo.m_scaled = true;

    m_spriteInfoList.push_back(spriteInfo);
}

void Sprite::RegisterTexture(const std::wstring& key, LPDIRECT3DTEXTURE9 texture)
{
    if (m_textureMap.find(key) != m_textureMap.end())
    {
        return;
    }

    m_textureMap[key] = texture;
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

SIZE Sprite::GetImageSize(const std::wstring& filename)
{
    SIZE size = { 0, 0 };
    const auto found = m_textureMap.find(filename);
    if (found == m_textureMap.end())
    {
        return size;
    }

    D3DSURFACE_DESC desc;
    found->second->GetLevelDesc(0, &desc);
    size.cx = static_cast<LONG>(desc.Width);
    size.cy = static_cast<LONG>(desc.Height);
    return size;
}

void Sprite::Draw()
{
    D3DXMATRIX mScale;
    D3DXMatrixScaling(&mScale, Common::ScaledSize().x, Common::ScaledSize().y, 1.0f);
    m_pSprite->SetTransform(&mScale);

    m_pSprite->Begin(D3DXSPRITE_ALPHABLEND);

    Common::D3DDevice()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    Common::D3DDevice()->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    Common::D3DDevice()->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

    for (auto& elem : m_spriteInfoList)
    {
        D3DXVECTOR3 pos((float)elem.m_rect.left, (float)elem.m_rect.top, 0);

        if (elem.m_scaled)
        {
            const SIZE imageSize = GetImageSize(elem.m_imageName);
            if (imageSize.cx > 0 && imageSize.cy > 0)
            {
                const float scaleXVal = static_cast<float>(elem.m_rect.right) / static_cast<float>(imageSize.cx);
                const float scaleYVal = static_cast<float>(elem.m_rect.bottom) / static_cast<float>(imageSize.cy);

                D3DXMATRIX perImageScale;
                D3DXMatrixScaling(&perImageScale, scaleXVal, scaleYVal, 1.0f);
                perImageScale = perImageScale * mScale;

                m_pSprite->SetTransform(&perImageScale);

                D3DXVECTOR3 scaledPos(static_cast<float>(elem.m_rect.left) / scaleXVal,
                                      static_cast<float>(elem.m_rect.top) / scaleYVal,
                                      0.0f);

                m_pSprite->Draw(m_textureMap.at(elem.m_imageName),
                                NULL,
                                NULL,
                                &scaledPos,
                                D3DCOLOR_RGBA(255, 255, 255, elem.m_transparency));

                m_pSprite->SetTransform(&mScale);
            }
            else
            {
                m_pSprite->Draw(m_textureMap.at(elem.m_imageName),
                                NULL,
                                NULL,
                                &pos,
                                D3DCOLOR_RGBA(255, 255, 255, elem.m_transparency));
            }
        }
        else
        {
            m_pSprite->Draw(m_textureMap.at(elem.m_imageName),
                            NULL,
                            NULL,
                            &pos,
                            D3DCOLOR_RGBA(255, 255, 255, elem.m_transparency));
        }
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
