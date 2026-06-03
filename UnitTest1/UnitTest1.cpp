#include "pch.h"
#include "CppUnitTest.h"

#include "../Render/MeshMixSkinAnim.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
    namespace
    {
        bool FileExists(const std::wstring& filePath)
        {
            const DWORD attributes = GetFileAttributesW(filePath.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES)
            {
                return false;
            }

            return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        }

        std::wstring GetWolf2FilePath(const std::wstring& fileName)
        {
            wchar_t currentDirectory[MAX_PATH] { };
            const DWORD length = GetCurrentDirectoryW(_countof(currentDirectory), currentDirectory);
            if (length == 0 || length >= _countof(currentDirectory))
            {
                return L"Sample\\res\\model\\wolf2\\" + fileName;
            }

            std::wstring directory(currentDirectory);
            for (int i = 0; i < 8; ++i)
            {
                const std::wstring candidate = directory + L"\\Sample\\res\\model\\wolf2\\" + fileName;
                if (FileExists(candidate))
                {
                    return candidate;
                }

                const std::size_t slashPos = directory.find_last_of(L"\\/");
                if (slashPos == std::wstring::npos)
                {
                    break;
                }

                directory = directory.substr(0, slashPos);
            }

            return L"Sample\\res\\model\\wolf2\\" + fileName;
        }
    }

    TEST_CLASS(CustomXLoaderTests)
    {
    public:

        TEST_METHOD(LoadWolfMeshFrameHierarchy)
        {
            const std::wstring filePath = GetWolf2FilePath(L"wolfAnim.x");

            const NSRender::CustomXFrameHierarchyLoadResult result =
                NSRender::LoadCustomXFrameHierarchyForTest(filePath);

            Assert::IsTrue(SUCCEEDED(result.hr), result.message.c_str());
            Assert::AreEqual(46, result.frameCount);
            Assert::AreEqual(std::wstring(L"Root"), result.rootFrameName);
        }

        TEST_METHOD(LoadWolfRunAnimationFrameHierarchy)
        {
            const std::wstring filePath = GetWolf2FilePath(L"wolfAnim.run.x");

            const NSRender::CustomXFrameHierarchyLoadResult result =
                NSRender::LoadCustomXFrameHierarchyForTest(filePath);

            Assert::IsTrue(SUCCEEDED(result.hr), result.message.c_str());
            Assert::AreEqual(45, result.frameCount);
            Assert::AreEqual(std::wstring(L"Root"), result.rootFrameName);
        }
    };
}
