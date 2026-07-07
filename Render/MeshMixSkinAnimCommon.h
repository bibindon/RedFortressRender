#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>

#include "Common.h"

namespace NSRender
{

enum class MeshMixSkinAnimLoadMode
{
    DirectX,
    Custom,
    Blender512Custom,
};

struct MeshMixSkinAnimAnimationInfo
{
    std::wstring name;
    std::wstring filePath;
    std::wstring mode;
    bool isDefault = false;
};

class IMeshMixSkinAnim : public IDeviceResettable
{
public:
    virtual ~IMeshMixSkinAnim() = default;

    virtual void Initialize(bool async = true) = 0;
    virtual void WaitForLoad() = 0;
    virtual void UpdateAnimation() = 0;
    virtual void Render() = 0;
    virtual void RenderToEffect(LPD3DXEFFECT effect) = 0;

    virtual void SetPos(const D3DXVECTOR3& pos) = 0;
    virtual void SetSaturateShadow(bool enabled) = 0;
    virtual void SetSaturateShadowIntensity(float intensity) = 0;
    virtual void SetShadowDarkness(float darkness) = 0;
    virtual void SetSpecularIntensity(float intensity) = 0;
    virtual void SetSpecularEdge(float edge) = 0;
    virtual void SetFresnelIntensity(float intensity) = 0;
    virtual void SetSpecularIntensityOverrideEnabled(bool enabled) = 0;
    virtual void SetSpecularEdgeOverrideEnabled(bool enabled) = 0;
    virtual void SetTreatTextureAsWhite(bool enabled) = 0;
    virtual void SetDamageFlash(bool enabled) = 0;
    virtual void SetYellowFlash(bool enabled) = 0;
    virtual void SetCustomFlash(bool enabled, const D3DXVECTOR4& color) = 0;
    virtual bool GetBoneWorldMatrix(const char* boneName, D3DXMATRIX& outMatrix) const = 0;
    virtual void SetAlphaClipEnabled(bool enabled) = 0;
    virtual void SetIgnoreTransparentMaterial(bool enabled) = 0;
    virtual void SetRotY(float rotY) = 0;
    virtual void SetScale(float scale) = 0;

    virtual D3DXVECTOR3 GetRot() const = 0;
    virtual D3DXVECTOR3 GetPos() const = 0;
    virtual float GetScale() const = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual std::wstring GetMeshName() const = 0;
    virtual const std::vector<MeshMixSkinAnimAnimationInfo>& GetAnimationInfoList() const = 0;
    virtual bool PlayAnimation(const std::wstring& name) = 0;
    virtual void SetAnimationSpeed(float speed) = 0;
    virtual bool IsLoaded() const = 0;
};

}
