#include "Common.h"
#include "Util.h"

namespace NSRender
{

LPDIRECT3D9 Common::m_pD3D = NULL;
LPDIRECT3DDEVICE9 Common::m_pD3DDev = NULL;

std::vector<LPD3DXFONT> Common::m_fontList;
std::vector<LPD3DXSPRITE> Common::m_spriteList;
std::vector<LPD3DXEFFECT> Common::m_effectList;

void Common::Initialize()
{

}

void Common::Finalize()
{
    m_fontList.clear();
    m_spriteList.clear();
    m_effectList.clear();
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
    for (auto& elem : m_fontList)
    {
        elem->OnLostDevice();
    }

    for (auto& elem : m_spriteList)
    {
        elem->OnLostDevice();
    }

    for (auto& elem : m_effectList)
    {
        elem->OnLostDevice();
    }
}

void Common::OnDeviceResetAll()
{
    for (auto& elem : m_fontList)
    {
        elem->OnResetDevice();
    }

    for (auto& elem : m_spriteList)
    {
        elem->OnResetDevice();
    }

    for (auto& elem : m_effectList)
    {
        elem->OnResetDevice();
    }
}

void Common::AddDeviceLostResource(const LPD3DXFONT font)
{
    m_fontList.push_back(font);
}

void Common::AddDeviceLostResource(const LPD3DXSPRITE sprite)
{
    m_spriteList.push_back(sprite);
}

void Common::AddDeviceLostResource(const LPD3DXEFFECT effect)
{
    m_effectList.push_back(effect);
}

void Common::RemoveDeviceLostResource(const LPD3DXFONT font)
{
    Util::Remove(m_fontList, font);
}

void Common::RemoveDeviceLostResource(const LPD3DXSPRITE sprite)
{
    Util::Remove(m_spriteList, sprite);
}

void Common::RemoveDeviceLostResource(const LPD3DXEFFECT effect)
{
    Util::Remove(m_effectList, effect);
}

}


