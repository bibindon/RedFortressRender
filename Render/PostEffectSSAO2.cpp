#include "PostEffectSSAO2.h"

#include "Camera.h"
#include "PostEffectSSAO.h"

namespace NSRender
{

void PostEffectSSAO2::Initialize()
{
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               // L"..\\..\\Render\\shader\\PostEffectSSAO2.fx",
                                               L"..\\x64\\Debug\\PostEffectSSAO2.cso",
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_fxSSAO2,
                                               NULL);
    assert(SUCCEEDED(hResult));

    CreateResources();
    Common::AddDeviceLostResource(this);
}

void PostEffectSSAO2::Finalize()
{
}

void PostEffectSSAO2::CreateResources()
{
    HRESULT hResult = D3DXCreateTexture(Common::D3DDevice(),
                                        Common::ScreenW(),
                                        Common::ScreenH(),
                                        1,
                                        D3DUSAGE_RENDERTARGET,
                                        D3DFMT_A16B16G16R16F,
                                        D3DPOOL_DEFAULT,
                                        &m_rtAoTex);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                Common::ScreenW(),
                                Common::ScreenH(),
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_rtAoTempTex);
    assert(SUCCEEDED(hResult));
}

LPDIRECT3DTEXTURE9 PostEffectSSAO2::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                         LPDIRECT3DTEXTURE9 texRenderTargetZ,
                                         LPDIRECT3DTEXTURE9 texRenderTargetPos,
                                         LPDIRECT3DTEXTURE9 texRenderTargetNormal,
                                         LPDIRECT3DTEXTURE9 texRenderTargetThickness)
{
    D3DSURFACE_DESC descZ = { };
    texRenderTargetZ->GetLevelDesc(0, &descZ);
    D3DXVECTOR2 invSize(1.0f / static_cast<float>(descZ.Width),
                        1.0f / static_cast<float>(descZ.Height));

    D3DXMATRIX matrixView = Camera::GetViewMatrix();
    D3DXMATRIX matrixProj = Camera::GetProjMatrix();

    LPDIRECT3DSURFACE9 oldRt0 = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &oldRt0);

    LPDIRECT3DSURFACE9 surfAO = NULL;
    LPDIRECT3DSURFACE9 surfAOTemp = NULL;
    LPDIRECT3DSURFACE9 surfRenderTarget = NULL;
    LPDIRECT3DTEXTURE9 aoTextureForComposite = m_rtAoTex;

    m_rtAoTex->GetSurfaceLevel(0, &surfAO);
    m_rtAoTempTex->GetSurfaceLevel(0, &surfAOTemp);
    renderTarget->GetSurfaceLevel(0, &surfRenderTarget);

    Common::D3DDevice()->SetRenderTarget(0, surfAO);
    Common::D3DDevice()->SetRenderTarget(1, NULL);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(255, 255, 255, 255), 1.0f, 0);

    m_fxSSAO2->SetMatrix("g_matView", &matrixView);
    m_fxSSAO2->SetMatrix("g_matProj", &matrixProj);
    m_fxSSAO2->SetFloat("g_fNear", m_nearPlane);
    m_fxSSAO2->SetFloat("g_fFar", m_farPlane);
    m_fxSSAO2->SetFloat("g_posRange", PostEffectSSAO::Z_RANGE);
    m_fxSSAO2->SetFloatArray("g_invSize", reinterpret_cast<FLOAT*>(&invSize), 2);
    m_fxSSAO2->SetTexture("texZ", texRenderTargetZ);
    m_fxSSAO2->SetTexture("texPos", texRenderTargetPos);
    m_fxSSAO2->SetTexture("texNormal", texRenderTargetNormal);
    m_fxSSAO2->SetTexture("texThickness", texRenderTargetThickness);
    m_fxSSAO2->SetFloat("g_sampleRadius", m_sampleRadius);
    m_fxSSAO2->SetFloat("g_depthCompareThreshold", 0.0f);
    m_fxSSAO2->SetFloat("g_depthBiasScale", 1.0f);
    m_fxSSAO2->SetFloat("g_normalBiasScale", 1.0f);

    m_fxSSAO2->SetTechnique("TechniqueAO2_Create");
    m_fxSSAO2->Begin(NULL, 0);
    m_fxSSAO2->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO2->EndPass();
    m_fxSSAO2->End();

    if (m_blurEnabled)
    {
        Common::D3DDevice()->SetRenderTarget(0, surfAOTemp);
        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(255, 255, 255, 255), 1.0f, 0);
        m_fxSSAO2->SetTexture("texAO", m_rtAoTex);
        m_fxSSAO2->SetTechnique("TechniqueAO2_Blur21x21");
        m_fxSSAO2->Begin(NULL, 0);
        m_fxSSAO2->BeginPass(0);
        DrawFullscreenQuad();
        m_fxSSAO2->EndPass();
        m_fxSSAO2->End();
        aoTextureForComposite = m_rtAoTempTex;
    }

    Common::D3DDevice()->SetRenderTarget(0, surfAOTemp);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);
    m_fxSSAO2->SetTexture("texColor", renderTarget);
    m_fxSSAO2->SetTexture("texAO", aoTextureForComposite);
    m_fxSSAO2->SetFloat("g_aoBrightness", m_brightness);
    m_fxSSAO2->SetFloat("g_aoSaturationBoost", m_saturationBoost);
    m_fxSSAO2->SetTechnique("TechniqueAO2_Composite");
    m_fxSSAO2->Begin(NULL, 0);
    m_fxSSAO2->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO2->EndPass();
    m_fxSSAO2->End();

    Common::D3DDevice()->StretchRect(surfAOTemp, NULL, surfRenderTarget, NULL, D3DTEXF_NONE);
    Common::D3DDevice()->SetRenderTarget(0, oldRt0);

    SAFE_RELEASE(surfAO);
    SAFE_RELEASE(surfAOTemp);
    SAFE_RELEASE(surfRenderTarget);
    SAFE_RELEASE(oldRt0);

    return renderTarget;
}

void PostEffectSSAO2::DrawFullscreenQuad()
{
    static FullscreenVertex vertices[4] =
    {
        { -1.f, -1.f, 0.f, 1.f, 0.f, 1.f },
        { -1.f,  1.f, 0.f, 1.f, 0.f, 0.f },
        {  1.f, -1.f, 0.f, 1.f, 1.f, 1.f },
        {  1.f,  1.f, 0.f, 1.f, 1.f, 0.f }
    };

    static D3DVERTEXELEMENT9 decl[] =
    {
        { 0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    LPDIRECT3DVERTEXDECLARATION9 vertexDecl = NULL;
    Common::D3DDevice()->CreateVertexDeclaration(decl, &vertexDecl);
    Common::D3DDevice()->SetVertexDeclaration(vertexDecl);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(FullscreenVertex));
    SAFE_RELEASE(vertexDecl);
}

void PostEffectSSAO2::SetBrightness(const float brightness)
{
    m_brightness = brightness;
}

void PostEffectSSAO2::SetSaturationBoost(const float saturationBoost)
{
    m_saturationBoost = saturationBoost;
}

void PostEffectSSAO2::SetSampleRadius(const float sampleRadius)
{
    m_sampleRadius = sampleRadius;
}

void PostEffectSSAO2::SetBlurEnabled(const bool enabled)
{
    m_blurEnabled = enabled;
}

void PostEffectSSAO2::SetDepthRange(const float nearPlane, const float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

void PostEffectSSAO2::OnDeviceLost()
{
    if (m_fxSSAO2 != NULL)
    {
        m_fxSSAO2->OnLostDevice();
    }
    SAFE_RELEASE(m_rtAoTex);
    SAFE_RELEASE(m_rtAoTempTex);
}

void PostEffectSSAO2::OnDeviceReset()
{
    if (m_fxSSAO2 != NULL)
    {
        m_fxSSAO2->OnResetDevice();
    }
    CreateResources();
}

}
