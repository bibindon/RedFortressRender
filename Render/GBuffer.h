#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Common.h"

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

struct GBufferFrameProfile
{
    double frontMilliseconds = 0.0;
    double thicknessMilliseconds = 0.0;
    unsigned int frontStaticSubsetDraws = 0;
    unsigned int thicknessStaticSubsetDraws = 0;
    unsigned int frontObjectDraws = 0;
    unsigned int thicknessObjectDraws = 0;
};

class IMeshMixSkinAnim;
class MeshMixAnimNoBone2;
class MeshMix2;
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
    void SetThicknessFormat(GBufferScalarFormat format);
    void SetBackDepthFormat(GBufferScalarFormat format);
    GBufferScalarFormat GetDepthFormat() const;
    GBufferScalarFormat GetFogDepthFormat() const;
    GBufferVectorFormat GetPositionFormat() const;
    GBufferVectorFormat GetNormalFormat() const;
    GBufferScalarFormat GetThicknessFormat() const;
    GBufferScalarFormat GetBackDepthFormat() const;
    bool IsInitialized() const;
    const GBufferFrameProfile& GetLastFrameProfile() const;
    void BindIntegratedRenderTargets();
    void UnbindIntegratedRenderTargets();
    static void ApplyIntegratedEffectParameters(LPD3DXEFFECT effect,
                                                bool shadowReceiverEnabled);

    void Draw(const std::vector<IMeshMixSkinAnim*>& meshMixSkinAnimList,
              const std::vector<MeshMixAnimNoBone2*>& meshMixAnimNoBone2List,
              ParticleSystem* particleSystem,
              bool frontBackfaceCullingEnabled,
              LPDIRECT3DTEXTURE9* Z,
              LPDIRECT3DTEXTURE9* CameraZ,
              LPDIRECT3DTEXTURE9* Pos,
              LPDIRECT3DTEXTURE9* Normal,
              LPDIRECT3DTEXTURE9* Thickness,
              LPDIRECT3DTEXTURE9* BackDepth);
    void DrawThickness(const std::vector<IMeshMixSkinAnim*>& meshMixSkinAnimList,
                       const std::vector<MeshMixAnimNoBone2*>& meshMixAnimNoBone2List,
                       const std::vector<MeshMix2*>& meshMix2List,
                       bool generateBackDepth);

    void Finalize();

    void OnDeviceLost();
    void OnDeviceReset();

private:

    LPDIRECT3DTEXTURE9 m_texRenderTargetZ = NULL;
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
    GBufferScalarFormat m_thicknessFormat = GBufferScalarFormat::R16F;
    GBufferScalarFormat m_backDepthFormat = GBufferScalarFormat::R16F;
    bool m_isInitialized = false;
    bool m_isRegisteredForDeviceReset = false;
    GBufferFrameProfile m_lastFrameProfile;

    void CreateRawResource();
    static D3DFORMAT ToD3DFormat(GBufferScalarFormat format);
    static D3DFORMAT ToD3DFormat(GBufferVectorFormat format);
    D3DFORMAT GetPackedDepthFormat() const;
};

}


