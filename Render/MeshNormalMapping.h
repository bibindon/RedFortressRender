#pragma once

#include "Common.h"
#include "Util.h"

namespace NSRender
{

//------------------------------------------------
// 法線マッピングができるメッシュ
//------------------------------------------------

class MeshNormalMapping : public IDeviceResettable
{

public:

    void Initialize(const std::wstring& filename,
                    const std::wstring& normalMap,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rot,
                    const float scale,
                    const float radius = -1.f);

    void EnsureMeshHasTangentBinormal(LPD3DXMESH& pMesh);

    void Finalize();

    void Draw();

    void OnDeviceLost();

    void OnDeviceReset();

private:

    //const std::wstring SHADER_FILENAME = L"res\\shader\\MeshNormalMapping.fx";
    const std::wstring SHADER_FILENAME = L"../x64/Debug/MeshNormalMapping.cso";

    LPD3DXEFFECT m_D3DEffect = NULL;

    LPD3DXMESH m_D3DMesh = NULL;

    DWORD m_materialCount = 0;

    std::vector<D3DMATERIAL9> m_materialList;

    std::vector<LPDIRECT3DTEXTURE9> m_textureList;

    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    LPDIRECT3DTEXTURE9 g_pNormalTex = NULL;

    float m_scale = 1.0f;

    float m_radius = 1.0f;
};

}

