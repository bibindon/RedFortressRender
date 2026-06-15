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
                         const int transparency,
                         const bool flipX)
{
    SpriteInfo spriteInfo;

    spriteInfo.m_rect.left = X;
    spriteInfo.m_rect.top = Y;
    spriteInfo.m_rect.right = width;
    spriteInfo.m_rect.bottom = height;
    spriteInfo.m_imageName = filename;
    spriteInfo.m_transparency = transparency;
    spriteInfo.m_scaled = true;
    spriteInfo.m_flipX = flipX;

    m_spriteInfoList.push_back(spriteInfo);
}

void Sprite::PlaceImage(const std::wstring& filename,
                         const int X,
                         const int Y,
                         const int width,
                         const int height,
                         const RECT& sourceRect,
                         const int transparency)
{
    SpriteInfo spriteInfo;

    spriteInfo.m_rect.left = X;
    spriteInfo.m_rect.top = Y;
    spriteInfo.m_rect.right = width;
    spriteInfo.m_rect.bottom = height;
    spriteInfo.m_sourceRect = sourceRect;
    spriteInfo.m_imageName = filename;
    spriteInfo.m_transparency = transparency;
    spriteInfo.m_scaled = true;
    spriteInfo.m_hasSourceRect = true;

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
                int sourceWidth = imageSize.cx;
                int sourceHeight = imageSize.cy;
                RECT* sourceRect = NULL;
                if (elem.m_hasSourceRect)
                {
                    sourceWidth = elem.m_sourceRect.right - elem.m_sourceRect.left;
                    sourceHeight = elem.m_sourceRect.bottom - elem.m_sourceRect.top;
                    sourceRect = &elem.m_sourceRect;
                }

                if (sourceWidth <= 0 || sourceHeight <= 0)
                {
                    continue;
                }

                const float scaleXVal = static_cast<float>(elem.m_rect.right) / static_cast<float>(sourceWidth);
                const float scaleYVal = static_cast<float>(elem.m_rect.bottom) / static_cast<float>(sourceHeight);
                float workScaleXVal = scaleXVal;
                if (elem.m_flipX)
                {
                    workScaleXVal *= -1.0f;
                }

                D3DXMATRIX perImageScale;
                D3DXMatrixScaling(&perImageScale, workScaleXVal, scaleYVal, 1.0f);
                perImageScale = perImageScale * mScale;

                m_pSprite->SetTransform(&perImageScale);

                D3DXVECTOR3 scaledPos(0.0f, 0.0f, 0.0f);
                if (elem.m_flipX)
                {
                    scaledPos.x = static_cast<float>(elem.m_rect.left + elem.m_rect.right) / workScaleXVal;
                }
                else
                {
                    scaledPos.x = static_cast<float>(elem.m_rect.left) / workScaleXVal;
                }
                scaledPos.y = static_cast<float>(elem.m_rect.top) / scaleYVal;

                m_pSprite->Draw(m_textureMap.at(elem.m_imageName),
                                sourceRect,
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
