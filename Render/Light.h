#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#include <vector>
#include <deque>

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

    // 平行光源がある方向
    // DirとNormalでは方向が逆なので誤用に注意
    static D3DXVECTOR4 GetLightDir();
    static void SetLightDir(const D3DXVECTOR4& normal);

    static D3DXCOLOR GetLightColor();

    static void SetLightColor(const D3DXCOLOR& color);

    static float GetBrightness();
    static void SetBrightness(const float brightness);
    
    // ひとまず10個くらいまで
    static void AddPointLight(const D3DXVECTOR3& pos,
                              const D3DXCOLOR& color,
                              const float brightness);
    static bool RemovePointLight(size_t index);

    static std::deque<PointLightInfo> GetPointLightList();

private:

    // 平行光源のある方向
    static D3DXVECTOR4 m_lightDir;
    static float m_Brightness;
    static D3DXCOLOR m_color;

    static std::deque<PointLightInfo> m_pointLightList;
};
}

