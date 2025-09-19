#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <memory>

#include "Font.h"
#include "Sprite.h"
#include "Mesh.h"
#include "AnimMesh.h"
#include "SkinAnimMesh.h"

#if defined(_DEBUG)
#define NEW ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define NEW new
#endif

namespace NSRender
{

class Font;
class Sprite;
class Mesh;
class AnimMesh;
class SkinAnimMesh;

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

    static void AddDeviceLostResource(Font* res);
    static void AddDeviceLostResource(Sprite* res);
    static void AddDeviceLostResource(Mesh* res);
    static void AddDeviceLostResource(AnimMesh* res);
    static void AddDeviceLostResource(SkinAnimMesh* res);

    static void RemoveDeviceLostResource(const Font* res);
    static void RemoveDeviceLostResource(const Sprite* res);
    static void RemoveDeviceLostResource(const Mesh* res);
    static void RemoveDeviceLostResource(const AnimMesh* res);
    static void RemoveDeviceLostResource(const SkinAnimMesh* res);

private:

    static LPDIRECT3D9 m_pD3D;
    static LPDIRECT3DDEVICE9 m_pD3DDev;

    static std::vector<Font*> m_fontList;
    static std::vector<Sprite*> m_spriteList;
    static std::vector<Mesh*> m_meshList;
    static std::vector<AnimMesh*> m_animMeshList;
    static std::vector<SkinAnimMesh*> m_skinAnimMeshList;
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

