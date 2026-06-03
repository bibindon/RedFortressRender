#include "pch.h"
#include "CppUnitTest.h"

#include "../Render/Common.h"
#include "../Render/MeshMixSkinAnim.h"
#include "../Render/Render.h"

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

        std::wstring GetCompiledShaderDirectory()
        {
            wchar_t currentDirectory[MAX_PATH] { };
            const DWORD length = GetCurrentDirectoryW(_countof(currentDirectory), currentDirectory);
            if (length == 0 || length >= _countof(currentDirectory))
            {
                return L"";
            }

            std::wstring directory(currentDirectory);
            for (int i = 0; i < 8; ++i)
            {
                const std::wstring candidate = directory + L"\\x64\\Debug\\MeshMixSkinAnim.cso";
                if (FileExists(candidate))
                {
                    return directory + L"\\x64\\Debug";
                }

                const std::wstring renderCandidate = directory + L"\\Render\\x64\\Debug\\MeshMixSkinAnim.cso";
                if (FileExists(renderCandidate))
                {
                    return directory + L"\\Render\\x64\\Debug";
                }

                const std::size_t slashPos = directory.find_last_of(L"\\/");
                if (slashPos == std::wstring::npos)
                {
                    break;
                }

                directory = directory.substr(0, slashPos);
            }

            return L"";
        }

        class CurrentDirectoryScope
        {
        public:
            explicit CurrentDirectoryScope(const std::wstring& directory)
            {
                const DWORD length = GetCurrentDirectoryW(_countof(m_originalDirectory), m_originalDirectory);
                m_hasOriginalDirectory = length > 0 && length < _countof(m_originalDirectory);
                if (!directory.empty())
                {
                    SetCurrentDirectoryW(directory.c_str());
                }
            }

            ~CurrentDirectoryScope()
            {
                if (m_hasOriginalDirectory)
                {
                    SetCurrentDirectoryW(m_originalDirectory);
                }
            }

        private:
            wchar_t m_originalDirectory[MAX_PATH] { };
            bool m_hasOriginalDirectory = false;
        };

        class HiddenWindowScope
        {
        public:
            HiddenWindowScope()
            {
                WNDCLASSW windowClass { };
                windowClass.lpfnWndProc = DefWindowProcW;
                windowClass.hInstance = GetModuleHandleW(nullptr);
                windowClass.lpszClassName = L"RedfortressRenderUnitTestWindow";
                RegisterClassW(&windowClass);

                m_hWnd = CreateWindowExW(0,
                                         windowClass.lpszClassName,
                                         L"RedfortressRenderUnitTestWindow",
                                         WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         64,
                                         64,
                                         nullptr,
                                         nullptr,
                                         windowClass.hInstance,
                                         nullptr);
            }

            ~HiddenWindowScope()
            {
                if (m_hWnd != nullptr)
                {
                    DestroyWindow(m_hWnd);
                    m_hWnd = nullptr;
                }
            }

            HWND GetHWnd() const
            {
                return m_hWnd;
            }

        private:
            HWND m_hWnd = nullptr;
        };

        class D3DDeviceScope
        {
        public:
            explicit D3DDeviceScope(HWND hWnd)
            {
                m_d3d = Direct3DCreate9(D3D_SDK_VERSION);
                if (m_d3d == nullptr)
                {
                    return;
                }

                D3DPRESENT_PARAMETERS d3dpp { };
                d3dpp.Windowed = TRUE;
                d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
                d3dpp.BackBufferCount = 1;
                d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
                d3dpp.MultiSampleQuality = 0;
                d3dpp.EnableAutoDepthStencil = TRUE;
                d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
                d3dpp.hDeviceWindow = hWnd;
                d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

                HRESULT hr = m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
                                                 D3DDEVTYPE_HAL,
                                                 hWnd,
                                                 D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                                                 &d3dpp,
                                                 &m_device);
                if (FAILED(hr))
                {
                    hr = m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
                                             D3DDEVTYPE_HAL,
                                             hWnd,
                                             D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                                             &d3dpp,
                                             &m_device);
                }

                if (SUCCEEDED(hr))
                {
                    NSRender::Common::SetD3DDevice(m_device);
                }
            }

            ~D3DDeviceScope()
            {
                NSRender::Common::SetD3DDevice(nullptr);
                NSRender::SAFE_RELEASE(m_device);
                NSRender::SAFE_RELEASE(m_d3d);
                NSRender::Common::Finalize();
            }

            bool IsValid() const
            {
                return m_device != nullptr;
            }

        private:
            LPDIRECT3D9 m_d3d = nullptr;
            LPDIRECT3DDEVICE9 m_device = nullptr;
        };
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

        TEST_METHOD(AddMeshMixSkinAnimWithCustomLoader)
        {
            const std::wstring shaderDirectory = GetCompiledShaderDirectory();
            Assert::IsFalse(shaderDirectory.empty(), L"MeshMixSkinAnim.cso was not found.");

            CurrentDirectoryScope currentDirectoryScope(shaderDirectory);
            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            NSRender::Render render;
            const std::wstring meshPath = GetWolf2FilePath(L"wolfAnim.x");
            const std::wstring animationPath = GetWolf2FilePath(L"wolfAnim.run.x");
            const NSRender::AnimSetMap animSetMap;

            const int renderId = render.AddMeshMixSkinAnim(meshPath,
                                                           animationPath,
                                                           D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                           D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                           1.0f,
                                                           animSetMap,
                                                           -1.0f,
                                                           false,
                                                           false,
                                                           NSRender::MeshMixSkinAnimLoadMode::Custom);

            Assert::IsTrue(renderId >= 0);
            Assert::IsTrue(render.RemoveMeshMixSkinAnim(renderId));
        }

        TEST_METHOD(AddMeshMixSkinAnimSingleFileWithCustomLoader)
        {
            const std::wstring shaderDirectory = GetCompiledShaderDirectory();
            Assert::IsFalse(shaderDirectory.empty(), L"MeshMixSkinAnim.cso was not found.");

            CurrentDirectoryScope currentDirectoryScope(shaderDirectory);
            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            NSRender::Render render;
            const std::wstring meshPath = GetWolf2FilePath(L"wolfAnim.x");
            const NSRender::AnimSetMap animSetMap;

            int renderId = -1;
            try
            {
                renderId = render.AddMeshMixSkinAnim(meshPath,
                                                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                     D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                                     1.0f,
                                                     animSetMap,
                                                     -1.0f,
                                                     false,
                                                     false,
                                                     NSRender::MeshMixSkinAnimLoadMode::Custom);
            }
            catch (...)
            {
                Assert::Fail(L"AddMeshMixSkinAnim(wolfAnim.x) should load animations from wolfAnim.csv.");
            }

            Assert::IsTrue(renderId >= 0);
            Assert::IsTrue(render.RemoveMeshMixSkinAnim(renderId));
        }
    };
}
