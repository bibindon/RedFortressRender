#pragma once

#include <d3d9.h>
#include <d3dx9.h>

namespace NSRender
{
class Light
{
public:

    static D3DXVECTOR4 GetLightNormal();
    static void SetLightNormal(const D3DXVECTOR4& normal);
    static float GetBrightness();
    static void SetBrightness(const float brightness);

private:

    static D3DXVECTOR4 m_lightNormal;
    static float m_Brightness;
};
}

