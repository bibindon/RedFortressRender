#pragma once

#include "Common.h"

namespace NSRender
{

struct stMeshParam
{
    // 角の法線を再計算するか。
    // 例えば半球の法線を再計算するとキノコのようになり、
    // 再計算しないとダイヤモンドのような見た目になる。
    bool smooth = true;

    // SSAOを有効にするか
    bool ssao = false;

    // 影が表示されるようにするかどうか。
    bool shadow = true;

    // SSS風の表示を行うか。
    //
    // 陰を表示するときに、輝度を少しだけ下げて、彩度を上げ、色相を少し変える
    bool fakeSSS = false;

    // 環境マッピングを行うか
    //
    // 鏡のような表現は、実は処理が重くない。
    // カメラが移動したらテクスチャも一緒に動いてるだけ。
    // 環境マッピングをボカシて表示すればそれが環境光になるので
    // 環境マッピングありの時に、環境光を表示すると明るくなりすぎることに注意
    bool cubeMapping = true;

    // 環境マッピングの表示結果をどれくらい混ぜるか
    //
    // 0.0 ~ 1.0
    float cubeMappingRate = 0.2f;

    // 環境マッピングの表示をどれくらいボカすか
    //
    // 0.0 ~ 1.0
    float cubeMappingGauss = 0.9f;

    // 視差マッピングを行うか
    // デコボコが本当にあるように見せる
    bool parallaxMapping = false;

    // 法線マッピングを行うか
    // デコボコがあるように見せる。近くで見ると平面だとすぐにわかってしまう。
    bool normalMapping = true;

    // ガラスか
    bool glass = false;

    // 波
    bool wave = true;

    // 波の強さ
    float waveIntensity = 0.1f;

    // ポイントライトで照らされるか
    // 
    // ポイントライトがあったら周りの物体が少し明るくなる。
    // 近くにポイントライトがあったら明るくなる、という処理は周りの物体が各々行う必要がある。
    bool pointLight = true;

    // 衝突判定の対象とするか
    bool collision = false;

    // 衝突判定が開始される半径
    //
    // 衝突判定はとてつもなく重い。この半径以内に入るまで衝突判定をしない、という距離
    // 高速化のため、球ではなく立方体で見る
    float collisionRadius = 2.0;
};

// TODO 何種類かのプリセットを用意して受け取れるようにする
stMeshParam GetMeshParamPreset();

// ポイントライト、スムーズ、SSS風、視差マッピング、SSAO、深度バッファシャドウ
// マルチレンダーターゲット、環境マッピングが有効なメッシュクラス
class MeshMix
{

public:

    MeshMix(const std::wstring& filename,
            const D3DXVECTOR3& pos,
            const D3DXVECTOR3& rotate,
            const float scale,
            const stMeshParam& param
            );

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

    float m_scale = 0.0f;

    bool m_bLoaded = false;

    //---------------------------------------------------------
    // TODO
    //---------------------------------------------------------

    stMeshParam m_param;

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

