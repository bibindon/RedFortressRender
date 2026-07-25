#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <thread>

#include "MeshParam.h"
#include "MeshMixSkinAnimCommon.h"
#include "Common.h"
#include "AnimController.h"
#include "MeshMixAnimNoBoneAlloc.h"

namespace NSRender
{

class MeshMixAnimNoBone : public IDeviceResettable
{
public:

    struct AnimationInfo
    {
        std::wstring name;
        std::wstring filePath;
        std::wstring mode;
        bool isDefault = false;
    };

    MeshMixAnimNoBone(const std::wstring& filename,
                      const D3DXVECTOR3& pos,
                      const D3DXVECTOR3& rotate,
                      const float scale,
                      const stMeshParam& param,
                      const AnimSetMap& animSetMap,
                      const MeshMixSkinAnimLoadMode loadMode = MeshMixSkinAnimLoadMode::DirectX);

    MeshMixAnimNoBone(const std::wstring& meshFilename,
                      const std::wstring& animationFilename,
                      const D3DXVECTOR3& pos,
                      const D3DXVECTOR3& rotate,
                      const float scale,
                      const stMeshParam& param,
                      const AnimSetMap& animSetMap,
                      const MeshMixSkinAnimLoadMode loadMode = MeshMixSkinAnimLoadMode::DirectX);

    virtual ~MeshMixAnimNoBone();

    void Initialize(bool async = true);
    void WaitForLoad();
    void UpdateAnimation();
    void Render();
    void RenderToEffect(LPD3DXEFFECT effect);
    void RenderToEffect(LPD3DXEFFECT effect, const D3DXMATRIX& viewProjectionMatrix);

    void SetPos(const D3DXVECTOR3& pos);
    void SetSaturateShadow(const bool enabled);
    void SetSaturateShadowIntensity(const float intensity);
    void SetShadowDarkness(const float darkness);
    void SetSpecularIntensity(const float intensity);
    void SetSpecularEdge(const float edge);
    void SetFresnelIntensity(const float intensity);
    void SetSpecularIntensityOverrideEnabled(const bool enabled);
    void SetSpecularEdgeOverrideEnabled(const bool enabled);
    void SetTreatTextureAsWhite(const bool enabled);
    void SetDamageFlash(const bool enabled);
    void SetYellowFlash(const bool enabled);
    void SetAlphaClipEnabled(const bool enabled);
    void SetIgnoreTransparentMaterial(const bool enabled);
    void SetRotY(const float rotY);
    void SetScale(const float scale);

    D3DXVECTOR3 GetRot() const;
    D3DXVECTOR3 GetPos() const;
    float GetScale() const;
    bool IsEnabled() const;
    std::wstring GetMeshName() const;
    const std::vector<AnimationInfo>& GetAnimationInfoList() const;
    bool PlayAnimation(const std::wstring& name);
    void SetAnimationSpeed(float speed);
    bool IsLoaded() const;
    void SetEnabled(const bool enabled);

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:

    struct AnimationClip
    {
        AnimationInfo info;
        AnimNoBoneMeshAlloc* allocator = nullptr;
        LPD3DXFRAME frameRoot = nullptr;
        LPD3DXANIMATIONCONTROLLER controller = nullptr;
        double currentTime = 0.0;
        double duration = 1.0;
        bool loop = true;
        bool stopWhenEnd = false;
    };

    void InitializeInternal();
    void ReleaseAnimationClips();
    void ReleaseMeshAllocatorRecursive(LPD3DXFRAME frame, AnimNoBoneMeshAlloc& allocator);
    void ReleaseMeshAllocator(const LPD3DXFRAME frame);
    void ReleaseMeshContainersRecursive(LPD3DXFRAME frame, AnimNoBoneMeshAlloc& allocator);
    bool LoadAnimationCsv();
    bool LoadAnimationClip(const AnimationInfo& info);
    bool LoadAnimationClip(const std::wstring& filePath, AnimationClip& outClip);
    HRESULT LoadMeshHierarchyWithDirectX(const std::wstring& path, AnimNoBoneMeshAlloc& allocator,
                                          LPD3DXFRAME& outRoot, LPD3DXANIMATIONCONTROLLER& outController);
    HRESULT LoadMeshHierarchyWithCustomLoader(const std::wstring& path, AnimNoBoneMeshAlloc& allocator,
                                               LPD3DXFRAME& outRoot);
    void ApplyAnimationFrameTransformsToMeshHierarchy(LPD3DXFRAME meshFrameRoot,
                                                       LPD3DXFRAME animFrameRoot);
    void UpdateFrameMatrix(const LPD3DXFRAME frameBase, const LPD3DXMATRIX matParent = nullptr);
    D3DXMATRIX BuildWorldMatrix() const;
    void UpdateActiveAnimationClip();
    void RenderFrameHierarchy(LPD3DXFRAME frame, LPD3DXEFFECT effect);
    void RenderFrameHierarchy(LPD3DXFRAME frame, LPD3DXEFFECT effect, const D3DXMATRIX& viewProjectionMatrix);
    HRESULT LoadMeshHierarchy(const std::wstring& filePath,
                               AnimNoBoneMeshAlloc& allocator,
                               LPD3DXFRAME* outRoot,
                               LPD3DXANIMATIONCONTROLLER* outController);

    static void SetSharedMirrorClipPlane(bool enabled, const D3DXVECTOR4& plane);
    HRESULT AllocateAllBoneMatrix(LPD3DXFRAME frame);
    HRESULT AllocateBoneMatrix(LPD3DXMESHCONTAINER container);
    void InvalidateBonePaletteCache();
    void RenderFrame(LPD3DXFRAME frame, const D3DXVECTOR3& cameraPos);
    void RenderMeshContainer(LPD3DXMESHCONTAINER container, DWORD attributeIndex);
    void RenderFrameToEffect(LPD3DXFRAME frame, LPD3DXEFFECT effect);
    void RenderMeshContainerToEffect(LPD3DXMESHCONTAINER container, DWORD attributeIndex, LPD3DXEFFECT effect);

    struct BonePaletteCache
    {
        unsigned int version = 0;
        DWORD paletteSize = 0;
        std::vector<std::vector<D3DXMATRIX>> palettes;
    };
    mutable std::unordered_map<const AnimNoBoneMeshContainer*, BonePaletteCache> m_bonePaletteCache;
    unsigned int m_bonePaletteVersion = 1;
    const std::vector<D3DXMATRIX>* GetCachedBonePalette(const AnimNoBoneMeshContainer* container, DWORD subsetIndex) const;

    const std::wstring SHADER_FILENAME = _T(".\\MeshMixAnimNoBone.cso");
    std::wstring m_meshName;
    std::wstring m_animationMeshName;
    float m_radius = -1.0f;
    bool m_useParallaxOcclusionMapping = false;
    bool m_useNormalMapping = false;

    AnimNoBoneMeshAlloc m_allocator;
    AnimNoBoneMeshAlloc m_animationAllocator;
    LPD3DXFRAME m_frameRoot = nullptr;
    LPD3DXFRAME m_animationFrameRoot = nullptr;
    LPD3DXANIMATIONCONTROLLER m_tempAnimController = nullptr;
    bool m_hasAnimationController = false;
    LPD3DXEFFECT m_D3DEffect = nullptr;

    D3DXVECTOR3 m_centerPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    float m_scale = 1.0f;
    bool m_enabled = true;
    bool m_damageFlash = false;
    bool m_yellowFlash = false;
    std::atomic<bool> m_bLoaded { false };
    std::thread m_loadThread;
    bool m_useExternalAnimation = false;
    bool m_alphaClipEnabled = true;
    bool m_ignoreTransparentMaterial = false;
    int m_activeAnimationClipIndex = -1;
    MeshMixSkinAnimLoadMode m_loadMode = MeshMixSkinAnimLoadMode::DirectX;
    stMeshParam m_param;
    AnimSetMap m_animSetMap;
    AnimController m_animController;
    std::vector<AnimationInfo> m_animationInfoList;
    std::vector<AnimationClip> m_animationClips;
    float m_animationSpeed = 1.0f;
    double m_embeddedAnimationTime = 0.0;
    double m_embeddedAnimationDuration = 1.0;
};

}
