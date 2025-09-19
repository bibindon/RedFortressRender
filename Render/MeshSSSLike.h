#pragma once

#include "Common.h"
#include "Util.h"

namespace NSRender
{

// SSS風メッシュ
// 陰の色を描画するときに、単純に輝度を下げるのではなく、
// 色相をずらして、彩度を上げる
class MeshSSSLike
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

    const std::wstring SHADER_FILENAME = L"res\\shader\\MeshSSSLike.fx";

    LPD3DXEFFECT m_D3DEffect = NULL;

    LPD3DXMESH m_D3DMesh = NULL;

    DWORD m_materialCount = 0;

    std::vector<D3DMATERIAL9> m_materialList;

    std::vector<LPDIRECT3DTEXTURE9> m_textureList;

    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    float m_scale = 1.0f;

    float m_radius = 1.0f;
};
}

