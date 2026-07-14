#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#include <vector>
#include <deque>
#include <string>

namespace NSRender
{

enum class PointLightShape : int
{
    Point = 0,
    Line = 1,
    Square = 2,
    Cube = 3,
    Sphere = 4
};

struct PointLightInfo
{
    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.f, 0.f, 0.f);
    D3DXCOLOR m_color = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);

    float m_brightness = 1.f;
    PointLightShape m_shape = PointLightShape::Point;
    float m_lineLength = 12.0f;
    float m_squareWidth = 10.0f;
    float m_squareHeight = 10.0f;
    D3DXVECTOR3 m_rotation = D3DXVECTOR3(0.f, 0.f, 0.f);
    float m_range = 12.0f;
    std::wstring m_ownerTag;
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

    static D3DXCOLOR GetAmbientColor();
    static void SetAmbientColor(const D3DXCOLOR& color);

    static float GetAmbientBrightness();
    static void SetAmbientBrightness(const float brightness);
    
    // ひとまず10個くらいまで
    static void AddPointLight(const D3DXVECTOR3& pos,
                              const D3DXCOLOR& color,
                              const float brightness,
                              const PointLightShape shape = PointLightShape::Point,
                              const float lineLength = 12.0f,
                              const float squareWidth = 10.0f,
                              const float squareHeight = 10.0f,
                              const D3DXVECTOR3& rotation = D3DXVECTOR3(0.f, 0.f, 0.f),
                              const float range = 12.0f,
                              const std::wstring& ownerTag = L"");
    static bool RemovePointLight(size_t index);
    static void RemovePointLightsByOwnerTag(const std::wstring& ownerTag);
    static bool SetPointLightPositionByOwnerTag(const std::wstring& ownerTag,
                                                const D3DXVECTOR3& pos);
    static void ClearPointLights();

    static std::deque<PointLightInfo> GetPointLightList();

private:

    // 平行光源のある方向
    static D3DXVECTOR4 m_lightDir;
    static float m_Brightness;
    static D3DXCOLOR m_color;
    static float m_ambientBrightness;
    static D3DXCOLOR m_ambientColor;

    static std::deque<PointLightInfo> m_pointLightList;
};
}

