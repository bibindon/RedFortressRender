#include "Light.h"
#include "Util.h"

namespace NSRender
{

D3DXVECTOR4 Light::m_lightNormal { 1.0f, 1.0f, 0.0f, 0.0f };
float Light::m_Brightness = 1.0f;

std::vector<PointLightInfo> Light::m_pointLightList;

D3DXVECTOR4 Light::GetLightNormal()
{
    return m_lightNormal;
}

void Light::SetLightNormal(const D3DXVECTOR4& normal)
{
    m_lightNormal = normal;
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
        m_pointLightList.erase(m_pointLightList.end());
    }
}

std::vector<PointLightInfo> Light::GetPointLightList()
{
    return m_pointLightList;
}

}

