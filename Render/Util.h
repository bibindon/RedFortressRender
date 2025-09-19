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

};

}

