#pragma once

#include "Common.h"
#include "Util.h"

namespace NSRender
{

// 視差遮蔽マッピングを行うメッシュ
class MeshPOM
{
public:

    void Initialize(const std::wstring& filename,
                    const D3DXVECTOR3& pos,
                    const D3DXVECTOR3& rot,
                    const float scale,
                    const float radius = -1.f);

    void Finalize();

    void Draw();

    void OnDeviceLost();

    void OnDeviceReset();

private:

    void AddTangentBinormalToMesh();

    //const std::wstring SHADER_FILENAME = L"res\\shader\\MeshPOM.fx";
    const std::wstring SHADER_FILENAME = L"../x64/Debug/MeshPOM.cso";

    LPD3DXEFFECT m_D3DEffect = NULL;

    LPD3DXMESH m_D3DMesh = NULL;

    LPD3DXBUFFER m_pAdjacency = NULL;

    DWORD m_materialCount = 0;

    std::vector<D3DMATERIAL9> m_materialList;

    // 1つ目が基本テクスチャ
    // 2つ目が法線テクスチャ
    // 3つ目が高さテクスチャ
    std::vector<LPDIRECT3DTEXTURE9> m_textureList;

    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    float m_scale = 1.0f;

    float m_radius = 1.0f;
};

}

