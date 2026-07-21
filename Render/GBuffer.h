#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include "Common.h"
#include "MeshMixManager.h"

namespace NSRender
{
enum class GBufferScalarFormat
{
    R32F,
    R16F,
};

enum class GBufferVectorFormat
{
    A16B16G16R16F,
    A8B8G8R8,
};

class IMeshMixSkinAnim;
class MeshMixAnimNoBone;
class MeshMix2;
class MeshInstancing;
class MeshInstancing2;
class ParticleSystem;

// 各ピクセルの深度とワールド座標を表した画像を生成
class GBuffer : public IDeviceResettable
{

public:

    static float ComputePositionRange(const float nearPlane, const float farPlane);

    void Initialize();
    void SetDepthRange(const float nearPlane, const float farPlane);
    void SetFogDepthRange(const float nearPlane, const float farPlane);
    void SetDepthFormat(GBufferScalarFormat format);
    void SetFogDepthFormat(GBufferScalarFormat format);
    void SetPositionFormat(GBufferVectorFormat format);
    void SetNormalFormat(GBufferVectorFormat format);
    void SetThicknessFormat(GBufferVectorFormat format);
    void SetBackDepthFormat(GBufferScalarFormat format);
    GBufferScalarFormat GetDepthFormat() const;
    GBufferScalarFormat GetFogDepthFormat() const;
    GBufferVectorFormat GetPositionFormat() const;
    GBufferVectorFormat GetNormalFormat() const;
    GBufferVectorFormat GetThicknessFormat() const;
    GBufferScalarFormat GetBackDepthFormat() const;
    bool IsInitialized() const;

    void Draw(const std::deque<MeshMixManager>& meshList,
              const std::vector<IMeshMixSkinAnim*>& meshMixSkinAnimList,
              const std::vector<MeshMixAnimNoBone*>& meshMixAnimNoBoneList,
              const std::vector<MeshMix2*>& meshMix2List,
              const std::unordered_map<std::wstring, MeshInstancing*>& meshInstancingMap,
              const std::unordered_map<std::wstring, MeshInstancing2*>& meshInstancing2Map,
              ParticleSystem* particleSystem,
              bool meshMixManagerShadowReceiverEnabled,
              bool generateBackDepth,
              LPDIRECT3DTEXTURE9* Z,
              LPDIRECT3DTEXTURE9* CameraZ,
              LPDIRECT3DTEXTURE9* Pos,
              LPDIRECT3DTEXTURE9* Normal,
              LPDIRECT3DTEXTURE9* Thickness,
              LPDIRECT3DTEXTURE9* BackDepth);

    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPDIRECT3DTEXTURE9 m_texRenderTargetZ = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetFogZ = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetPos = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetNormal = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetThickness = NULL;
    LPDIRECT3DTEXTURE9 m_texRenderTargetBackDepth = NULL;
    LPDIRECT3DSURFACE9 m_surfaceThicknessDepthStencil = NULL;

    LPD3DXEFFECT m_fxGBuffer = NULL;
    float m_nearPlane = 0.1f;
    float m_farPlane = 30'000.0f;
    float m_fogNearPlane = 0.1f;
    float m_fogFarPlane = 30'000.0f;
    float m_positionRange = 30'000.0f;
    GBufferScalarFormat m_depthFormat = GBufferScalarFormat::R16F;
    GBufferScalarFormat m_fogDepthFormat = GBufferScalarFormat::R16F;
    GBufferVectorFormat m_positionFormat = GBufferVectorFormat::A8B8G8R8;
    GBufferVectorFormat m_normalFormat = GBufferVectorFormat::A8B8G8R8;
    GBufferVectorFormat m_thicknessFormat = GBufferVectorFormat::A8B8G8R8;
    GBufferScalarFormat m_backDepthFormat = GBufferScalarFormat::R16F;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;

    void CreateRawResource();
    static D3DFORMAT ToD3DFormat(GBufferScalarFormat format);
    static D3DFORMAT ToD3DFormat(GBufferVectorFormat format);
};

}


