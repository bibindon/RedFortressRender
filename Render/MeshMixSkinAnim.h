#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>

#include "Common.h"
#include "AnimController.h"
#include "MeshMix.h"
#include "SkinAnimMeshAlloc.h"

namespace NSRender
{

class MeshMixSkinAnim : public IDeviceResettable
{
public:
    MeshMixSkinAnim(const std::wstring& filename,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rotate,
                    const float scale,
                    const stMeshParam& param);

    ~MeshMixSkinAnim();

    void Initialize();
    void Render();

    void SetPos(const D3DXVECTOR3& pos);
    void SetSaturateShadow(const bool enabled);
    void SetSaturateShadowIntensity(const float intensity);
    void SetShadowDarkness(const float darkness);
    void SetSpecularIntensity(const float intensity);
    void SetSpecularEdge(const float edge);
    void SetRotY(const float rotY);

    D3DXVECTOR3 GetRot() const;
    D3DXVECTOR3 GetPos() const;
    float GetScale() const;
    bool IsEnabled() const;
    void SetEnabled(const bool enabled);
    std::wstring GetMeshName() const;

    void OnDeviceLost() override;
    void OnDeviceReset() override;

private:
    void UpdateFrameMatrix(const LPD3DXFRAME frameBase, const LPD3DXMATRIX matParent);
    void RenderFrame(const LPD3DXFRAME frame);
    void RenderMeshContainer(const LPD3DXMESHCONTAINER containerBase);
    HRESULT AllocateBoneMatrix(LPD3DXMESHCONTAINER containerBase);
    HRESULT AllocateAllBoneMatrix(LPD3DXFRAME frame);
    void ReleaseMeshAllocator(const LPD3DXFRAME frame);
    void BuildDefaultAnimSetMap(const LPD3DXANIMATIONCONTROLLER animationController);

    const std::wstring SHADER_FILENAME = L".\\SkinAnimMeshShader.cso";

    std::wstring m_meshName;
    SkinAnimMeshAlloc m_allocator;
    LPD3DXFRAME m_frameRoot = nullptr;
    LPD3DXEFFECT m_D3DEffect = nullptr;

    std::vector<D3DXMATRIX> m_matWorldArray;
    D3DXVECTOR3 m_centerPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    float m_scale = 1.0f;
    bool m_enabled = true;
    bool m_bLoaded = false;
    stMeshParam m_param;
    AnimController m_animController;
};

}
