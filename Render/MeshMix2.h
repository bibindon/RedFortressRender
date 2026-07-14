#pragma once

#include "Common.h"
#include "MeshMix2Hierarchy.h"

#include <atomic>
#include <thread>

namespace NSRender
{

struct MeshMix2Param
{
    bool saturateShadow = true;
    float saturateShadowIntensity = 1.2f;
    float shadowDarkness = 1.0f;
    float specularIntensity = 0.0f;
    float specularEdge = 0.0f;
    bool specularIntensityOverrideEnabled = false;
    bool specularEdgeOverrideEnabled = false;
    bool treatTextureAsWhite = false;
    bool shadow = true;
    bool ssao = true;
};

class MeshMix2 : public IDeviceResettable
{
public:
    MeshMix2(const std::wstring& filename,
             const D3DXVECTOR3& pos,
             const D3DXVECTOR3& rotate,
             float scale,
             const MeshMix2Param& param);
    ~MeshMix2();

    MeshMix2(const MeshMix2&) = delete;
    MeshMix2& operator=(const MeshMix2&) = delete;

    void Initialize(bool async = true);
    void WaitForLoad();
    void Finalize();

    void Render();
    void RenderToEffect(LPD3DXEFFECT effect);
    void RenderToEffect(LPD3DXEFFECT effect, const D3DXMATRIX& viewProjectionMatrix);

    void SetPos(const D3DXVECTOR3& pos);
    void SetRotY(float rotY);
    void SetWorldMatrix(const D3DXMATRIX& matrix);
    void SetEnabled(bool enabled);
    void SetDamageFlash(bool enabled);
    void SetSaturateShadow(bool enabled);
    void SetSaturateShadowIntensity(float intensity);
    void SetShadowDarkness(float darkness);
    void SetSpecularIntensity(float intensity);
    void SetSpecularEdge(float edge);
    void SetSpecularIntensityOverrideEnabled(bool enabled);
    void SetSpecularEdgeOverrideEnabled(bool enabled);
    void SetTreatTextureAsWhite(bool enabled);

    D3DXVECTOR3 GetPos() const;
    D3DXVECTOR3 GetRot() const;
    D3DXMATRIX GetWorldMatrix() const;
    float GetScale() const;
    float GetRadius() const;
    bool IsEnabled() const;
    bool IsLoaded() const;
    bool IsSsaoEnabled() const;
    bool IsDepthBufferShadowEnabled() const;
    std::wstring GetMeshName() const;

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:
    static constexpr const wchar_t* kShaderFilename = L".\\MeshMixAnimNoBone.cso";

    std::wstring m_meshName;
    MeshMix2MeshAlloc m_allocator;
    LPD3DXFRAME m_frameRoot = nullptr;
    LPD3DXEFFECT m_D3DEffect = nullptr;
    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    float m_scale = 1.0f;
    float m_radius = 0.0f;
    D3DXMATRIX m_matrixOverride;
    bool m_useMatrixOverride = false;
    std::atomic<bool> m_loaded { false };
    bool m_enabled = true;
    bool m_damageFlash = false;
    bool m_deviceResourceRegistered = false;
    MeshMix2Param m_param;
    std::thread m_loadThread;

    void InitializeInternal();
    void ReleaseOwnedResources();
    void UpdateFrameMatrices(LPD3DXFRAME frame, const D3DXMATRIX* parentMatrix);
    void RenderFrameHierarchy(LPD3DXFRAME frame,
                              LPD3DXEFFECT effect,
                              const D3DXMATRIX& viewProjectionMatrix,
                              bool configureMaterial);
    void RenderMeshContainer(const MeshMix2Frame& frame,
                             MeshMix2MeshContainer& container,
                             LPD3DXEFFECT effect,
                             const D3DXMATRIX& viewProjectionMatrix,
                             bool configureMaterial);
    void CalculateRadius(LPD3DXFRAME frame, float& maxDistanceSquared) const;
    D3DXMATRIX BuildWorldMatrix() const;
};

}
