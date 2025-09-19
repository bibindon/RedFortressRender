#pragma once

#include "Common.h"

namespace NSRender
{

// インスタンシング可能なメッシュ
// 大量に描画しても軽い
class MeshInstancing
{

public:

    MeshInstancing();

    void Initialize();

    void Finalize();

    // TODO rotate, scale
    void AddInstance(const D3DXVECTOR3& pos);

    void Draw();

    void OnDeviceLost();
    void OnDeviceReset();


private:

    LPD3DXMESH m_pMesh = NULL;

    std::vector<D3DMATERIAL9> m_pMaterials;

    std::vector<LPDIRECT3DTEXTURE9> m_pTextures;

    DWORD m_dwNumMaterials = 0;

    LPD3DXEFFECT m_pEffect = NULL;

    struct WorldPos
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    WorldPos* m_worldPos = nullptr;

    IDirect3DVertexBuffer9* m_worldPosBuf = nullptr;

    IDirect3DVertexDeclaration9* m_decl = nullptr;

    void copyBuf(unsigned sz, void* src, IDirect3DVertexBuffer9* buf);

    const int W = 10;
    const int H = 10;
    const int D = 10;

    // スクリーン上のチップ総数
    const int tipNum = W * H * D;

};

}

