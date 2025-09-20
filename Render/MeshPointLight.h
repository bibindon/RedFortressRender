#pragma once

#include "Common.h"
#include "Util.h"

namespace NSRender
{

// ポイントライトが反映されるメッシュ
// 逆に、これ以外のメッシュはポイントライトが反映されない
class MeshPointLight
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

    const std::wstring SHADER_FILENAME = L"res\\shader\\MeshPointLight.fx";

    LPD3DXEFFECT m_D3DEffect = NULL;

    LPD3DXMESH m_D3DMesh = NULL;

    DWORD m_materialCount = 0;

    std::vector<D3DMATERIAL9> m_materialList;

    std::vector<LPDIRECT3DTEXTURE9> m_textureList;

    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    float m_scale = 1.0f;

    float m_radius = 1.0f;

    struct PointLight
    {
        D3DXVECTOR3 pos = D3DXVECTOR3(0.f, 0.f, 0.f);

        float range = 1.f;  // 1 float4

        D3DXVECTOR3 color = D3DXVECTOR3(0.f, 0.f, 0.f);

        float pad = 0.f;    // 1 float4（アラインメント）
    };

};

}

