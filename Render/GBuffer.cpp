
#include "GBuffer.h"

#include <algorithm>
#include <cmath>

#include "Common.h"
#include "Camera.h"
#include "MeshInstancing.h"
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
                                D3DFMT_R32F,
                                //D3DFMT_R16F,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetZ);
    assert(hResult == S_OK);

    // Fog 専用 Z 画像
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_R32F,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetFogZ);
    assert(hResult == S_OK);

    // World座標
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                //D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetPos);
    assert(hResult == S_OK);

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                //D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetNormal);
    assert(hResult == S_OK);

    // 厚み情報（バックフェイス深度）
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                //D3DFMT_A8R8G8B8,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetThickness);
    assert(hResult == S_OK);

    // バックフェイスパスで実際に描いた線形深度
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_R32F,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetBackDepth);
    assert(hResult == S_OK);
}

void GBuffer::Draw(const std::deque<MeshMixManager>& meshList,
                   const std::vector<MeshMixSkinAnim*>& meshMixSkinAnimList,
                   const std::unordered_map<std::wstring, MeshInstancing*>& meshInstancingMap,
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

    // Z と POS のサーフェスを取得
    LPDIRECT3DSURFACE9 surfaceZ = NULL;
    LPDIRECT3DSURFACE9 surfacePos = NULL;
    LPDIRECT3DSURFACE9 surfaceNorm = NULL;

    hr = m_texRenderTargetZ->GetSurfaceLevel(0, &surfaceZ);
    hr = m_texRenderTargetPos->GetSurfaceLevel(0, &surfacePos);
    hr = m_texRenderTargetNormal->GetSurfaceLevel(0, &surfaceNorm);

    // MRT×2 をセット
    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceZ);
    hr = Common::D3DDevice()->SetRenderTarget(1, surfacePos);
    hr = Common::D3DDevice()->SetRenderTarget(2, surfaceNorm);

    // クリア。Zバッファも一緒に初期化して素直に全描画
    hr = Common::D3DDevice()->Clear(0,
                                    NULL,
                                    D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                    D3DCOLOR_RGBA(0, 0, 0, 0),
                                    1.0f,
                                    0);

    hr = Common::D3DDevice()->BeginScene();

    // ここで「不透明物体のみ」を GBuffer.fx で描く
    auto mView = Camera::GetViewMatrix();
    auto mProj = Camera::GetProjMatrix();

    m_fxGBuffer->SetMatrix("g_matView",  &mView);
    m_fxGBuffer->SetMatrix("g_matProj",  &mProj);
    m_fxGBuffer->SetFloat("g_fNear", m_nearPlane);
    m_fxGBuffer->SetFloat("g_fFar",  m_farPlane);
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
        D3DXMATRIX matWorld;
        D3DXMatrixIdentity(&matWorld);
        {
            D3DXMATRIX m;
            D3DXMatrixIdentity(&m);
            D3DXMatrixScaling(&m, mesh.GetScale(), mesh.GetScale(), mesh.GetScale());
            matWorld *= m;
            D3DXMatrixRotationYawPitchRoll(&m, mesh.GetRot().y, mesh.GetRot().x, mesh.GetRot().z);
            matWorld *= m;
            D3DXVECTOR3 p = mesh.GetPos();
            D3DXMatrixTranslation(&m, p.x, p.y, p.z);
            matWorld *= m;
        }

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

    for (const auto& mesh : meshInstancingMap)
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
    Common::D3DDevice()->SetRenderTarget(2, NULL);
    Common::D3DDevice()->SetRenderTarget(1, NULL);
    Common::D3DDevice()->SetRenderTarget(0, surfaceOld);

    SAFE_RELEASE(surfaceZ);
    SAFE_RELEASE(surfacePos);
    SAFE_RELEASE(surfaceNorm);
    SAFE_RELEASE(surfaceOld);

    // --- Fog 専用深度パス ---
    LPDIRECT3DSURFACE9 surfaceFogZ = NULL;
    m_texRenderTargetFogZ->GetSurfaceLevel(0, &surfaceFogZ);

    LPDIRECT3DSURFACE9 surfaceOldFog = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &surfaceOldFog);
    Common::D3DDevice()->SetRenderTarget(0, surfaceFogZ);

    Common::D3DDevice()->Clear(0,
                               NULL,
                               D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                               D3DCOLOR_XRGB(255, 255, 255),
                               1.0f,
                               0);
    Common::D3DDevice()->BeginScene();

    m_fxGBuffer->SetFloat("g_fNear", m_fogNearPlane);
    m_fxGBuffer->SetFloat("g_fFar", m_fogFarPlane);

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

        D3DXMATRIX matWorld;
        D3DXMatrixIdentity(&matWorld);
        {
            D3DXMATRIX m;
            D3DXMatrixIdentity(&m);
            D3DXMatrixScaling(&m, mesh.GetScale(), mesh.GetScale(), mesh.GetScale());
            matWorld *= m;
            D3DXMatrixRotationYawPitchRoll(&m, mesh.GetRot().y, mesh.GetRot().x, mesh.GetRot().z);
            matWorld *= m;
            D3DXVECTOR3 p = mesh.GetPos();
            D3DXMatrixTranslation(&m, p.x, p.y, p.z);
            matWorld *= m;
        }

        m_fxGBuffer->SetMatrix("g_matWorld", &matWorld);
        m_fxGBuffer->SetTechnique("TechniqueGBuffer");
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

    m_fxGBuffer->SetTechnique("TechniqueGBufferSkin");
    for (auto& mesh : meshMixSkinAnimList)
    {
        if (mesh != nullptr)
        {
            mesh->RenderToEffect(m_fxGBuffer);
        }
    }

    for (const auto& mesh : meshInstancingMap)
    {
        if (mesh.second != nullptr)
        {
            mesh.second->RenderToGBufferEffect(m_fxGBuffer, "TechniqueGBufferInstancingFog");
        }
    }

    if (particleSystem != nullptr)
    {
        particleSystem->RenderDustToGBufferEffect(m_fxGBuffer,
                                                  mView,
                                                  mProj,
                                                  "TechniqueGBufferParticleFog");
    }

    Common::D3DDevice()->EndScene();
    Common::D3DDevice()->SetRenderTarget(0, surfaceOldFog);
    SAFE_RELEASE(surfaceFogZ);
    SAFE_RELEASE(surfaceOldFog);

    // 後続パス向けに GBuffer の深度レンジへ戻す
    m_fxGBuffer->SetFloat("g_fNear", m_nearPlane);
    m_fxGBuffer->SetFloat("g_fFar", m_farPlane);

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

        D3DXMATRIX matWorld;
        D3DXMatrixIdentity(&matWorld);
        {
            D3DXMATRIX m;
            D3DXMatrixIdentity(&m);
            D3DXMatrixScaling(&m, mesh.GetScale(), mesh.GetScale(), mesh.GetScale());
            matWorld *= m;
            D3DXMatrixRotationYawPitchRoll(&m, mesh.GetRot().y, mesh.GetRot().x, mesh.GetRot().z);
            matWorld *= m;
            D3DXVECTOR3 p = mesh.GetPos();
            D3DXMatrixTranslation(&m, p.x, p.y, p.z);
            matWorld *= m;
        }

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
