#include "Common.h"
#include "Util.h"

namespace NSRender
{

LPDIRECT3D9 Common::m_pD3D = NULL;
LPDIRECT3DDEVICE9 Common::m_pD3DDev = NULL;

std::vector<Font*> Common::m_fontList;
std::vector<Sprite*> Common::m_spriteList;
std::vector<Mesh*> Common::m_meshList;
std::vector<AnimMesh*> Common::m_animMeshList;
std::vector<SkinAnimMesh*> Common::m_skinAnimMeshList;
int Common::m_screenW = 1600;
int Common::m_screenH = 900;

void Common::Initialize()
{

}

void Common::Finalize()
{
    m_fontList.clear();
    m_spriteList.clear();
    m_meshList.clear();
    m_animMeshList.clear();
    m_skinAnimMeshList.clear();
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
        elem->OnDeviceLost();
    }

    for (auto& elem : m_spriteList)
    {
        elem->OnDeviceLost();
    }

    for (auto& elem : m_meshList)
    {
        elem->OnDeviceLost();
    }

    for (auto& elem : m_animMeshList)
    {
        elem->OnDeviceLost();
    }

    for (auto& elem : m_skinAnimMeshList)
    {
        elem->OnDeviceLost();
    }
}

void Common::OnDeviceResetAll()
{
    for (auto& elem : m_fontList)
    {
        elem->OnDeviceReset();
    }

    for (auto& elem : m_spriteList)
    {
        elem->OnDeviceReset();
    }

    for (auto& elem : m_meshList)
    {
        elem->OnDeviceReset();
    }

    for (auto& elem : m_animMeshList)
    {
        elem->OnDeviceReset();
    }

    for (auto& elem : m_skinAnimMeshList)
    {
        elem->OnDeviceReset();
    }
}

void Common::AddDeviceLostResource(Font* font)
{
    m_fontList.push_back(font);
}

void Common::AddDeviceLostResource(Sprite* sprite)
{
    m_spriteList.push_back(sprite);
}

void Common::AddDeviceLostResource(Mesh* res)
{
    m_meshList.push_back(res);
}

void Common::AddDeviceLostResource(AnimMesh* res)
{
    m_animMeshList.push_back(res);
}

void Common::AddDeviceLostResource(SkinAnimMesh* res)
{
    m_skinAnimMeshList.push_back(res);
}

void Common::RemoveDeviceLostResource(const Font* res)
{
    Util::Remove(m_fontList, res);
}

void Common::RemoveDeviceLostResource(const Sprite* res)
{
    Util::Remove(m_spriteList, res);
}

void Common::RemoveDeviceLostResource(const Mesh* res)
{
    Util::Remove(m_meshList, res);
}

void Common::RemoveDeviceLostResource(const AnimMesh* res)
{
    Util::Remove(m_animMeshList, res);
}

void Common::RemoveDeviceLostResource(const SkinAnimMesh* res)
{
    Util::Remove(m_skinAnimMeshList, res);
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

}


