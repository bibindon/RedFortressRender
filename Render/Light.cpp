#include "Light.h"
#include "Util.h"
#include <deque>

namespace NSRender
{

D3DXVECTOR4 Light::m_lightDir { 0.8f, 1.0f, 0.0f, 0.0f };
float Light::m_Brightness = 1.0f;
float Light::m_ambientBrightness = 1.0f;

std::deque<PointLightInfo> Light::m_pointLightList;
D3DXCOLOR Light::m_color = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);
D3DXCOLOR Light::m_ambientColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.f);

D3DXVECTOR4 Light::GetLightDir()
{
    return m_lightDir;
}

D3DXCOLOR Light::GetLightColor()
{
    return m_color;
}

void Light::SetLightColor(const D3DXCOLOR& color)
{
    m_color = color;
}

void Light::SetLightDir(const D3DXVECTOR4& normal)
{
    m_lightDir = normal;
}

float Light::GetBrightness()
{
    return m_Brightness;
}

void Light::SetBrightness(const float brightness)
{
    m_Brightness = brightness;
}

D3DXCOLOR Light::GetAmbientColor()
{
    return m_ambientColor;
}

void Light::SetAmbientColor(const D3DXCOLOR& color)
{
    m_ambientColor = color;
}

float Light::GetAmbientBrightness()
{
    return m_ambientBrightness;
}

void Light::SetAmbientBrightness(const float brightness)
{
    m_ambientBrightness = brightness;
}

void Light::AddPointLight(const D3DXVECTOR3& pos,
                          const D3DXCOLOR& color,
                          const float brightness,
                          const PointLightShape shape,
                          const float lineLength,
                          const float squareWidth,
                          const float squareHeight,
                          const D3DXVECTOR3& rotation,
                          const std::wstring& ownerTag)
{
    PointLightInfo pointLightInfo;

    pointLightInfo.m_pos = pos;
    pointLightInfo.m_color = color;
    pointLightInfo.m_brightness = brightness;
    pointLightInfo.m_shape = shape;
    pointLightInfo.m_lineLength = lineLength;
    pointLightInfo.m_squareWidth = squareWidth;
    pointLightInfo.m_squareHeight = squareHeight;
    pointLightInfo.m_rotation = rotation;
    pointLightInfo.m_ownerTag = ownerTag;

    m_pointLightList.push_back(pointLightInfo);

    if (m_pointLightList.size() > 16)
    {
        m_pointLightList.pop_front();
    }
}

bool Light::RemovePointLight(const size_t index)
{
    if (index >= m_pointLightList.size())
    {
        return false;
    }

    m_pointLightList.erase(m_pointLightList.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void Light::RemovePointLightsByOwnerTag(const std::wstring& ownerTag)
{
    if (ownerTag.empty())
    {
        return;
    }

    for (auto it = m_pointLightList.begin(); it != m_pointLightList.end();)
    {
        if (it->m_ownerTag == ownerTag)
        {
            it = m_pointLightList.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool Light::SetPointLightPositionByOwnerTag(const std::wstring& ownerTag,
                                            const D3DXVECTOR3& pos)
{
    if (ownerTag.empty())
    {
        return false;
    }

    bool updated = false;
    for (auto& pointLight : m_pointLightList)
    {
        if (pointLight.m_ownerTag == ownerTag)
        {
            pointLight.m_pos = pos;
            updated = true;
        }
    }
    return updated;
}

void Light::ClearPointLights()
{
    m_pointLightList.clear();
}

std::deque<PointLightInfo> Light::GetPointLightList()
{
    return m_pointLightList;
}

}

