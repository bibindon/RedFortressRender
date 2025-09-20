#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#include <vector>

namespace NSRender
{

struct PointLightInfo
{
    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.f, 0.f, 0.f);
    D3DXCOLOR m_color = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);

    float m_brightness = 1.f;
};

class Light
{
public:

    static D3DXVECTOR4 GetLightNormal();
    static void SetLightNormal(const D3DXVECTOR4& normal);
    static float GetBrightness();
    static void SetBrightness(const float brightness);
    
    // ひとまず10個くらいまで
    static void AddPointLight(const D3DXVECTOR3& pos,
                              const D3DXCOLOR& color,
                              const float brightness);

    static std::vector<PointLightInfo> GetPointLightList();

private:

    static D3DXVECTOR4 m_lightNormal;
    static float m_Brightness;

    static std::vector<PointLightInfo> m_pointLightList;
};
}

