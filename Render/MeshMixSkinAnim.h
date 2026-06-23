#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Common.h"
#include "CustomXLoader.h"
#include "AnimController.h"
#include "MeshMix.h"
#include "SkinAnimMeshAlloc.h"

namespace NSRender
{

enum class MeshMixSkinAnimLoadMode
{
    DirectX,
    Custom,
};

class MeshMixSkinAnim : public IDeviceResettable
{
public:
    struct AnimationInfo
    {
        std::wstring name;
        std::wstring filePath;
        std::wstring mode;
        bool isDefault = false;
    };

    static void SetSharedMirrorClipPlane(bool enabled, const D3DXVECTOR4& plane);

    MeshMixSkinAnim(const std::wstring& filename,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rotate,
                    const float scale,
                    const stMeshParam& param,
                    const AnimSetMap& animSetMap,
                    const MeshMixSkinAnimLoadMode loadMode = MeshMixSkinAnimLoadMode::DirectX);
    MeshMixSkinAnim(const std::wstring& meshFilename,
                    const std::wstring& animationFilename,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rotate,
                    const float scale,
                    const stMeshParam& param,
                    const AnimSetMap& animSetMap,
                    const MeshMixSkinAnimLoadMode loadMode = MeshMixSkinAnimLoadMode::DirectX);

    ~MeshMixSkinAnim();

    void Initialize(bool async = true);
    void WaitForLoad();
    void UpdateAnimation();
    void Render();
    void RenderToEffect(LPD3DXEFFECT effect);

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
    void SetAlphaClipEnabled(const bool enabled);
    void SetIgnoreTransparentMaterial(const bool enabled);
    void SetRotY(const float rotY);
    void SetScale(const float scale);

    D3DXVECTOR3 GetRot() const;
    D3DXVECTOR3 GetPos() const;
    float GetScale() const;
    bool IsEnabled() const;
    void SetEnabled(const bool enabled);
    std::wstring GetMeshName() const;
    const std::vector<AnimationInfo>& GetAnimationInfoList() const;
    bool PlayAnimation(const std::wstring& name);
    void SetAnimationSpeed(float speed);
    bool IsLoaded() const;

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:
    void InitializeInternal();

    struct AnimationClip
    {
        AnimationInfo info;
        SkinAnimMeshAlloc* allocator = nullptr;
        LPD3DXFRAME frameRoot = nullptr;
        LPD3DXANIMATIONCONTROLLER controller = nullptr;
        double currentTime = 0.0;
        double duration = 1.0;
        bool loop = true;
        bool stopWhenEnd = false;
    };

    struct BonePaletteCache
    {
        unsigned int version = 0;
        DWORD paletteSize = 0;
        std::vector<std::vector<D3DXMATRIX>> palettes;
    };

    D3DXMATRIX BuildWorldMatrix() const;
    void InvalidateBonePaletteCache();
    const std::vector<D3DXMATRIX>* GetCachedBonePalette(const SkinAnimMeshContainer* container, DWORD paletteIndex);
    void UpdateFrameMatrix(const LPD3DXFRAME frameBase, const LPD3DXMATRIX matParent);
    void ApplyAnimationFrameTransformsToMeshHierarchy(const LPD3DXFRAME meshFrameBase,
                                                      const LPD3DXFRAME animationFrameRoot);
    void UpdateActiveAnimationClip();
    bool LoadAnimationCsv();
    bool LoadAnimationClip(const AnimationInfo& info);
    void ReleaseAnimationClips();
    void RenderFrame(const LPD3DXFRAME frame);
    void RenderMeshContainer(const LPD3DXMESHCONTAINER containerBase);
    void RenderFrameToEffect(const LPD3DXFRAME frame, LPD3DXEFFECT effect);
    void RenderMeshContainerToEffect(const LPD3DXMESHCONTAINER containerBase, LPD3DXEFFECT effect);
    HRESULT LoadMeshHierarchy(const std::wstring& filePath,
                               SkinAnimMeshAlloc& allocator,
                               LPD3DXFRAME* frameRoot,
                               LPD3DXANIMATIONCONTROLLER* animationController,
                               CustomXLoadPurpose loadPurpose = CustomXLoadPurpose::MeshAndAnimation);
    HRESULT LoadMeshHierarchyWithDirectX(const std::wstring& filePath,
                                          SkinAnimMeshAlloc& allocator,
                                          LPD3DXFRAME* frameRoot,
                                          LPD3DXANIMATIONCONTROLLER* animationController);
    HRESULT LoadMeshHierarchyWithCustomLoader(const std::wstring& filePath,
                                               SkinAnimMeshAlloc& allocator,
                                               LPD3DXFRAME* frameRoot,
                                               LPD3DXANIMATIONCONTROLLER* animationController,
                                               CustomXLoadPurpose loadPurpose = CustomXLoadPurpose::MeshAndAnimation);
    HRESULT AllocateBoneMatrix(LPD3DXMESHCONTAINER containerBase);
    HRESULT AllocateAllBoneMatrix(LPD3DXFRAME frame);
    void ReleaseMeshAllocator(const LPD3DXFRAME frame);
    void ReleaseMeshContainersRecursive(const LPD3DXFRAME frame, SkinAnimMeshAlloc& allocator);
    void ReleaseMeshAllocatorRecursive(const LPD3DXFRAME frame, SkinAnimMeshAlloc& allocator);
    const std::wstring SHADER_FILENAME = L".\\MeshMixSkinAnim.cso";

    std::wstring m_meshName;
    std::wstring m_animationMeshName;
    SkinAnimMeshAlloc m_allocator;
    SkinAnimMeshAlloc m_animationAllocator;
    LPD3DXFRAME m_frameRoot = nullptr;
    LPD3DXFRAME m_animationFrameRoot = nullptr;
    LPD3DXEFFECT m_D3DEffect = nullptr;

    std::unordered_map<const SkinAnimMeshContainer*, BonePaletteCache> m_bonePaletteCache;
    unsigned int m_bonePaletteVersion = 1;
    D3DXVECTOR3 m_centerPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    float m_scale = 1.0f;
    bool m_enabled = true;
    bool m_damageFlash = false;
    std::atomic<bool> m_bLoaded { false };
    std::thread m_loadThread;
    bool m_useExternalAnimation = false;
    bool m_alphaClipEnabled = true;
    bool m_ignoreTransparentMaterial = false;
    int m_activeAnimationClipIndex = -1;
    MeshMixSkinAnimLoadMode m_loadMode = MeshMixSkinAnimLoadMode::DirectX;
    bool m_hasAnimationController = false;
    stMeshParam m_param;
    AnimSetMap m_animSetMap;
    AnimController m_animController;
    std::vector<AnimationInfo> m_animationInfoList;
    std::vector<AnimationClip> m_animationClips;
    float m_animationSpeed = 1.0f;
};

}
