#include "Light.h"
#include "Util.h"
#include <deque>

namespace NSRender
{

D3DXVECTOR4 Light::m_lightDir { 1.0f, 1.0f, 0.0f, 0.0f };
float Light::m_Brightness = 1.0f;

std::deque<PointLightInfo> Light::m_pointLightList;
D3DXCOLOR Light::m_color = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);

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

void Light::AddPointLight(const D3DXVECTOR3& pos,
                          const D3DXCOLOR& color,
                          const float brightness)
{
    PointLightInfo pointLightInfo;

    pointLightInfo.m_pos = pos;
    pointLightInfo.m_color = color;
    pointLightInfo.m_brightness = brightness;

    m_pointLightList.push_back(pointLightInfo);

    if (m_pointLightList.size() > 10)
    {
        m_pointLightList.pop_front();
    }
}

std::deque<PointLightInfo> Light::GetPointLightList()
{
    return m_pointLightList;
}

}

