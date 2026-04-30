#include "PostEffectDepthOfField.h"

#include "Camera.h"
#include "PostEffectSSAO.h"

namespace NSRender
{

namespace
{
constexpr float POSITION_RANGE = PostEffectSSAO::Z_RANGE;
}

void PostEffectDepthOfField::Initialize()
{
    HRESULT hr = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                          L"../x64/Debug/PostEffectDepthOfField.cso",
                                          NULL,
                                          NULL,
                                          D3DXSHADER_DEBUG,
                                          NULL,
                                          &m_d3dEffect,
                                          NULL);
    assert(SUCCEEDED(hr));

    hr = D3DXCreateTexture(Common::D3DDevice(),
                           Common::ScreenW(),
                           Common::ScreenH(),
                           1,
                           D3DUSAGE_RENDERTARGET,
                           D3DFMT_A16B16G16R16F,
                           D3DPOOL_DEFAULT,
                           &m_texWork);
    assert(SUCCEEDED(hr));

    Common::AddDeviceLostResource(this);
}

LPDIRECT3DTEXTURE9 PostEffectDepthOfField::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                                LPDIRECT3DTEXTURE9 texRenderTargetPos)
{
    if (!m_enable || renderTarget == NULL || texRenderTargetPos == NULL)
    {
        return renderTarget;
    }

    const D3DXVECTOR3 cameraPos = Camera::GetEyePos();
    const D3DXVECTOR4 cameraPos4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);

    m_d3dEffect->SetVector("g_cameraPos", &cameraPos4);
    m_d3dEffect->SetFloat("g_focalDistanceMeters", m_focalDistance);
    m_d3dEffect->SetFloat("g_focusBandHalfWidthMeters", m_focusBandHalfWidth);
    m_d3dEffect->SetFloat("g_blurRadiusPixels", m_blurRadiusPixels);
    m_d3dEffect->SetFloat("g_positionRange", POSITION_RANGE);

    DrawFullscreenQuad(renderTarget, texRenderTargetPos, m_texWork, "Technique1");
    return m_texWork;
}

void PostEffectDepthOfField::Finalize()
{
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectDepthOfField::SetEnable(bool enable)
{
    m_enable = enable;
}

bool PostEffectDepthOfField::GetEnable() const
{
    return m_enable;
}

void PostEffectDepthOfField::SetFocalDistance(float focalDistance)
{
    m_focalDistance = focalDistance;
}

void PostEffectDepthOfField::SetFocusBandHalfWidth(float focusBandHalfWidth)
{
    m_focusBandHalfWidth = focusBandHalfWidth;
}

void PostEffectDepthOfField::SetBlurRadiusPixels(float blurRadiusPixels)
{
    m_blurRadiusPixels = blurRadiusPixels;
}

void PostEffectDepthOfField::OnDeviceLost()
{
    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnLostDevice();
    }

    SAFE_RELEASE(m_texWork);
}

void PostEffectDepthOfField::OnDeviceReset()
{
    if (m_d3dEffect != NULL)
    {
        m_d3dEffect->OnResetDevice();
    }

    HRESULT hr = D3DXCreateTexture(Common::D3DDevice(),
                                   Common::ScreenW(),
                                   Common::ScreenH(),
                                   1,
                                   D3DUSAGE_RENDERTARGET,
                                   D3DFMT_A16B16G16R16F,
                                   D3DPOOL_DEFAULT,
                                   &m_texWork);
    assert(SUCCEEDED(hr));
}

void PostEffectDepthOfField::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                                LPDIRECT3DTEXTURE9 texPosition,
                                                LPDIRECT3DTEXTURE9 texTarget,
                                                const std::string& technique)
{
    LPDIRECT3DSURFACE9 sceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &sceneRT);
    Common::D3DDevice()->SetRenderTarget(0, sceneRT);
    SAFE_RELEASE(sceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texSource);
    m_d3dEffect->SetTexture("g_PositionTex", texPosition);

    const float texelSize[2] =
    {
        1.0f / static_cast<float>(Common::ScreenW()),
        1.0f / static_cast<float>(Common::ScreenH())
    };
    m_d3dEffect->SetFloatArray("g_TexelSize", texelSize, 2);

    ScreenVertex quad[4] { };
    quad[0] = { 0.f,                              0.f,                               0.0f, 1.0f, 0.0f, 0.0f };
    quad[1] = { static_cast<float>(Common::ScreenW()), 0.f,                   0.0f, 1.0f, 1.0f, 0.0f };
    quad[2] = { 0.f,                              static_cast<float>(Common::ScreenH()), 0.0f, 1.0f, 0.0f, 1.0f };
    quad[3] = { static_cast<float>(Common::ScreenW()), static_cast<float>(Common::ScreenH()), 0.0f, 1.0f, 1.0f, 1.0f };

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

}
