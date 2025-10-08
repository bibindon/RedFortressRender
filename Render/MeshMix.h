#pragma once

#include "Common.h"

namespace NSRender
{

// ポイントライト、スムーズ、SSS風、視差マッピング、SSAO、深度バッファシャドウ
// マルチレンダーターゲット、環境マッピングが有効なメッシュクラス
class MeshMix
{

public:

    MeshMix(const std::wstring& filename,
            const D3DXVECTOR3& pos,
            const D3DXVECTOR3& rotate,
            const float scale,
            const float radius = -1.f);

    void Initialize();

    void Finalize();

    void Render();

    void SetPos(const D3DXVECTOR3& pos);

    D3DXVECTOR3 GetPos() const;

    void SetRotY(const float rotY);

    float GetScale() const;

    LPD3DXMESH GetD3DMesh() const;

    float GetRadius() const;

    std::wstring GetMeshName();

    // 解像度やウィンドウモードを変更したときのための関数
    void OnDeviceLost();
    void OnDeviceReset();

private:

    const std::wstring SHADER_FILENAME = L"res\\shader\\MeshMix.fx";
    std::wstring m_meshName;

    LPD3DXEFFECT m_D3DEffect = nullptr;
    LPD3DXMESH m_D3DMesh = nullptr;

    DWORD m_materialCount = 0;
    std::vector<D3DXVECTOR4> m_vecDiffuse;
    std::vector<LPDIRECT3DTEXTURE9> m_vecTexture;

    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.f, 0.f, 0.f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.f, 0.f, 0.f);

    //-------------------------------------------------
    // この物体の半径
    // プレイヤーがこの半径以内に近づいたらこの物体は衝突判定の対象となる
    // -1だったら必ず衝突判定の対象にする
    //-------------------------------------------------
    float m_radius = 0.0f;

    float m_scale = 0.0f;

    bool m_bLoaded = false;

    //---------------------------------------------------------
    // TODO
    //---------------------------------------------------------

    // 環境マッピングをどれくらい効かせるか

    // アンビエント光のあり・なし
    // 環境マッピングがあるなら要らないはず

    // SSS風効果をどれくらい効かせるか

    // スペキュラ光をどれくらい鋭くするか
    // スペキュラ光をどれだけ明るくするか

    // 視差マッピングのON/OFF

    // 法線マッピングのON/OFF

    // SSAOのON/OFF

    // 深度バッファシャドウの対象に含めるか

    // ポイントライトを登録

    // 頂点シェーダーでユラユラと揺らすか

    // ガラスか

    // 波か
};
}

