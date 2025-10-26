
#include "GBuffer.h"

#include "Common.h"
#include "Camera.h"

#include "PostEffectSSAO.h"

namespace NSRender
{

void GBuffer::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                  //L"res/shader/GBuffer.fx",
                                  L"../x64/Debug/GBuffer.cso",
                                  NULL,
                                  NULL,
                                  D3DXSHADER_DEBUG,
                                  NULL,
                                  &m_fxGBuffer,
                                  NULL);
    assert(hResult == S_OK);

    CreateRawResource();

    Common::AddDeviceLostResource(this);
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
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetZ);
    assert(hResult == S_OK);

    // World座標
    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_texRenderTargetPos);
    assert(hResult == S_OK);
}

void GBuffer::Draw(const std::vector<MeshMix>& meshList, LPDIRECT3DTEXTURE9* Z, LPDIRECT3DTEXTURE9* Pos)
{
    HRESULT hr = E_FAIL;

    // 既存の RT0 を退避
    LPDIRECT3DSURFACE9 surfaceOld = NULL;
    hr = Common::D3DDevice()->GetRenderTarget(0, &surfaceOld);

    // Z と POS のサーフェスを取得
    LPDIRECT3DSURFACE9 surfaceZ = NULL;
    LPDIRECT3DSURFACE9 surfacePos = NULL;
    hr = m_texRenderTargetZ->GetSurfaceLevel(0, &surfaceZ);
    hr = m_texRenderTargetPos->GetSurfaceLevel(0, &surfacePos);

    // MRT×2 をセット
    hr = Common::D3DDevice()->SetRenderTarget(0, surfaceZ);
    hr = Common::D3DDevice()->SetRenderTarget(1, surfacePos);

    // クリア。Zバッファも一緒に初期化して素直に全描画
    hr = Common::D3DDevice()->Clear(0,
                                    NULL,
                                    D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                    D3DCOLOR_RGBA(0, 0, 0, 0),
                                    1.0f,
                                    0);

    hr = Common::D3DDevice()->BeginScene();

    // ここで「不透明物体のみ」を GBuffer.fx で描く
    for (auto& mesh : meshList)
    {
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
        auto mView = Camera::GetViewMatrix();
        auto mProj = Camera::GetProjMatrix();
        m_fxGBuffer->SetMatrix("g_matView",  &mView);
        m_fxGBuffer->SetMatrix("g_matProj",  &mProj);

        // 近遠と posRange は SSAO 側と必ず一致させる
        m_fxGBuffer->SetFloat("g_fNear", Camera::GetNear());
        m_fxGBuffer->SetFloat("g_fFar",  Camera::GetFar());
        m_fxGBuffer->SetFloat("g_posRange", PostEffectSSAO::Z_RANGE);

        m_fxGBuffer->SetTechnique("TechniqueGBuffer");
        m_fxGBuffer->Begin(NULL, 0);
        m_fxGBuffer->BeginPass(0);

        // メッシュ本体を描画
        LPD3DXMESH d3dMesh = mesh.GetD3DMesh();
        d3dMesh->DrawSubset(0);

        m_fxGBuffer->EndPass();
        m_fxGBuffer->End();
    }

    hr = Common::D3DDevice()->EndScene();

    // MRT を外し、RT0 を元に戻す
    Common::D3DDevice()->SetRenderTarget(1, NULL);
    Common::D3DDevice()->SetRenderTarget(0, surfaceOld);

    SAFE_RELEASE(surfaceZ);
    SAFE_RELEASE(surfacePos);
    SAFE_RELEASE(surfaceOld);

    *Z = m_texRenderTargetZ;
    *Pos = m_texRenderTargetPos;
}

void GBuffer::Finalize()
{

}

void GBuffer::OnDeviceLost()
{
    m_fxGBuffer->OnLostDevice();
    SAFE_RELEASE(m_texRenderTargetZ);
    SAFE_RELEASE(m_texRenderTargetPos);
}

void GBuffer::OnDeviceReset()
{
    CreateRawResource();
    m_fxGBuffer->OnResetDevice();
}

}
