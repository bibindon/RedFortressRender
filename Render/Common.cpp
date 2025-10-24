#include "Common.h"
#include "Util.h"

namespace NSRender
{

LPDIRECT3D9 Common::m_pD3D = NULL;
LPDIRECT3DDEVICE9 Common::m_pD3DDev = NULL;

std::vector<IDeviceResettable*> Common::m_resourceList;

int Common::m_screenW = 1600;
int Common::m_screenH = 900;

void Common::Initialize()
{

}

void Common::Finalize()
{
    m_resourceList.clear();
}

LPDIRECT3DDEVICE9 Common::D3DDevice()
{
    return m_pD3DDev;
}

void Common::SetD3DDevice(LPDIRECT3DDEVICE9 arg)
{
    m_pD3DDev = arg;
}

void Common::OnDeviceLostAll()
{
    for (auto& elem : m_resourceList)
    {
        elem->OnDeviceLost();
    }
}

void Common::OnDeviceResetAll()
{
    for (auto& elem : m_resourceList)
    {
        elem->OnDeviceReset();
    }
}

void Common::AddDeviceLostResource(IDeviceResettable* res)
{
    m_resourceList.push_back(res);
}

void Common::RemoveDeviceLostResource(const IDeviceResettable* res)
{
    Util::Remove(m_resourceList, res);
}

int Common::ScreenW()
{
    return m_screenW;
}

void Common::SetScreenW(const int W)
{
    m_screenW = W;
}

int Common::ScreenH()
{
    return m_screenH;
}

void Common::SetScreenH(const int H)
{
    m_screenH = H;
}

POINT Common::ScaledPoint(const POINT& pt)
{
    POINT pt2 { };

    float scaleW = (float)m_screenW / BASE_W;
    float scaleH = (float)m_screenH / BASE_H;

    pt2.x = pt.x * scaleW;
    pt2.y = pt.y * scaleH;

    return pt2;
}

POINT Common::ScaledPoint(const int x, const int y)
{
    POINT pt2 { };

    float scaleW = (float)m_screenW / BASE_W;
    float scaleH = (float)m_screenH / BASE_H;

    pt2.x = x * scaleW;
    pt2.y = y * scaleH;

    return pt2;
}

D3DXVECTOR2 Common::ScaledSize()
{
    float scaleW = (float)m_screenW / BASE_W;
    float scaleH = (float)m_screenH / BASE_H;
    return D3DXVECTOR2(scaleW, scaleH);
}

}


