#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <cassert>

namespace NSRender
{

class Common
{
public:

    static void Initialize();
    static void Finalize();

    static LPDIRECT3DDEVICE9 D3DDevice();
    static void SetD3DDevice(LPDIRECT3DDEVICE9 arg);

private:

    static LPDIRECT3D9 m_pD3D;
    static LPDIRECT3DDEVICE9 m_pD3DDev;

};

}

