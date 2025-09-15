#include "Util.h"

#include <Windows.h>
#include <exception>

std::wstring Util::Utf8ToWstring(const std::string& utf8)
{
    if (utf8.empty())
    {
        return std::wstring();
    }

    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len == 0)
    {
        throw std::exception("UTF-8 to UTF-16 conversion failed.");
    }

    std::wstring result(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);

    return result;
}

std::string Util::WstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty())
    {
        return std::string();
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0)
    {
        throw std::exception("UTF-16 to UTF-8 conversion failed.");
    }

    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);

    return result;
}

