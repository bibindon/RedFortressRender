#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include "Common.h"
#include "MeshMixManager.h"

namespace NSRender
{
class MeshMixSkinAnim;
class MeshInstancing;
class ParticleSystem;

// 各ピクセルの深度とワールド座標を表した画像を生成
class GBuffer : public IDeviceResettable
{

public:

    static float ComputePositionRange(const float nearPlane, const float farPlane);

    void Initialize();
    void SetDepthRange(const float nearPlane, const float farPlane);
    void SetFogDepthRange(const float nearPlane, const float farPlane);
    bool IsInitialized() const;

    void Draw(const std::deque<MeshMixManager>& meshList,
              const std::vector<MeshMixSkinAnim*>& meshMixSkinAnimList,
              const std::unordered_map<std::wstring, MeshInstancing*>& meshInstancingMap,
              ParticleSystem* particleSystem,
              LPDIRECT3DTEXTURE9* Z,
              LPDIRECT3DTEXTURE9* CameraZ,
              LPDIRECT3DTEXTURE9* Pos,
              LPDIRECT3DTEXTURE9* Normal,
              LPDIRECT3DTEXTURE9* Thickness);

    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPDIRECT3DTEXTURE9 m_texRenderTargetZ = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetFogZ = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetPos = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetNormal = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetThickness = NULL;

    LPD3DXEFFECT m_fxGBuffer = NULL;
    float m_nearPlane = 0.1f;
    float m_farPlane = 30'000.0f;
    float m_fogNearPlane = 0.1f;
    float m_fogFarPlane = 30'000.0f;
    float m_positionRange = 30'000.0f;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    void CreateRawResource();
};

}

