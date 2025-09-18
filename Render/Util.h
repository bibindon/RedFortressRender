#pragma once

#include <string>

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

class Util
{
public:

    static std::wstring Utf8ToWstring(const std::string& utf8);
    static std::string WstringToUtf8(const std::wstring& wstr);

};

