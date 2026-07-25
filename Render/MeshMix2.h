#pragma once

#include "Common.h"
#include "MeshParam.h"
#include "MeshMix2Hierarchy.h"

#include <atomic>
#include <thread>

namespace NSRender
{

class MeshMix2 : public IDeviceResettable
{
public:
    static void SetSharedThicknessTexture(LPDIRECT3DTEXTURE9 texture);
    static void SetSharedMirrorTexture(LPDIRECT3DTEXTURE9 texture);
    static void SetSharedMirrorViewProj(const D3DXMATRIX& matrix);
    static void SetSharedMirrorClipPlane(bool enabled, const D3DXVECTOR4& plane);

    MeshMix2(const std::wstring& filename,
             const D3DXVECTOR3& pos,
             const D3DXVECTOR3& rotate,
             float scale,
             const stMeshParam& param);
    ~MeshMix2();

    MeshMix2(const MeshMix2&) = delete;
    MeshMix2& operator=(const MeshMix2&) = delete;

    void Initialize(bool async = true);
    void WaitForLoad();
    void Finalize();

    void Render(bool renderAsMirrorSurface = false);
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
    void SetFresnelIntensity(float intensity);
    void SetCubeMappingRate(float rate);
    void SetSpecularIntensityOverrideEnabled(bool enabled);
    void SetSpecularEdgeOverrideEnabled(bool enabled);
    void SetTreatTextureAsWhite(bool enabled);
    void SetSSS(bool enabled);
    void SetSSSIntensity(float intensity);
    void SetSSSColor(DWORD color);

    D3DXVECTOR3 GetPos() const;
    D3DXVECTOR3 GetRot() const;
    D3DXMATRIX GetWorldMatrix() const;
    float GetScale() const;
    float GetRadius() const;
    bool IsEnabled() const;
    bool IsLoaded() const;
    bool IsSsaoEnabled() const;
    bool IsDepthBufferShadowEnabled() const;
    bool IsMirror() const;
    bool TryGetMirrorPlaneWorld(D3DXVECTOR3& planePoint, D3DXVECTOR3& planeNormal) const;
    std::wstring GetMeshName() const;

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:
    static constexpr const wchar_t* kShaderFilename = L".\\MeshMix2.cso";

    std::wstring m_meshName;
    MeshMix2MeshAlloc m_allocator;
    LPD3DXFRAME m_frameRoot = nullptr;
    LPD3DXEFFECT m_D3DEffect = nullptr;
    LPDIRECT3DBASETEXTURE9 m_csvCubeMap = nullptr;
    LPDIRECT3DBASETEXTURE9 m_csvNormalMap = nullptr;
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
    bool m_autoPointLightAdded = false;
    stMeshParam m_param;
    std::wstring m_autoPointLightOwnerTag;
    std::thread m_loadThread;

    void InitializeInternal();
    void ReleaseOwnedResources();
    void CorrectBlenderOfficialAxisTransforms(LPD3DXFRAME frame, bool skipCurrentFrame);
    void UpdateFrameMatrices(LPD3DXFRAME frame, const D3DXMATRIX* parentMatrix);
    void RenderFrameHierarchy(LPD3DXFRAME frame,
                              LPD3DXEFFECT effect,
                              const D3DXMATRIX& viewProjectionMatrix,
                              bool configureMaterial,
                              bool renderAsMirrorSurface);
    void RenderMeshContainer(const MeshMix2Frame& frame,
                             MeshMix2MeshContainer& container,
                             LPD3DXEFFECT effect,
                             const D3DXMATRIX& viewProjectionMatrix,
                             bool configureMaterial,
                             bool renderAsMirrorSurface);
    void CalculateRadius(LPD3DXFRAME frame, float& maxDistanceSquared) const;
    D3DXMATRIX BuildWorldMatrix() const;
    void AddAutoPointLight();
    void UpdateAutoPointLightPosition();
};

}
