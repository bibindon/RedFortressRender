#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <memory>

#if defined(_DEBUG)
#define NEW ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define NEW new
#endif

namespace NSRender
{

class Common
{
public:

    static void Initialize();
    static void Finalize();

    static LPDIRECT3DDEVICE9 D3DDevice();
    static void SetD3DDevice(LPDIRECT3DDEVICE9 arg);

    static constexpr float ANIMATION_SPEED { 1.0f / 60 };

    static void OnDeviceLostAll();
    static void OnDeviceResetAll();

    static void AddDeviceLostResource(const LPD3DXFONT font);
    static void AddDeviceLostResource(const LPD3DXSPRITE sprite);
    static void AddDeviceLostResource(const LPD3DXEFFECT effect);

private:

    static LPDIRECT3D9 m_pD3D;
    static LPDIRECT3DDEVICE9 m_pD3DDev;

    static std::vector<LPD3DXFONT> m_fontList;
    static std::vector<LPD3DXSPRITE> m_spriteList;
    static std::vector<LPD3DXEFFECT> m_effectList;
};

template <typename T>
inline void SAFE_RELEASE(T*& p)
{
    if (p == nullptr)
    {
        return;
    }

    while (true)
    {
        auto refCnt = p->Release();
        if (refCnt == 0)
        {
            break;
        }
    }
    p = nullptr;
}

template <typename T>
inline void SAFE_DELETE(T*& p)
{
    delete p;
    p = nullptr;
}

template <typename T>
inline void SAFE_DELETE_ARRAY(T*& p)
{
    delete[] p;
    p = nullptr;
}

template<typename T>
using Ptr = std::shared_ptr<T>;

template<typename T>
using Vec = std::vector<T>;

template<typename T1, typename T2>
using Umap = std::unordered_map<T1, T2>;

using Wstr = std::wstring;

}

