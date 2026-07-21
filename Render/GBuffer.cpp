
#include "GBuffer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Common.h"
#include "Camera.h"
#include "MeshInstancing.h"
#include "MeshInstancing2.h"
#include "MeshMixAnimNoBone.h"
#include "MeshMix2.h"
#include "MeshMixSkinAnim.h"
#include "ParticleSystem.h"

#include "Util.h"

namespace NSRender
{

float GBuffer::ComputePositionRange(const float nearPlane, const float farPlane)
{
    const float absoluteNear = fabsf(nearPlane);
    const float absoluteFar = fabsf(farPlane);
    return (std::max)(1.0f, (std::max)(absoluteNear, absoluteFar));
}

void GBuffer::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    HRESULT hResult = E_FAIL;

    const std::wstring effectPath = Util::GetExeDir() + L"GBuffer.cso";
    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                  effectPath.c_str(),
                                  NULL,
                                  NULL,
                                  //D3DXSHADER_DEBUG,
                                  0,
                                  NULL,
                                  &m_fxGBuffer,
                                  NULL);
    assert(hResult == S_OK);

    D3DCAPS9 deviceCaps{};
    hResult = Common::D3DDevice()->GetDeviceCaps(&deviceCaps);
    if (FAILED(hResult))
    {
        throw std::runtime_error("Failed to query DirectX 9 device capabilities for GBuffer.");
    }
    if (deviceCaps.NumSimultaneousRTs < 4)
    {
        throw std::runtime_error("GBuffer requires four simultaneous render targets.");
    }

    CreateRawResource();

    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }

    m_isInitialized = true;
}

void GBuffer::SetDepthRange(const float nearPlane, const float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_positionRange = ComputePositionRange(nearPlane, farPlane);
}

void GBuffer::SetFogDepthRange(const float nearPlane, const float farPlane)
{
    m_fogNearPlane = nearPlane;
    m_fogFarPlane = farPlane;
}

void GBuffer::SetDepthFormat(const GBufferScalarFormat format)
{
    m_depthFormat = format;
}

void GBuffer::SetFogDepthFormat(const GBufferScalarFormat format)
{
    m_fogDepthFormat = format;
}

void GBuffer::SetPositionFormat(const GBufferVectorFormat format)
{
    m_positionFormat = format;
}

void GBuffer::SetNormalFormat(const GBufferVectorFormat format)
{
    m_normalFormat = format;
}

void GBuffer::SetThicknessFormat(const GBufferVectorFormat format)
{
    m_thicknessFormat = format;
}

void GBuffer::SetBackDepthFormat(const GBufferScalarFormat format)
{
    m_backDepthFormat = format;
}

GBufferScalarFormat GBuffer::GetDepthFormat() const
{
    return m_depthFormat;
}

GBufferScalarFormat GBuffer::GetFogDepthFormat() const
{
    return m_fogDepthFormat;
}

GBufferVectorFormat GBuffer::GetPositionFormat() const
{
    return m_positionFormat;
}

GBufferVectorFormat GBuffer::GetNormalFormat() const
{
    return m_normalFormat;
}

GBufferVectorFormat GBuffer::GetThicknessFormat() const
{
    return m_thicknessFormat;
}

GBufferScalarFormat GBuffer::GetBackDepthFormat() const
{
    return m_backDepthFormat;
}

D3DFORMAT GBuffer::ToD3DFormat(const GBufferScalarFormat format)
{
    if (format == GBufferScalarFormat::R16F)
    {
        return D3DFMT_R16F;
    }

    return D3DFMT_R32F;
}

D3DFORMAT GBuffer::ToD3DFormat(const GBufferVectorFormat format)
{
    if (format == GBufferVectorFormat::A8B8G8R8)
    {
        return D3DFMT_A8B8G8R8;
    }

    return D3DFMT_A16B16G16R16F;
}

bool GBuffer::IsInitialized() const
{
    return m_isInitialized;
}

void GBuffer::CreateRawResource()
{
    HRESULT hResult = E_FAIL;

    // Z画像
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                ToD3DFormat(m_depthFormat),
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetZ);
    assert(hResult == S_OK);

    // Fog 専用 Z 画像
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                ToD3DFormat(m_fogDepthFormat),
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetFogZ);
    assert(hResult == S_OK);

    // World座標
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                ToD3DFormat(m_positionFormat),
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetPos);
    assert(hResult == S_OK);

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                ToD3DFormat(m_normalFormat),
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetNormal);
    assert(hResult == S_OK);

    // 厚み情報（バックフェイス深度）
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                ToD3DFormat(m_thicknessFormat),
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetThickness);
    assert(hResult == S_OK);

    // バックフェイスパスで実際に描いた線形深度
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                ToD3DFormat(m_backDepthFormat),
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetBackDepth);
    assert(hResult == S_OK);
}

void GBuffer::Draw(const std::deque<MeshMixManager>& meshList,
                   const std::vector<IMeshMixSkinAnim*>& meshMixSkinAnimList,
                   const std::vector<MeshMixAnimNoBone*>& meshMixAnimNoBoneList,
                   const std::vector<MeshMix2*>& meshMix2List,
                   const std::unordered_map<std::wstring, MeshInstancing*>& meshInstancingMap,
                   const std::unordered_map<std::wstring, MeshInstancing2*>& meshInstancing2Map,
                   ParticleSystem* particleSystem,
                   LPDIRECT3DTEXTURE9* Z,
                   LPDIRECT3DTEXTURE9* CameraZ,
                   LPDIRECT3DTEXTURE9* Pos,
                   LPDIRECT3DTEXTURE9* Normal,
                   LPDIRECT3DTEXTURE9* Thickness,
                   LPDIRECT3DTEXTURE9* BackDepth)
{
    HRESULT hr = E_FAIL;

    // 既存の RT0 を退避
    LPDIRECT3DSURFACE9 surfaceOld = NULL;
    hr = Common::D3DDevice()->GetRenderTarget(0, &surfaceOld);

    // GBuffer の各サーフェスを取得
    LPDIRECT3DSURFACE9 surfaceZ = NULL;
    LPDIRECT3DSURFACE9 surfaceFogZ = NULL;
    LPDIRECT3DSURFACE9 surfacePos = NULL;
    LPDIRECT3DSURFACE9 surfaceNorm = NULL;

    hr = m_texRenderTargetZ->GetSurfaceLevel(0, &surfaceZ);
    hr = m_texRenderTargetFogZ->GetSurfaceLevel(0, &surfaceFogZ);
    hr = m_texRenderTargetPos->GetSurfaceLevel(0, &surfacePos);
    hr = m_texRenderTargetNormal->GetSurfaceLevel(0, &surfaceNorm);

    // 通常深度と Fog 深度は同じカメラから別の距離範囲でエンコードする。
    // RT3 へ Fog 深度を同時出力し、専用のシーン再描画を不要にする。
    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceZ);
    hr = Common::D3DDevice()->SetRenderTarget(1, surfacePos);
    hr = Common::D3DDevice()->SetRenderTarget(2, surfaceNorm);
    hr = Common::D3DDevice()->SetRenderTarget(3, surfaceFogZ);
    if (FAILED(hr))
    {
        SAFE_RELEASE(surfaceZ);
        SAFE_RELEASE(surfaceFogZ);
        SAFE_RELEASE(surfacePos);
        SAFE_RELEASE(surfaceNorm);
        SAFE_RELEASE(surfaceOld);
        throw std::runtime_error("Failed to bind the fourth GBuffer render target.");
    }

    // クリア。Zバッファも一緒に初期化して素直に全描画
    hr = Common::D3DDevice()->Clear(0,
                                    NULL,
                                    D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                    D3DCOLOR_XRGB(255, 0, 0),
                                    1.0f,
                                    0);

    hr = Common::D3DDevice()->BeginScene();

    // ここで「不透明物体のみ」を GBuffer.fx で描く
    auto mView = Camera::GetViewMatrix();
    auto mProj = Camera::GetProjMatrix();
    D3DXMATRIX viewProjectionMatrix = mView * mProj;

    m_fxGBuffer->SetMatrix("g_matView",  &mView);
    m_fxGBuffer->SetMatrix("g_matProj",  &mProj);
    m_fxGBuffer->SetFloat("g_fNear", m_nearPlane);
    m_fxGBuffer->SetFloat("g_fFar",  m_farPlane);
    m_fxGBuffer->SetFloat("g_fogNear", m_fogNearPlane);
    m_fxGBuffer->SetFloat("g_fogFar", m_fogFarPlane);
    m_fxGBuffer->SetFloat("g_posRange", m_positionRange);

    for (auto& mesh : meshList)
    {
        if (!mesh.IsEnabled())
        {
            continue;
        }

        if (!mesh.IsLoaded())
        {
            continue;
        }

        if (!mesh.IsSsaoEnabled())
        {
            continue;
        }

        // 必要な定数の投入
        const D3DXMATRIX matWorld = mesh.GetWorldMatrix();

        m_fxGBuffer->SetMatrix("g_matWorld", &matWorld);
        m_fxGBuffer->SetTechnique("TechniqueGBuffer");
        m_fxGBuffer->Begin(NULL, 0);
        m_fxGBuffer->BeginPass(0);

        // subset ごとに描画
        LPD3DXMESH d3dMesh = mesh.GetD3DMesh();
        DWORD subsetCount = 1;
        if (mesh.GetSubsetCount() > 0)
        {
            subsetCount = mesh.GetSubsetCount();
        }
        for (DWORD subsetIndex = 0; subsetIndex < subsetCount; ++subsetIndex)
        {
            d3dMesh->DrawSubset(subsetIndex);
        }

        m_fxGBuffer->EndPass();
        m_fxGBuffer->End();
    }

    m_fxGBuffer->SetTechnique("TechniqueGBufferSkin");
    for (auto& mesh : meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(m_fxGBuffer);
        }
    }

    m_fxGBuffer->SetTechnique("TechniqueGBuffer");
    for (auto& mesh : meshMixAnimNoBoneList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(m_fxGBuffer, viewProjectionMatrix);
        }
    }

    for (auto& mesh : meshMix2List)
    {
        if (mesh != nullptr && mesh->IsSsaoEnabled())
        {
            mesh->RenderToEffect(m_fxGBuffer, viewProjectionMatrix);
        }
    }

    for (const auto& mesh : meshInstancingMap)
    {
        if (mesh.second != nullptr)
        {
            mesh.second->RenderToGBufferEffect(m_fxGBuffer, "TechniqueGBufferInstancing");
        }
    }

    for (const auto& mesh : meshInstancing2Map)
    {
        if (mesh.second != nullptr)
        {
            mesh.second->RenderToGBufferEffect(m_fxGBuffer, "TechniqueGBufferInstancing");
        }
    }

    if (particleSystem != nullptr)
    {
        particleSystem->RenderDustToGBufferEffect(m_fxGBuffer,
                                                  mView,
                                                  mProj,
                                                  "TechniqueGBufferParticle");
    }

    hr = Common::D3DDevice()->EndScene();

    // MRT を外し、RT0 を元に戻す
    Common::D3DDevice()->SetRenderTarget(3, NULL);
    Common::D3DDevice()->SetRenderTarget(2, NULL);
    Common::D3DDevice()->SetRenderTarget(1, NULL);
    Common::D3DDevice()->SetRenderTarget(0, surfaceOld);

    SAFE_RELEASE(surfaceZ);
    SAFE_RELEASE(surfaceFogZ);
    SAFE_RELEASE(surfacePos);
    SAFE_RELEASE(surfaceNorm);
    SAFE_RELEASE(surfaceOld);

    // --- 厚みパス（バックフェイス深度）---
    LPDIRECT3DSURFACE9 surfaceThickness = NULL;
    LPDIRECT3DSURFACE9 surfaceBackDepth = NULL;
    m_texRenderTargetThickness->GetSurfaceLevel(0, &surfaceThickness);
    m_texRenderTargetBackDepth->GetSurfaceLevel(0, &surfaceBackDepth);

    LPDIRECT3DSURFACE9 surfaceOld2 = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &surfaceOld2);
    Common::D3DDevice()->SetRenderTarget(0, surfaceThickness);
    Common::D3DDevice()->SetRenderTarget(1, surfaceBackDepth);

    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                               D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);
    Common::D3DDevice()->BeginScene();

    m_fxGBuffer->SetTexture("g_texFrontDepth", m_texRenderTargetZ);

    for (auto& mesh : meshList)
    {
        if (!mesh.IsEnabled())
        {
            continue;
        }

        if (!mesh.IsLoaded())
        {
            continue;
        }

        if (!mesh.IsSsaoEnabled())
        {
            continue;
        }

        const D3DXMATRIX matWorld = mesh.GetWorldMatrix();

        m_fxGBuffer->SetMatrix("g_matWorld", &matWorld);
        m_fxGBuffer->SetTechnique("TechniqueGBufferBackFace");
        m_fxGBuffer->Begin(NULL, 0);
        m_fxGBuffer->BeginPass(0);

        LPD3DXMESH d3dMesh = mesh.GetD3DMesh();
        DWORD subsetCount = 1;
        if (mesh.GetSubsetCount() > 0)
        {
            subsetCount = mesh.GetSubsetCount();
        }
        for (DWORD subsetIndex = 0; subsetIndex < subsetCount; ++subsetIndex)
        {
            d3dMesh->DrawSubset(subsetIndex);
        }

        m_fxGBuffer->EndPass();
        m_fxGBuffer->End();
    }

    m_fxGBuffer->SetTechnique("TechniqueGBufferSkinBackFace");
    for (auto& mesh : meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(m_fxGBuffer);
        }
    }

    m_fxGBuffer->SetTechnique("TechniqueGBufferBackFace");
    for (auto& mesh : meshMixAnimNoBoneList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(m_fxGBuffer, viewProjectionMatrix);
        }
    }

    for (auto& mesh : meshMix2List)
    {
        if (mesh != nullptr && mesh->IsSsaoEnabled())
        {
            mesh->RenderToEffect(m_fxGBuffer, viewProjectionMatrix);
        }
    }

    Common::D3DDevice()->EndScene();
    Common::D3DDevice()->SetRenderTarget(1, NULL);
    Common::D3DDevice()->SetRenderTarget(0, surfaceOld2);
    SAFE_RELEASE(surfaceBackDepth);
    SAFE_RELEASE(surfaceThickness);
    SAFE_RELEASE(surfaceOld2);

    *Z = m_texRenderTargetZ;
    *CameraZ = m_texRenderTargetFogZ;
    *Pos = m_texRenderTargetPos;
    *Normal = m_texRenderTargetNormal;
    *Thickness = m_texRenderTargetThickness;
    *BackDepth = m_texRenderTargetBackDepth;
}

void GBuffer::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }

    SAFE_RELEASE(m_fxGBuffer);
    SAFE_RELEASE(m_texRenderTargetZ);
    SAFE_RELEASE(m_texRenderTargetFogZ);
    SAFE_RELEASE(m_texRenderTargetPos);
    SAFE_RELEASE(m_texRenderTargetNormal);
    SAFE_RELEASE(m_texRenderTargetThickness);
    SAFE_RELEASE(m_texRenderTargetBackDepth);

    m_isInitialized = false;
}

void GBuffer::OnDeviceLost()
{
    if (!m_isInitialized || m_fxGBuffer == NULL)
    {
        return;
    }

    m_fxGBuffer->OnLostDevice();
    SAFE_RELEASE(m_texRenderTargetZ);
    SAFE_RELEASE(m_texRenderTargetFogZ);
    SAFE_RELEASE(m_texRenderTargetPos);
    SAFE_RELEASE(m_texRenderTargetNormal);
    SAFE_RELEASE(m_texRenderTargetThickness);
    SAFE_RELEASE(m_texRenderTargetBackDepth);
}

void GBuffer::OnDeviceReset()
{
    if (!m_isInitialized || m_fxGBuffer == NULL)
    {
        return;
    }

    CreateRawResource();
    m_fxGBuffer->OnResetDevice();
}

}

