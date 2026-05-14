#pragma once

#include "Common.h"
#include "MeshMix.h"

#include <atomic>
#include <thread>

namespace NSRender
{

// D3DXEFFECT ファイルはメッシュごとに生成するのではなく、
// 複数のメッシュで 1 つのエフェクトを共有するためのクラス。
class MeshMixManager : public IDeviceResettable
{

public:

    static void SetSharedThicknessTexture(LPDIRECT3DTEXTURE9 texture);

    MeshMixManager(const std::wstring& filename,
                   const D3DXVECTOR3& pos,
                   const D3DXVECTOR3& rotate,
                   const float scale,
                   const stMeshParam& param);

    ~MeshMixManager();

    MeshMixManager(const MeshMixManager&) = delete;
    MeshMixManager& operator=(const MeshMixManager&) = delete;
    MeshMixManager(MeshMixManager&& other) noexcept;
    MeshMixManager& operator=(MeshMixManager&& other) noexcept;

    void Initialize(bool async = true);

    void WaitForLoad();

    void Finalize();

    void Render();

    void SetPos(const D3DXVECTOR3& pos);
    void SetSaturateShadow(const bool enabled);
    void SetSaturateShadowIntensity(const float intensity);
    void SetShadowDarkness(const float darkness);
    void SetSpecularIntensity(const float intensity);
    void SetSpecularEdge(const float edge);
    void SetSpecularIntensityOverrideEnabled(const bool enabled);
    void SetSpecularEdgeOverrideEnabled(const bool enabled);
    void SetSSS(const bool enabled);
    void SetSSSIntensity(const float intensity);
    void SetSSSColor(const DWORD color);

    D3DXVECTOR3 GetPos() const;

    void SetRotY(const float rotY);

    D3DXVECTOR3 GetRot() const;

    float GetScale() const;
    DWORD GetSubsetCount() const;
    bool IsEnabled() const;
    void SetEnabled(bool enabled);
    bool IsLoaded() const;
    bool IsSsaoEnabled() const;
    bool IsDepthBufferShadowEnabled() const;

    LPD3DXMESH GetD3DMesh() const;

    float GetRadius() const;

    std::wstring GetMeshName();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    const std::wstring SHADER_FILENAME = L".\\MeshMix.cso";
    std::wstring m_meshName;

    LPD3DXMESH m_D3DMesh = nullptr;

    DWORD m_materialCount = 0;
    DWORD m_subsetCount = 0;
    std::vector<D3DXVECTOR4> m_vecDiffuse;
    std::vector<float> m_vecSpecularIntensity;
    std::vector<float> m_vecSpecularPower;
    std::vector<LPDIRECT3DBASETEXTURE9> m_vecTexture;
    LPDIRECT3DBASETEXTURE9 m_texCubeMap = nullptr;
    LPDIRECT3DBASETEXTURE9 m_texNormalMap = nullptr;
    LPDIRECT3DBASETEXTURE9 m_texHeightMap = nullptr;

    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.f, 0.f, 0.f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.f, 0.f, 0.f);
    float m_scale = 0.0f;

    std::atomic<bool> m_bLoaded { false };
    bool m_enabled = true;
    bool m_autoPointLightAdded = false;
    bool m_deviceResourceRegistered = false;
    std::thread m_loadThread;

    stMeshParam m_param;
    std::wstring m_autoPointLightOwnerTag;

    void ModifyMeshForNormalMapping(LPD3DXMESH& pMesh);
    void DrawAllSubsets(LPD3DXEFFECT sharedEffect, UINT passIndex);
    D3DXVECTOR4 GetSubsetDiffuse(const DWORD subsetIndex) const;
    float GetSubsetSpecularIntensity(const DWORD subsetIndex) const;
    float GetSubsetSpecularPower(const DWORD subsetIndex) const;
    LPDIRECT3DBASETEXTURE9 GetSubsetTexture(const DWORD subsetIndex) const;
    void ReleaseOwnedResources();
    void InitializeInternal();
};
}
