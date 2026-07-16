#include "Util.h"

#include <Windows.h>
#include <exception>
#include <fstream>
#include <mutex>
#include <set>

namespace NSRender
{
namespace
{

std::mutex g_debugFileLogMutex;
std::set<std::wstring> g_initializedDebugLogFiles;

}

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

std::wstring Util::GetExeDir()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileName(nullptr, path, MAX_PATH);

    std::wstring exePath = path;
    const size_t pos = exePath.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
    {
        return L"";
    }

    return exePath.substr(0, pos + 1);
}

void Util::WriteDebugAndFileLog(const std::wstring& fileName,
                                const std::wstring& source,
                                const std::wstring& message)
{
    SYSTEMTIME localTime = {};
    GetLocalTime(&localTime);

    wchar_t prefix[160] = {};
    swprintf_s(prefix,
               _countof(prefix),
               L"[%04u-%02u-%02u %02u:%02u:%02u.%03u] [Thread %lu] [%ls] ",
               localTime.wYear,
               localTime.wMonth,
               localTime.wDay,
               localTime.wHour,
               localTime.wMinute,
               localTime.wSecond,
               localTime.wMilliseconds,
               GetCurrentThreadId(),
               source.c_str());

    const std::wstring line = std::wstring(prefix) + message + L"\r\n";

    std::lock_guard<std::mutex> lock(g_debugFileLogMutex);
    OutputDebugStringW(line.c_str());

    const int utf8Length = WideCharToMultiByte(CP_UTF8,
                                                WC_ERR_INVALID_CHARS,
                                                line.c_str(),
                                                static_cast<int>(line.size()),
                                                nullptr,
                                                0,
                                                nullptr,
                                                nullptr);
    if (utf8Length <= 0)
    {
        OutputDebugStringW(L"[CustomXLoaderLog] Failed to convert a log message to UTF-8.\r\n");
        return;
    }

    std::string utf8Line(static_cast<std::size_t>(utf8Length), '\0');
    const int convertedLength = WideCharToMultiByte(CP_UTF8,
                                                     WC_ERR_INVALID_CHARS,
                                                     line.c_str(),
                                                     static_cast<int>(line.size()),
                                                     &utf8Line[0],
                                                     utf8Length,
                                                     nullptr,
                                                     nullptr);
    if (convertedLength != utf8Length)
    {
        OutputDebugStringW(L"[CustomXLoaderLog] Failed to write a complete UTF-8 log message.\r\n");
        return;
    }

    const std::wstring logPath = GetExeDir() + fileName;
    std::ios::openmode openMode = std::ios::out | std::ios::binary;
    const bool isInitialized = g_initializedDebugLogFiles.find(logPath) !=
                               g_initializedDebugLogFiles.end();
    if (isInitialized)
    {
        openMode |= std::ios::app;
    }
    else
    {
        openMode |= std::ios::trunc;
    }

    std::ofstream logFile(logPath.c_str(), openMode);
    if (!logFile)
    {
        const std::wstring errorMessage = L"[CustomXLoaderLog] Failed to open log file: " +
                                          logPath +
                                          L"\r\n";
        OutputDebugStringW(errorMessage.c_str());
        return;
    }

    if (!isInitialized)
    {
        const unsigned char utf8Bom[] = { 0xEF, 0xBB, 0xBF };
        logFile.write(reinterpret_cast<const char*>(utf8Bom), sizeof(utf8Bom));
        g_initializedDebugLogFiles.insert(logPath);
    }

    logFile.write(utf8Line.data(), static_cast<std::streamsize>(utf8Line.size()));
    logFile.flush();
    if (!logFile)
    {
        const std::wstring errorMessage = L"[CustomXLoaderLog] Failed to write log file: " +
                                          logPath +
                                          L"\r\n";
        OutputDebugStringW(errorMessage.c_str());
    }
}

}

