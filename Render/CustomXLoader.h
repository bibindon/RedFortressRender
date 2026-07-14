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

struct CustomXLoadOptions
{
    bool allowDuplicateSkinWeightsCount = false;
    bool transposeAnimationMatrixKeys = false;
};

struct CustomXFrameHierarchyLoadResult
{
    HRESULT hr = E_FAIL;
    int frameCount = 0;
    int meshContainerCount = 0;
    std::wstring rootFrameName;
    std::wstring message;
};

struct CustomXSkinningDiagnosticResult
{
    HRESULT hr = E_FAIL;
    int frameCount = 0;
    int meshContainerCount = 0;
    DWORD maxPaletteSize = 0;
    DWORD maxInfluenceCount = 0;
    DWORD maxBoneCount = 0;
    double maxAbsFrameCombined = 0.0;
    double maxAbsBoneOffset = 0.0;
    double maxBindPoseError = 0.0;
    double maxBindPoseErrorParentLocalFrame = 0.0;
    double maxBindPoseErrorCombinedOffset = 0.0;
    double maxBindPoseErrorTransposedOffset = 0.0;
    std::wstring maxBindPoseErrorBoneName;
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

CustomXSkinningDiagnosticResult DiagnoseCustomXSkinningForTest(const std::wstring& filePath,
                                                               const CustomXLoadOptions& options = CustomXLoadOptions());

HRESULT LoadCustomXFrameHierarchyFromText(const std::string& fileText,
                                          ID3DXAllocateHierarchy* allocator,
                                          LPD3DXFRAME* frameRoot,
                                          std::vector<CustomXAnimationSet>* outAnimationSets = nullptr,
                                          CustomXLoadPurpose loadPurpose = CustomXLoadPurpose::MeshAndAnimation,
                                          const CustomXLoadOptions& options = CustomXLoadOptions());

HRESULT CreateAnimationControllerFromParsedData(const std::vector<CustomXAnimationSet>& animationSets,
                                                 LPD3DXFRAME frameRoot,
                                                 LPD3DXANIMATIONCONTROLLER* outController,
                                                 const CustomXLoadOptions& options = CustomXLoadOptions());

void DestroyCustomXFrameHierarchy(LPD3DXFRAME frame);

void DestroyCustomXFrameHierarchyWithAllocator(LPD3DXFRAME frame, SkinAnimMeshAlloc& allocator);

int CountCustomXFrames(const LPD3DXFRAME frame);

int CountCustomXMeshContainers(const LPD3DXFRAME frame);

}
