#pragma once

#include "Common.h"

namespace NSRender
{

// TODO 一つのモデルの中に、鏡のように表示したい物体と、岩のように表示したい物体があったら？
// stMeshParamは複数の持つ必要がある？

// TODO 雨に濡れたらスペキュラ光を変化させる必要がある
// スペキュラ光は可変である、ということだ

struct stMeshParam
{
    // 環境光あり
    bool ambient = true;

    // 環境光の色
    DWORD ambientColor = 0x101010ff;

    // スペキュラ光の色
    DWORD specularColor = 0xffffffff;

    // スペキュラ光の強さ
    //float specularIntensity = 0.4f;
    float specularIntensity = 0.0f;

    // スペキュラ光の鋭さ
    //float specularEdge = 0.8f;
    float specularEdge = 0.0f;

    // TODO 霧は陰の後に表示しないといけないので、ポストエフェクトで実行しなければいけない。

    //--------------------------------------------------------
    // 距離フォグ
    //--------------------------------------------------------

    // 距離による霧を表示するか
    bool fogDistance = true;

    // 距離による霧の、100%になる距離
    // その距離になったら、霧しか見えなくなる距離
    // 0.0 ~ 1.0
    float fogDistanceLength = 10000.0f;

    // 距離による霧の、100%に近づいていく速さ
    // 0.0だったら線形補間で100%に近づいていく
    // 1.0だったらちょっと離れただけで99%になってしまう
    float fogDistanceSpeed = 0.0f;

    // 距離による霧の色
    // 昼は青、夕方はオレンジ、夜は青
    DWORD fogDistanceColor = 0x7f7fffff;

    //--------------------------------------------------------
    // 高さフォグ
    //--------------------------------------------------------

    // 高さによる霧を表示するか
    bool fogHeight = false;

    // 角の法線を再計算するか
    //
    // 例えば半球を表示する場合、法線を再計算するとキノコのようになり、
    // 再計算しないとダイヤモンドのような見た目になる。
    // 視差遮蔽マッピングを行う場合、スムーズ化を行うと角ばっている部分で表示がおかしくなる。
    // 視差遮蔽マッピングを行う場合、スムーズ化はオフにした方がいいけど、できないわけではない。
    bool smooth = false;

    // 影が表示されるようにするかどうか。
    bool shadow = false;

    // 彩度影の表示を行うか。
    //
    // 陰を表示するとき、単純に輝度を下げる、というのは昔のゲーム画面風の
    // 見た目になる。綺麗ではない。
    // 陰を表示するときに、輝度を少しだけ下げて、彩度を上げ、色相を少し変えると
    // 良い感じになる。
    bool saturateShadow = true;

    // 彩度影をどれくらい強く効かせるか
    float saturateShadowIntensity = 1.2f;

    // 陰の暗さをどれくらい強く効かせるか
    // 0.0なら陰を暗くしない、1.0なら通常のランバート
    float shadowDarkness = 1.0f;

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

    // 視差遮蔽マッピングを行うか
    // デコボコが本当にあるように見せる
    // 視差遮蔽マッピングを行うが、法線マッピングを行わない、ということは理論上可能。
    bool parallaxOcclusionMapping = false;

    // 法線マッピングを行うか
    // デコボコがあるように見せる。近くで見ると平面だとすぐにわかってしまう。
    bool normalMapping = false;

    // ガラスか
    bool glass = false;

    // 波
    //
    // 海面のような波
    bool wave = false;

    // 波の強さ
    // TODO 大きさと速さは分けるべき？
    float waveIntensity = 0.1f;

    // 揺らすか
    //
    // 草を風でそよがせたい時に使う
    // 波はモデルをぐちゃぐちゃに変形させるが、こちらは少し、しならせるだけ。
    bool sway = false;

    // そよぐ強さ
    // TODO 大きさと速さは分けるべき？
    float swayIntensity = 0.1f;

    // ポイントライトで照らされるか
    // 
    // ポイントライトがあったら周りの物体が少し明るくなる。
    // 近くにポイントライトがあったら明るくなる、という処理は周りの物体が各々行う必要がある。
    bool pointLight = true;

    // SSAOを有効にするか
    bool ssao = false;

    // 衝突判定の対象とするか
    bool collision = false;

    // 衝突判定が開始される半径
    //
    // 衝突判定はとてつもなく重い。この半径以内に入るまで衝突判定をしない、という距離
    // 高速化のため、球ではなく立方体で見る
    float collisionRadius = 2.0f;

    // サブサーフェイススキャッタリングを有効にする
    // もやし、大理石、ロウソク、すりガラスなどの
    // 光がちょっと透ける物体用。
    bool sss = false;

    float sssIntensity = 0.0f;

    // SSSで表示される色を自ら指定？
    // 自動化できないのだろうか
    // 緑の物体でSSSをやる場合、透けた色を黄色で表示すると綺麗な見た目になる
    // なぜかはよくわからない。
    DWORD sssColor = 0xffff80;
};

enum class eMeshParamPreset
{
    // 木
    TREE,

    // 草
    GRASS,

    // 石
    STONE,

    // 鏡
    MIRROR,

    // ガラス
    GLASS,

    // 人の肌
    SKIN,

    // 髪
    HAIR,

    // 静かな海面
    WAVE,

    // 布
    CLOTH,

    // 金属
    METAL,

    // ゴム
    RUBBER
};

stMeshParam GetMeshParamPreset(const eMeshParamPreset preset);

// ポイントライト、スムーズ、彩度影、法線マッピング、視差遮蔽マッピング、SSAO、深度バッファシャドウ
// マルチレンダーターゲット、環境マッピングが有効なメッシュクラス
class MeshMix : public IDeviceResettable
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
    void SetSaturateShadow(const bool enabled);
    void SetSaturateShadowIntensity(const float intensity);
    void SetShadowDarkness(const float darkness);
    void SetSpecularIntensity(const float intensity);
    void SetSpecularEdge(const float edge);

    D3DXVECTOR3 GetPos() const;

    void SetRotY(const float rotY);

    D3DXVECTOR3 GetRot() const;

    float GetScale() const;
    DWORD GetSubsetCount() const;
    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    LPD3DXMESH GetD3DMesh() const;

    float GetRadius() const;

    std::wstring GetMeshName();

    // 解像度やウィンドウモードを変更したときのための関数
    void OnDeviceLost();
    void OnDeviceReset();

private:

    //const std::wstring SHADER_FILENAME = L"res\\shader\\MeshMix.fx";
    const std::wstring SHADER_FILENAME = L".\\MeshMix.cso";
    std::wstring m_meshName;

    LPD3DXEFFECT m_D3DEffect = nullptr;
    LPD3DXMESH m_D3DMesh = nullptr;

    DWORD m_materialCount = 0;
    DWORD m_subsetCount = 0;
    std::vector<D3DXVECTOR4> m_vecDiffuse;
    std::vector<LPDIRECT3DBASETEXTURE9> m_vecTexture;
    LPDIRECT3DBASETEXTURE9 m_texCubeMap = nullptr;
    LPDIRECT3DBASETEXTURE9 m_texNormalMap = nullptr;
    LPDIRECT3DBASETEXTURE9 m_texHeightMap = nullptr;

    D3DXVECTOR3 m_pos = D3DXVECTOR3(0.f, 0.f, 0.f);
    D3DXVECTOR3 m_rotate = D3DXVECTOR3(0.f, 0.f, 0.f);

    //-------------------------------------------------
    // この物体の半径
    // プレイヤーがこの半径以内に近づいたらこの物体は衝突判定の対象となる
    // -1だったら必ず衝突判定の対象にする
    //-------------------------------------------------

    float m_scale = 0.0f;

    bool m_bLoaded = false;
    bool m_enabled = true;

    stMeshParam m_param;

    void ModifyMeshForNormalMapping(LPD3DXMESH& pMesh);
    D3DXVECTOR4 GetSubsetDiffuse(const DWORD subsetIndex) const;
    LPDIRECT3DBASETEXTURE9 GetSubsetTexture(const DWORD subsetIndex) const;
};
}

