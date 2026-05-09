#include "PostEffectGodRay.h"

namespace NSRender
{

void PostEffectGodRay::Initialize()
{
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               L"../x64/Debug/PostEffectGodRay.cso",
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_d3dEffect,
                                               NULL);
    assert(SUCCEEDED(hResult));

    CreateTexture();

    Common::AddDeviceLostResource(this);
}

void PostEffectGodRay::CreateTexture()
{
    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texOcclusion);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texBlurTemp);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texOcclusionBlurred);

    D3DXCreateTexture(Common::D3DDevice(),
                      Common::ScreenW(),
                      Common::ScreenH(),
                      1,
                      D3DUSAGE_RENDERTARGET,
                      D3DFMT_A8R8G8B8,
                      D3DPOOL_DEFAULT,
                      &m_texResult);
}

LPDIRECT3DTEXTURE9 PostEffectGodRay::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                          LPDIRECT3DTEXTURE9 texZ)
{
    D3DXMATRIX matView = Camera::GetViewMatrix();
    D3DXMATRIX matProj = Camera::GetProjMatrix();

    D3DXVECTOR4 lightClip;
    D3DXVECTOR4 lightWorld4(m_lightPosWorld.x,
                            m_lightPosWorld.y,
                            m_lightPosWorld.z,
                            1.0f);
    D3DXVec4Transform(&lightClip, &lightWorld4, &matView);
    float lightViewZ = (lightClip.z - m_depthNearPlane) /
                       (m_depthFarPlane - m_depthNearPlane);
    lightViewZ = max(0.0f, min(1.0f, lightViewZ));

    D3DXVECTOR4 lightProj;
    D3DXMATRIX matViewProj = matView * matProj;
    D3DXVec4Transform(&lightProj, &lightWorld4, &matViewProj);

    float screenU = 0.5f;
    float screenV = 0.5f;
    float lightVisible = 0.0f;
    if (lightProj.w > 0.0f)
    {
        screenU = (lightProj.x / lightProj.w) * 0.5f + 0.5f;
        screenV = (-lightProj.y / lightProj.w) * 0.5f + 0.5f;
        lightVisible = 1.0f;
    }

    m_d3dEffect->SetTexture("g_ZTex", texZ);
    m_d3dEffect->SetFloat("g_LightViewZ", lightViewZ);
    DrawFullscreenQuad(m_texOcclusion, "OcclusionMask");

    BlurOcclusionTexture();

    float lightPos2[2] = { screenU, screenV };
    float lightColor3[3] = { m_lightColor.x, m_lightColor.y, m_lightColor.z };
    const float reverseSampling = m_reverseSampling ? 1.0f : 0.0f;

    m_d3dEffect->SetTexture("g_SceneTex", renderTarget);
    m_d3dEffect->SetTexture("g_OcclusionTex", m_texOcclusionBlurred);
    m_d3dEffect->SetFloatArray("g_LightScreenPos", lightPos2, 2);
    m_d3dEffect->SetFloatArray("g_LightColor", lightColor3, 3);
    m_d3dEffect->SetFloat("g_ReverseSampling", reverseSampling);
    m_d3dEffect->SetFloat("g_RayLength", m_rayLength);
    m_d3dEffect->SetFloat("g_RayIntensity", m_rayIntensity * lightVisible);
    m_d3dEffect->SetFloat("g_VirtualProximityStrength", m_virtualProximityStrength);
    m_d3dEffect->SetFloat("g_OcclusionFalloff", m_occlusionFalloff);
    DrawFullscreenQuad(m_texResult, "GodRay");

    return m_texResult;
}

void PostEffectGodRay::SetDepthRange(const float nearPlane, const float farPlane)
{
    m_depthNearPlane = nearPlane;
    m_depthFarPlane = farPlane;
    if (m_depthFarPlane <= m_depthNearPlane)
    {
        m_depthFarPlane = m_depthNearPlane + 0.01f;
    }
}

void PostEffectGodRay::BlurOcclusionTexture()
{
    const float texelSize[2] =
    {
        1.0f / static_cast<float>(Common::ScreenW()),
        1.0f / static_cast<float>(Common::ScreenH())
    };
    const float horizontal[2] = { 1.0f, 0.0f };
    const float vertical[2] = { 0.0f, 1.0f };

    m_d3dEffect->SetTexture("g_BlurSourceTex", m_texOcclusion);
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);
    m_d3dEffect->SetFloatArray("g_BlurDirection", horizontal, 2);
    DrawFullscreenQuad(m_texBlurTemp, "Blur");

    m_d3dEffect->SetTexture("g_BlurSourceTex", m_texBlurTemp);
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);
    m_d3dEffect->SetFloatArray("g_BlurDirection", vertical, 2);
    DrawFullscreenQuad(m_texOcclusionBlurred, "Blur");
}

void PostEffectGodRay::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                                          const std::string& technique)
{
    LPDIRECT3DSURFACE9 pSurface = NULL;
    texTarget->GetSurfaceLevel(0, &pSurface);
    Common::D3DDevice()->SetRenderTarget(0, pSurface);
    SAFE_RELEASE(pSurface);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());

    const float w = static_cast<float>(Common::ScreenW());
    const float h = static_cast<float>(Common::ScreenH());

    ScreenVertex quad[4]{};
    quad[0] = { -0.5f,     -0.5f,     0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { -0.5f + w, -0.5f,     0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { -0.5f,     -0.5f + h, 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { -0.5f + w, -0.5f + h, 0.0f, 1.0f, 1.0f, 1.0f };

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    Common::D3DDevice()->BeginScene();

    m_d3dEffect->Begin(NULL, 0);
    m_d3dEffect->BeginPass(0);

    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));

    m_d3dEffect->EndPass();
    m_d3dEffect->End();

    Common::D3DDevice()->EndScene();
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}

void PostEffectGodRay::Finalize()
{
    SAFE_RELEASE(m_d3dEffect);
    SAFE_RELEASE(m_texOcclusion);
    SAFE_RELEASE(m_texBlurTemp);
    SAFE_RELEASE(m_texOcclusionBlurred);
    SAFE_RELEASE(m_texResult);
}

void PostEffectGodRay::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
    SAFE_RELEASE(m_texOcclusion);
    SAFE_RELEASE(m_texBlurTemp);
    SAFE_RELEASE(m_texOcclusionBlurred);
    SAFE_RELEASE(m_texResult);
}

void PostEffectGodRay::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();
    CreateTexture();
}

}
