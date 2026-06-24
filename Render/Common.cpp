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
    const size_t count = m_resourceList.size();
    wchar_t buf[512];
    swprintf_s(buf, L"[OnDeviceLostAll] count=%zu\n", count);
    OutputDebugStringW(buf);

    for (size_t i = 0; i < count; ++i)
    {
        IDeviceResettable* elem = m_resourceList[i];
        swprintf_s(buf, L"[OnDeviceLostAll] [%zu/%zu] %hs\n", i, count, typeid(*elem).name());
        OutputDebugStringW(buf);
        elem->OnDeviceLost();
        OutputDebugStringW(L"[OnDeviceLostAll]   -> done\n");
    }

    OutputDebugStringW(L"[OnDeviceLostAll] completed\n");
}

void Common::OnDeviceResetAll()
{
    const size_t count = m_resourceList.size();
    wchar_t buf[512];
    swprintf_s(buf, L"[OnDeviceResetAll] count=%zu\n", count);
    OutputDebugStringW(buf);

    for (size_t i = 0; i < count; ++i)
    {
        IDeviceResettable* elem = m_resourceList[i];
        swprintf_s(buf, L"[OnDeviceResetAll] [%zu/%zu] %hs\n", i, count, typeid(*elem).name());
        OutputDebugStringW(buf);
        elem->OnDeviceReset();
        OutputDebugStringW(L"[OnDeviceResetAll]   -> done\n");
    }

    OutputDebugStringW(L"[OnDeviceResetAll] completed\n");
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

    pt2.x = (LONG)(pt.x * scaleW);
    pt2.y = (LONG)(pt.y * scaleH);

    return pt2;
}

POINT Common::ScaledPoint(const int x, const int y)
{
    POINT pt2 { };

    float scaleW = (float)m_screenW / BASE_W;
    float scaleH = (float)m_screenH / BASE_H;

    pt2.x = (LONG)(x * scaleW);
    pt2.y = (LONG)(y * scaleH);

    return pt2;
}

D3DXVECTOR2 Common::ScaledSize()
{
    float scaleW = (float)m_screenW / BASE_W;
    float scaleH = (float)m_screenH / BASE_H;
    return D3DXVECTOR2(scaleW, scaleH);
}

}


