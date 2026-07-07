#include "pch.h"
#include "CppUnitTest.h"

#include "../Render/Common.h"
#include "../Render/CustomXLoader.h"
#include "../Render/MeshMixSkinAnim.h"
#include "../Render/Render.h"

#include <cmath>

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

        bool IsMatrixDifferent(const D3DXMATRIX& lhs, const D3DXMATRIX& rhs)
        {
            for (int row = 0; row < 4; ++row)
            {
                for (int column = 0; column < 4; ++column)
                {
                    if (std::fabs(lhs.m[row][column] - rhs.m[row][column]) > 0.0001f)
                    {
                        return true;
                    }
                }
            }

            return false;
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

        std::wstring GetWolfFilePath(const std::wstring& fileName)
        {
            wchar_t currentDirectory[MAX_PATH] { };
            const DWORD length = GetCurrentDirectoryW(_countof(currentDirectory), currentDirectory);
            if (length == 0 || length >= _countof(currentDirectory))
            {
                return L"Sample\\res\\model\\wolf\\" + fileName;
            }

            std::wstring directory(currentDirectory);
            for (int i = 0; i < 8; ++i)
            {
                const std::wstring candidate = directory + L"\\Sample\\res\\model\\wolf\\" + fileName;
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

            return L"Sample\\res\\model\\wolf\\" + fileName;
        }

        std::wstring GetBlender512CylinderSkinnedFilePath()
        {
            wchar_t currentDirectory[MAX_PATH] { };
            const DWORD length = GetCurrentDirectoryW(_countof(currentDirectory), currentDirectory);
            if (length == 0 || length >= _countof(currentDirectory))
            {
                return L"Sample\\res\\Blender5.1.2Sample\\CylinderSkinned\\untitled.X";
            }

            std::wstring directory(currentDirectory);
            for (int i = 0; i < 8; ++i)
            {
                const std::wstring candidate =
                    directory + L"\\Sample\\res\\Blender5.1.2Sample\\CylinderSkinned\\untitled.X";
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

            return L"Sample\\res\\Blender5.1.2Sample\\CylinderSkinned\\untitled.X";
        }

        std::wstring GetBlender512CylinderSkinnedSeparatedFilePath(const std::wstring& fileName)
        {
            wchar_t currentDirectory[MAX_PATH] { };
            const DWORD length = GetCurrentDirectoryW(_countof(currentDirectory), currentDirectory);
            if (length == 0 || length >= _countof(currentDirectory))
            {
                return L"Sample\\res\\Blender5.1.2Sample\\CylinderSkinned_Separated\\" + fileName;
            }

            std::wstring directory(currentDirectory);
            for (int i = 0; i < 8; ++i)
            {
                const std::wstring candidate =
                    directory + L"\\Sample\\res\\Blender5.1.2Sample\\CylinderSkinned_Separated\\" + fileName;
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

            return L"Sample\\res\\Blender5.1.2Sample\\CylinderSkinned_Separated\\" + fileName;
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

        std::wstring GetMarineAssetFilePath(const std::wstring& fileName)
        {
            return L"C:\\Users\\bibindon\\Nextcloud\\RedFortressAsset\\marine\\blender5.1.2\\" + fileName;
        }

        std::wstring FormatDiagnosticDouble(const double value)
        {
            wchar_t buffer[64] { };
            std::swprintf(buffer, _countof(buffer), L"%.9f", value);
            return buffer;
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

        TEST_METHOD(LoadBlender512CylinderSkinnedWithDirectXLoader)
        {
            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            const std::wstring filePath = GetBlender512CylinderSkinnedFilePath();
            NSRender::SkinAnimMeshAlloc allocator(filePath);
            LPD3DXFRAME frameRoot = nullptr;
            LPD3DXANIMATIONCONTROLLER animationController = nullptr;

            const HRESULT hr = D3DXLoadMeshHierarchyFromX(filePath.c_str(),
                                                          D3DXMESH_MANAGED | D3DXMESH_32BIT,
                                                          NSRender::Common::D3DDevice(),
                                                          &allocator,
                                                          nullptr,
                                                          &frameRoot,
                                                          &animationController);

            UINT animationSetCount = 0;
            std::wstring animationSetName;
            if (animationController != nullptr)
            {
                animationSetCount = animationController->GetNumAnimationSets();
                LPD3DXANIMATIONSET animationSet = nullptr;
                if (SUCCEEDED(animationController->GetAnimationSet(0, &animationSet)) && animationSet != nullptr)
                {
                    const char* rawName = animationSet->GetName();
                    if (rawName != nullptr)
                    {
                        const int required = MultiByteToWideChar(CP_UTF8, 0, rawName, -1, nullptr, 0);
                        if (required > 1)
                        {
                            animationSetName.resize(required - 1);
                            MultiByteToWideChar(CP_UTF8, 0, rawName, -1, &animationSetName[0], required);
                        }
                    }
                    NSRender::SAFE_RELEASE(animationSet);
                }
            }

            NSRender::SAFE_RELEASE(animationController);
            if (frameRoot != nullptr)
            {
                D3DXFrameDestroy(frameRoot, &allocator);
            }

            wchar_t message[128] { };
            swprintf_s(message,
                       L"D3DXLoadMeshHierarchyFromX failed for Blender 5.1.2 skinned X file. HR=0x%08X",
                       static_cast<unsigned int>(hr));
            Assert::IsTrue(SUCCEEDED(hr), message);
            Assert::AreEqual(1U, animationSetCount);
            Assert::AreEqual(std::wstring(L"BlockWiggle"), animationSetName);
        }

        TEST_METHOD(LoadBlender512CylinderSkinnedWithCustomLoader)
        {
            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            const std::wstring filePath = GetBlender512CylinderSkinnedFilePath();
            const NSRender::CustomXFrameHierarchyLoadResult result =
                NSRender::LoadCustomXFrameHierarchyForTest(filePath, true);

            Assert::IsTrue(SUCCEEDED(result.hr), result.message.c_str());
            Assert::IsTrue(result.frameCount >= 3);
            Assert::IsTrue(result.meshContainerCount >= 1);
        }

        TEST_METHOD(AddBlender512CylinderSkinnedDirectXLoaderAnimationInfo)
        {
            const std::wstring shaderDirectory = GetCompiledShaderDirectory();
            Assert::IsFalse(shaderDirectory.empty(), L"MeshMixSkinAnim.cso was not found.");

            CurrentDirectoryScope currentDirectoryScope(shaderDirectory);
            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            NSRender::AnimSetMap animSetMap;
            NSRender::MeshMixSkinAnim mesh(GetBlender512CylinderSkinnedFilePath(),
                                           D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                           D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                                           1.0f,
                                           NSRender::GetMeshParamPreset(NSRender::eMeshParamPreset::GRASS),
                                           animSetMap,
                                           NSRender::MeshMixSkinAnimLoadMode::DirectX);

            mesh.Initialize(false);

            const auto& animationInfoList = mesh.GetAnimationInfoList();
            Assert::AreEqual(static_cast<std::size_t>(1), animationInfoList.size());
            Assert::AreEqual(std::wstring(L"BlockWiggle"), animationInfoList.at(0).name);
            Assert::IsTrue(animationInfoList.at(0).isDefault);
            Assert::IsTrue(mesh.PlayAnimation(L"BlockWiggle"));

            D3DXMATRIX beforeUpdate { };
            D3DXMATRIX afterUpdate { };
            Assert::IsTrue(mesh.GetBoneWorldMatrix("Bend_02_Upper", beforeUpdate));
            for (int i = 0; i < 60; ++i)
            {
                mesh.UpdateAnimation();
            }
            Assert::IsTrue(mesh.GetBoneWorldMatrix("Bend_02_Upper", afterUpdate));
            Assert::IsTrue(IsMatrixDifferent(beforeUpdate, afterUpdate),
                           L"Bend_02_Upper matrix did not change after advancing BlockWiggle.");
        }

        TEST_METHOD(AddBlender512CylinderSkinnedSeparatedDirectXLoaderAnimation)
        {
            const std::wstring shaderDirectory = GetCompiledShaderDirectory();
            Assert::IsFalse(shaderDirectory.empty(), L"MeshMixSkinAnim.cso was not found.");

            CurrentDirectoryScope currentDirectoryScope(shaderDirectory);
            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            NSRender::AnimSetMap animSetMap;
            NSRender::MeshMixSkinAnim mesh(
                GetBlender512CylinderSkinnedSeparatedFilePath(L"untitled.nonAnim.X"),
                GetBlender512CylinderSkinnedSeparatedFilePath(L"untitled.AnimOnly.X"),
                D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                D3DXVECTOR3(0.0f, 0.0f, 0.0f),
                1.0f,
                NSRender::GetMeshParamPreset(NSRender::eMeshParamPreset::GRASS),
                animSetMap,
                NSRender::MeshMixSkinAnimLoadMode::DirectX);

            mesh.Initialize(false);

            const auto& animationInfoList = mesh.GetAnimationInfoList();
            Assert::AreEqual(static_cast<std::size_t>(1), animationInfoList.size());
            Assert::AreEqual(std::wstring(L"BlockWiggle"), animationInfoList.at(0).name);
            Assert::IsTrue(mesh.PlayAnimation(L"BlockWiggle"));

            D3DXMATRIX beforeUpdate { };
            D3DXMATRIX afterUpdate { };
            Assert::IsTrue(mesh.GetBoneWorldMatrix("Bend_02_Upper", beforeUpdate));
            for (int i = 0; i < 60; ++i)
            {
                mesh.UpdateAnimation();
            }
            Assert::IsTrue(mesh.GetBoneWorldMatrix("Bend_02_Upper", afterUpdate));
            Assert::IsTrue(IsMatrixDifferent(beforeUpdate, afterUpdate),
                           L"Bend_02_Upper matrix did not change after advancing separated BlockWiggle.");
        }

        TEST_METHOD(DiagnoseMarineCustomXSkinning)
        {
            const std::wstring filePath = GetMarineAssetFilePath(L"marine_decimate50.nonAnim.X");
            if (!FileExists(filePath))
            {
                Logger::WriteMessage(L"Marine diagnostic skipped. Asset file was not found.");
                return;
            }

            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            const NSRender::CustomXSkinningDiagnosticResult result =
                NSRender::DiagnoseCustomXSkinningForTest(filePath);

            std::wstring message = L"Marine custom X diagnostic\n";
            message += L"HR=" + NSRender::FormatHRESULT(result.hr) + L"\n";
            message += L"FrameCount=" + std::to_wstring(result.frameCount) + L"\n";
            message += L"MeshContainerCount=" + std::to_wstring(result.meshContainerCount) + L"\n";
            message += L"MaxPaletteSize=" + std::to_wstring(result.maxPaletteSize) + L"\n";
            message += L"MaxInfluenceCount=" + std::to_wstring(result.maxInfluenceCount) + L"\n";
            message += L"MaxBoneCount=" + std::to_wstring(result.maxBoneCount) + L"\n";
            message += L"MaxAbsFrameCombined=" + FormatDiagnosticDouble(result.maxAbsFrameCombined) + L"\n";
            message += L"MaxAbsBoneOffset=" + FormatDiagnosticDouble(result.maxAbsBoneOffset) + L"\n";
            message += L"MaxBindPoseError=" + FormatDiagnosticDouble(result.maxBindPoseError) + L"\n";
            message += L"MaxBindPoseErrorParentLocalFrame=" +
                       FormatDiagnosticDouble(result.maxBindPoseErrorParentLocalFrame) + L"\n";
            message += L"MaxBindPoseErrorCombinedOffset=" +
                       FormatDiagnosticDouble(result.maxBindPoseErrorCombinedOffset) + L"\n";
            message += L"MaxBindPoseErrorTransposedOffset=" +
                       FormatDiagnosticDouble(result.maxBindPoseErrorTransposedOffset) + L"\n";
            message += L"MaxBindPoseErrorBoneName=" + result.maxBindPoseErrorBoneName + L"\n";
            message += L"Message=" + result.message + L"\n";
            Logger::WriteMessage(message.c_str());

            Assert::IsTrue(SUCCEEDED(result.hr), L"Custom X diagnostic load failed.");
            Assert::IsTrue(result.meshContainerCount > 0, L"No mesh containers were created.");
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

        TEST_METHOD(AddMeshMixSkinAnimEmbeddedAnimationWithCustomLoader)
        {
            const std::wstring shaderDirectory = GetCompiledShaderDirectory();
            Assert::IsFalse(shaderDirectory.empty(), L"MeshMixSkinAnim.cso was not found.");

            CurrentDirectoryScope currentDirectoryScope(shaderDirectory);
            HiddenWindowScope windowScope;
            Assert::IsNotNull(windowScope.GetHWnd(), L"Failed to create a hidden test window.");

            D3DDeviceScope deviceScope(windowScope.GetHWnd());
            Assert::IsTrue(deviceScope.IsValid(), L"Failed to create a Direct3D9 test device.");

            NSRender::Render render;
            const std::wstring meshPath = GetWolfFilePath(L"wolf.x");
            const NSRender::AnimSetMap animSetMap;

            const NSRender::CustomXFrameHierarchyLoadResult loadResult =
                NSRender::LoadCustomXFrameHierarchyForTest(meshPath, true);
            Assert::IsTrue(SUCCEEDED(loadResult.hr), loadResult.message.c_str());
            Assert::IsTrue(loadResult.meshContainerCount > 0);

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
                Assert::Fail(L"AddMeshMixSkinAnim(wolf.x) should load without a csv file when the custom loader is used.");
            }

            Assert::IsTrue(renderId >= 0);
            Assert::IsTrue(render.RemoveMeshMixSkinAnim(renderId));
        }
    };
}
