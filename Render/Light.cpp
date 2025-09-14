#include "Light.h"

D3DXVECTOR4 NSRender::Light::m_lightNormal { 1.0f, 1.0f, 0.0f, 0.0f };
float NSRender::Light::m_Brightness = 1.0f;

D3DXVECTOR4 NSRender::Light::GetLightNormal()
{
    return m_lightNormal;
}

void NSRender::Light::SetLightNormal(const D3DXVECTOR4& normal)
{
    m_lightNormal = normal;
}

float NSRender::Light::GetBrightness()
{
    return m_Brightness;
}

void NSRender::Light::SetBrightness(const float brightness)
{
    m_Brightness = brightness;
}
