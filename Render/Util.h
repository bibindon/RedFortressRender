#pragma once

#include <string>

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

namespace NSRender
{

class Util
{
public:

    static std::wstring Utf8ToWstring(const std::string& utf8);
    static std::string WstringToUtf8(const std::wstring& wstr);

    // �l��v�ō폜�Fc ���� value �����ׂĎ�菜���i�߂�l�Ȃ��j
    template <class Seq, class T>
    static void Remove(Seq& c, const T& value)
    {
        c.erase(std::remove(std::begin(c), std::end(c), value), std::end(c));
    }

    // �����ō폜�Fpred �� true �̗v�f����菜���i�߂�l�Ȃ��j
    template <class Seq, class Pred>
    static void RemoveIf(Seq& c, Pred pred)
    {
        c.erase(std::remove_if(std::begin(c), std::end(c), std::move(pred)), std::end(c));
    }

};

}

