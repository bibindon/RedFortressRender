#include "Common.h"

LPDIRECT3D9 NSRender::Common::m_pD3D = NULL;
LPDIRECT3DDEVICE9 NSRender::Common::m_pD3DDev = NULL;

void NSRender::Common::Initialize()
{

}

void NSRender::Common::Finalize()
{

}

LPDIRECT3DDEVICE9 NSRender::Common::D3DDevice()
{
    return m_pD3DDev;
}

void NSRender::Common::SetD3DDevice(LPDIRECT3DDEVICE9 arg)
{
    m_pD3DDev = arg;
}




