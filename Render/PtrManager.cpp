#include "PtrManager.h"

namespace NSRender
{

ComManager* ComManager::m_p = nullptr;

ComManager* ComManager::Get()
{
    if (m_p == nullptr)
    {
        m_p = new ComManager();
    }

    return m_p;
}

void ComManager::Destroy()
{
    if (m_p != nullptr)
    {
        delete m_p;
        m_p = nullptr;
    }
}

void ComManager::Set(LPUNKNOWN ptr,
                     const std::wstring& typename_,
                     const std::wstring& filename,
                     const std::wstring& line,
                     const std::wstring& id)
{
    ComInfo comInfo;
    comInfo.m_comPtr = ptr;
    comInfo.m_typename = typename_;
    comInfo.m_filename = filename;
    comInfo.m_line = line;
    comInfo.m_id = id;
    comInfo.m_refCnt = 1;

    m_comMap[ptr] = comInfo;
}

void ComManager::AddRef(LPUNKNOWN ptr,
                        const std::wstring& typename_,
                        const std::wstring& filename,
                        const std::wstring& line,
                        const std::wstring& id)
{
    if (m_comMap.count(ptr) == 0)
    {
        throw std::exception("illegal release.");
    }

    m_comMap[ptr].m_refCnt++;

    ComInfo comInfo;
    comInfo.m_comPtr = ptr;
    comInfo.m_typename = typename_;
    comInfo.m_filename = filename;
    comInfo.m_line = line;
    comInfo.m_id = id;
    comInfo.m_refCnt = m_comMap[ptr].m_refCnt;

    m_addRefListMap[ptr].push_back(comInfo);
}

void ComManager::Release(LPUNKNOWN ptr)
{
    if (m_comMap.count(ptr) == 0)
    {
        throw std::exception("illegal release.");
    }

    m_comMap[ptr].m_refCnt--;

    if (m_comMap[ptr].m_refCnt == 0)
    {
        ptr->Release();
    }

    if (m_addRefListMap.count(ptr) != 0)
    {
        m_addRefListMap[ptr].pop_back();
    }

}

void PtrManager::New(void* p,
                     const std::wstring& typename_,
                     const std::wstring& filename,
                     const std::wstring& line,
                     const std::wstring& id)
{
    PtrInfo info;
    info.m_ptr = p;
    info.m_typename = typename_;
    info.m_filename = filename;
    info.m_line = line;
    info.m_id = id;

    m_ptrMap[p] = info;
}

void PtrManager::Delete(void* p)
{

    if (m_ptrMap.count(p) == 0)
    {
        throw std::exception("illegal deletion.");
    }

    delete m_ptrMap[p].m_ptr;

}

}

