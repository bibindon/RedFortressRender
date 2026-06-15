#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <memory>

#include "Common.h"
#include "AnimController.h"
#include "SkinAnimMeshAlloc.h"

namespace NSRender

{

const DWORD MAX_MATRICES = 32;

class SkinAnimMesh : public IDeviceResettable
{
public:
    SkinAnimMesh(const std::wstring &,
                 const D3DXVECTOR3 &,
                 const D3DXVECTOR3 &,
                 const float &,
                 const AnimSetMap& animSetMap);

    ~SkinAnimMesh();

    void UpdateAnimation();
    void Render(const D3DXMATRIX&,
                const D3DXMATRIX&,
                const D3DXVECTOR4&,
                const float&);

    void OnDeviceLost();
    void OnDeviceReset();

private:

    D3DXMATRIX BuildWorldMatrix() const;
    void RenderImpl(const D3DXMATRIX &, const D3DXMATRIX &);

    void ReleaseMeshAllocator(const LPD3DXFRAME);

    const static std::wstring SHADER_FILENAME;

    SkinAnimMeshAlloc m_allocator;

    LPD3DXFRAME m_frameRoot = NULL;

    D3DXMATRIX m_matRotate;
    std::vector<D3DXMATRIX> m_matWorldArray;
    D3DXVECTOR3 m_centerPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    void UpdateFrameMatrix(const LPD3DXFRAME, const LPD3DXMATRIX);
    void RenderFrame(const LPD3DXFRAME);
    void RenderMeshContainer(const LPD3DXMESHCONTAINER);

    HRESULT AllocateBoneMatrix(LPD3DXMESHCONTAINER);
    HRESULT AllocateAllBoneMatrix(LPD3DXFRAME);

    LPD3DXEFFECT m_D3DEffect = NULL;
    D3DXVECTOR3 m_position = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    float m_scale = 1.0f;

    AnimController m_animController;
};

}


