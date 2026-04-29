#pragma once

#include <string>

// #define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

namespace NSRender
{

class Util
{
public:

    static std::wstring Utf8ToWstring(const std::string& utf8);
    static std::string WstringToUtf8(const std::wstring& wstr);

    static std::wstring GetExeDir();

    // 値一致で削除：c から value をすべて取り除く（戻り値なし）
    template <class Seq, class T>
    static void Remove(Seq& c, const T& value)
    {
        c.erase(std::remove(std::begin(c), std::end(c), value), std::end(c));
    }

    // 条件で削除：pred が true の要素を取り除く（戻り値なし）
    template <class Seq, class Pred>
    static void RemoveIf(Seq& c, Pred pred)
    {
        c.erase(std::remove_if(std::begin(c), std::end(c), std::move(pred)), std::end(c));
    }

    // 含有チェック（値一致）：要素が含まれていれば true
    template <class Seq, class T>
    static bool Contain(const Seq& container, const T& value)
    {
        auto foundIter = std::find(std::begin(container), std::end(container), value);
        if (foundIter != std::end(container))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // 含有チェック（条件）：述語が true を返す要素があれば true
    template <class Seq, class Pred>
    static bool ContainIf(const Seq& container, Pred predicate)
    {
        auto foundIter = std::find_if(std::begin(container), std::end(container), predicate);
        if (foundIter != std::end(container))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

}

