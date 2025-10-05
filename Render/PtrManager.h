#pragma once

#include <d3dx9.h>

#include "Common.h"

namespace NSRender
{

// Comポインタの管理クラス
// 通常は使わない。よほど困ったときに詳細な情報を得るために使う想定

struct ComInfo
{
    LPUNKNOWN m_comPtr;

    // 用途未確定
    std::wstring m_id;

    std::wstring m_typename;

    std::wstring m_filename;

    std::wstring m_line;

    int m_refCnt = 0;
};

class ComManager
{
public:

    static ComManager* Get();
    static void Destroy();

    void Set(LPUNKNOWN ptr,
             const std::wstring& typename_,
             const std::wstring& filename,
             const std::wstring& line,
             const std::wstring& id = L"");

    void AddRef(LPUNKNOWN ptr,
                const std::wstring& typename_,
                const std::wstring& filename,
                const std::wstring& line,
                const std::wstring& id = L"");

    void Release(LPUNKNOWN ptr);

private:

    std::unordered_map<LPUNKNOWN, ComInfo> m_comMap;
    std::unordered_map<LPUNKNOWN, std::vector<ComInfo>> m_addRefListMap;

    static ComManager* m_p;

};

// 使う予定なし。よほど難しい問題に直面したときに使用を検討する
struct PtrInfo
{
    void* m_ptr;

    // 用途未確定
    std::wstring m_id;

    std::wstring m_typename;

    std::wstring m_filename;

    std::wstring m_line;
};

class PtrManager
{
public:

    void New(void* p,
             const std::wstring& typename_,
             const std::wstring& filename,
             const std::wstring& line,
             const std::wstring& id = L"");

    void Delete(void* p);

private:

    std::unordered_map<void*, PtrInfo> m_ptrMap;

};
}

