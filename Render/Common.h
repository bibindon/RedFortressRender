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

class IDeviceResettable;

class Common
{
public:

    static void Initialize();
    static void Finalize();

    static LPDIRECT3DDEVICE9 D3DDevice();
    static void SetD3DDevice(LPDIRECT3DDEVICE9 arg);

    static constexpr float ANIMATION_SPEED = 1.0f / 60;

    static void OnDeviceLostAll();
    static void OnDeviceResetAll();

    static void AddDeviceLostResource(IDeviceResettable* res);

    static void RemoveDeviceLostResource(const IDeviceResettable* res);

    static int ScreenW();
    static void SetScreenW(const int W);

    static int ScreenH();
    static void SetScreenH(const int H);

private:

    static LPDIRECT3D9 m_pD3D;
    static LPDIRECT3DDEVICE9 m_pD3DDev;

    static std::vector<IDeviceResettable*> m_resourceList;

    static int m_screenW;
    static int m_screenH;
};

template <typename T>
inline void SAFE_RELEASE(T*& p)
{
    if (p == nullptr)
    {
        return;
    }

    p->Release();
    p = nullptr;
}

template <typename T>
inline void FORCE_RELEASE(T*& p)
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

// デバイスリセット用インターフェース
class IDeviceResettable
{
public :
    virtual void OnDeviceLost() = 0;
    virtual void OnDeviceReset() = 0;
};


// 要らない気がする

template<typename T>
using Ptr = std::shared_ptr<T>;

template<typename T>
using Vec = std::vector<T>;

template<typename T1, typename T2>
using Umap = std::unordered_map<T1, T2>;

using Wstr = std::wstring;

}

