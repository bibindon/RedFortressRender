#include "PostEffectGodRay.h"

#include "Util.h"

namespace NSRender
{

namespace
{

float Clamp01(const float value)
{
    return max(0.0f, min(1.0f, value));
}

void ClampProjectedPointToScreenEdge(float& screenU, float& screenV)
{
    if (screenU >= 0.0f && screenU <= 1.0f &&
        screenV >= 0.0f && screenV <= 1.0f)
    {
        return;
    }

    const float du = screenU - 0.5f;
    const float dv = screenV - 0.5f;
    float scale = 1.0f;

    if (fabsf(du) > 0.000001f)
    {
        const float edgeU = du > 0.0f ? 1.0f : 0.0f;
        scale = min(scale, (edgeU - 0.5f) / du);
    }

    if (fabsf(dv) > 0.000001f)
    {
        const float edgeV = dv > 0.0f ? 1.0f : 0.0f;
        scale = min(scale, (edgeV - 0.5f) / dv);
    }

    scale = Clamp01(scale);
    screenU = Clamp01(0.5f + du * scale);
    screenV = Clamp01(0.5f + dv * scale);
}

}

void PostEffectGodRay::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectGodRay.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               effectPath.c_str(),
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_d3dEffect,
                                               NULL);
    assert(SUCCEEDED(hResult));

    CreateTexture();

    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }
    m_isInitialized = true;
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

}

void PostEffectGodRay::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                            LPDIRECT3DTEXTURE9 texZ,
                            LPDIRECT3DTEXTURE9 texTarget)
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

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
        ClampProjectedPointToScreenEdge(screenU, screenV);
        lightVisible = 1.0f;
    }

    float sideAttenuation = 1.0f;
    D3DXVECTOR3 cameraForward = Camera::GetLookAtPos() - Camera::GetEyePos();
    D3DXVECTOR3 toLight = m_lightPosWorld - Camera::GetEyePos();
    if (D3DXVec3LengthSq(&cameraForward) > 0.000001f &&
        D3DXVec3LengthSq(&toLight) > 0.000001f)
    {
        D3DXVec3Normalize(&cameraForward, &cameraForward);
        D3DXVec3Normalize(&toLight, &toLight);

        float alignment = fabsf(D3DXVec3Dot(&cameraForward, &toLight));
        const float attenuationStart = 0.15f;
        const float attenuationEnd = 0.45f;
        sideAttenuation = (alignment - attenuationStart) / (attenuationEnd - attenuationStart);
        sideAttenuation = max(0.0f, min(1.0f, sideAttenuation));
    }

    m_d3dEffect->SetTexture("g_ZTex", texZ);
    m_d3dEffect->SetFloat("g_LightViewZ", lightViewZ);
    DrawFullscreenQuad(m_texOcclusion, "OcclusionMask");

    BlurOcclusionTexture();

    float lightPos2[2] = { screenU, screenV };
    float lightColor3[3] = { m_lightColor.x, m_lightColor.y, m_lightColor.z };
    float reverseSampling = 0.0f;
    if (m_reverseSampling)
    {
        reverseSampling = 1.0f;
    }

    m_d3dEffect->SetTexture("g_SceneTex", renderTarget);
    m_d3dEffect->SetTexture("g_OcclusionTex", m_texOcclusionBlurred);
    m_d3dEffect->SetFloatArray("g_LightScreenPos", lightPos2, 2);
    m_d3dEffect->SetFloatArray("g_LightColor", lightColor3, 3);
    m_d3dEffect->SetFloat("g_ReverseSampling", reverseSampling);
    m_d3dEffect->SetFloat("g_RayLength", m_rayLength);
    m_d3dEffect->SetFloat("g_RayIntensity", m_rayIntensity * lightVisible * sideAttenuation);
    m_d3dEffect->SetFloat("g_VirtualProximityStrength", m_virtualProximityStrength);
    m_d3dEffect->SetFloat("g_OcclusionFalloff", m_occlusionFalloff);
    DrawFullscreenQuad(texTarget, "GodRay");
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
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    SAFE_RELEASE(m_d3dEffect);
    SAFE_RELEASE(m_texOcclusion);
    SAFE_RELEASE(m_texBlurTemp);
    SAFE_RELEASE(m_texOcclusionBlurred);
    m_isInitialized = false;
}

void PostEffectGodRay::OnDeviceLost()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnLostDevice();
    SAFE_RELEASE(m_texOcclusion);
    SAFE_RELEASE(m_texBlurTemp);
    SAFE_RELEASE(m_texOcclusionBlurred);
}

void PostEffectGodRay::OnDeviceReset()
{
    if (!m_isInitialized || m_d3dEffect == NULL)
    {
        return;
    }

    m_d3dEffect->OnResetDevice();
    CreateTexture();
}

}
