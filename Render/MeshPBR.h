#pragma once
#include "Common.h"

namespace NSRender
{

struct stMeshPBRParam
{
    bool ambient = true;
    DWORD ambientColor = 0x101010ff;
    DWORD specularColor = 0xffffffff;
    float specularIntensity = 0.0f;
    float specularEdge = 0.0f;
    bool specularIntensityOverrideEnabled = false;
    bool specularEdgeOverrideEnabled = false;
    bool fogDistance = true;
    float fogDistanceLength = 10000.0f;
    float fogDistanceSpeed = 0.0f;
    DWORD fogDistanceColor = 0x7f7fffff;
    bool fogHeight = false;
    bool smooth = false;
    bool shadow = true;
    bool saturateShadow = true;
    float saturateShadowIntensity = 1.2f;
    float shadowDarkness = 1.0f;
    bool cubeMapping = false;
    float cubeMappingRate = 1.0f;
    float cubeMappingGauss = 0.0f;
    bool parallaxOcclusionMapping = false;
    bool normalMapping = false;
    bool glass = false;
    bool mirror = false;
    bool emit = false;
    float emitIntensity = 1.0f;
    DWORD emitColor = 0x00ffffff;
    bool wave = false;
    float waveIntensity = 0.1f;
    bool sway = false;
    float swayIntensity = 0.1f;
    bool pointLight = true;
    bool ssao = true;
    bool collision = false;
    float collisionRadius = 2.0f;
    bool sss = false;
    float sssIntensity = 1.0f;
    DWORD sssColor = 0xffff80;
    D3DXCOLOR pbrBaseColorFactor = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
    float pbrRoughness = 0.85f;
    float pbrMetallic = 0.0f;
    bool pbrEnableSrgbToLinear = true;
    bool pbrEnableLinearToSrgb = true;
    float envReflectionIntensity = 0.05f;
    float envMaxMipLevel = 5.0f;
    float envDiffuseIntensity = 0.8f;
    float envDiffuseMipLevel = 3.0f;
    std::wstring envMapTexturePath;
};

enum class eMeshPBRParamPreset
{
    TREE,
    GRASS,
    STONE,
    MIRROR,
    GLASS,
    SKIN,
    HAIR,
    WAVE,
    CLOTH,
    METAL,
    RUBBER
};

stMeshPBRParam GetMeshPBRParamPreset(eMeshPBRParamPreset preset);

// MeshMix を元にした PBR 用の土台クラス。
class MeshPBR : public IDeviceResettable
{

public:

    MeshPBR(const std::wstring& filename,
            const D3DXVECTOR3& pos,
            const D3DXVECTOR3& rotate,
            const float scale,
            const stMeshPBRParam& param
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
    void SetSSS(const bool enabled);
    void SetSSSIntensity(const float intensity);
    void SetSSSColor(const DWORD color);

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
    const std::wstring SHADER_FILENAME = L".\\MeshPBR.cso";
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

    stMeshPBRParam m_param;

    void ModifyMeshForNormalMapping(LPD3DXMESH& pMesh);
    D3DXVECTOR4 GetSubsetDiffuse(const DWORD subsetIndex) const;
    LPDIRECT3DBASETEXTURE9 GetSubsetTexture(const DWORD subsetIndex) const;
};
}

