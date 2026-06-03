#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>

#include "SkinAnimMeshAlloc.h"

namespace NSRender
{

#ifdef ENABLE_CUSTOM_X_LOADER_LOG
#define CUSTOM_X_LOADER_LOG(message) WriteMeshMixSkinAnimLoadLog(message)
#else
#define CUSTOM_X_LOADER_LOG(message) do { } while (0)
#endif

enum class CustomXLoadPurpose
{
    MeshAndAnimation,
    AnimationOnly
};

struct CustomXFrameHierarchyLoadResult
{
    HRESULT hr = E_FAIL;
    int frameCount = 0;
    int meshContainerCount = 0;
    std::wstring rootFrameName;
    std::wstring message;
};

struct CustomXAnimationKey
{
    DWORD keyType = 0;
    DWORD valueCount = 0;
    std::vector<double> times;
    std::vector<float> values;
};

struct CustomXAnimation
{
    std::string frameName;
    std::vector<CustomXAnimationKey> keys;
};

struct CustomXAnimationSet
{
    std::string name;
    double ticksPerSecond = 4800.0;
    std::vector<CustomXAnimation> animations;
};

void WriteMeshMixSkinAnimLoadLog(const std::wstring& message);
std::wstring FormatHRESULT(const HRESULT hr);
std::wstring AnsiTextToWideText(const std::string& text);

CustomXFrameHierarchyLoadResult LoadCustomXFrameHierarchyForTest(const std::wstring& filePath,
                                                                 bool loadMeshContainers = false);

HRESULT LoadCustomXFrameHierarchyFromText(const std::string& fileText,
                                          SkinAnimMeshAlloc* allocator,
                                          LPD3DXFRAME* frameRoot,
                                          std::vector<CustomXAnimationSet>* outAnimationSets = nullptr,
                                          CustomXLoadPurpose loadPurpose = CustomXLoadPurpose::MeshAndAnimation);

HRESULT CreateAnimationControllerFromParsedData(const std::vector<CustomXAnimationSet>& animationSets,
                                                 LPD3DXFRAME frameRoot,
                                                 LPD3DXANIMATIONCONTROLLER* outController);

void DestroyCustomXFrameHierarchy(LPD3DXFRAME frame);

void DestroyCustomXFrameHierarchyWithAllocator(LPD3DXFRAME frame, SkinAnimMeshAlloc& allocator);

int CountCustomXFrames(const LPD3DXFRAME frame);

int CountCustomXMeshContainers(const LPD3DXFRAME frame);

}
