#include "PostEffectSSAO.h"

#include "Camera.h"
#include "GBuffer.h"

#include "Util.h"

namespace NSRender
{

void PostEffectSSAO::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectSSAO.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               effectPath.c_str(),
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_fxSSAO,
                                               NULL);
    assert(SUCCEEDED(hResult));

    CreateResources();
    if (!m_isRegisteredForDeviceReset)
    {
        Common::AddDeviceLostResource(this);
        m_isRegisteredForDeviceReset = true;
    }
    m_isInitialized = true;
}

void PostEffectSSAO::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }
    SAFE_RELEASE(m_rtAoTex);
    SAFE_RELEASE(m_rtAoTempTex);
    SAFE_RELEASE(m_fxSSAO);
    m_isInitialized = false;
}

void PostEffectSSAO::CreateResources()
{
    const UINT textureWidth = ComputeTextureSize(Common::ScreenW());
    const UINT textureHeight = ComputeTextureSize(Common::ScreenH());

    HRESULT hResult = D3DXCreateTexture(Common::D3DDevice(),
                                        textureWidth,
                                        textureHeight,
                                        1,
                                        D3DUSAGE_RENDERTARGET,
                                        D3DFMT_A16B16G16R16F,
                                        D3DPOOL_DEFAULT,
                                        &m_rtAoTex);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                textureWidth,
                                textureHeight,
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_rtAoTempTex);
    assert(SUCCEEDED(hResult));
}

void PostEffectSSAO::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                           LPDIRECT3DTEXTURE9 texTarget,
                           LPDIRECT3DTEXTURE9 texRenderTargetZ,
                           LPDIRECT3DTEXTURE9 texRenderTargetPos,
                           LPDIRECT3DTEXTURE9 texRenderTargetNormal,
                           LPDIRECT3DTEXTURE9 texRenderTargetThickness)
{
    if (!m_isInitialized || m_fxSSAO == NULL)
    {
        return;
    }

    D3DSURFACE_DESC descZ = { };
    texRenderTargetZ->GetLevelDesc(0, &descZ);

    D3DSURFACE_DESC descAO = { };
    m_rtAoTex->GetLevelDesc(0, &descAO);
    D3DXVECTOR2 invSize(1.0f / static_cast<float>(descAO.Width),
                        1.0f / static_cast<float>(descAO.Height));

    D3DXMATRIX matrixView = Camera::GetViewMatrix();
    D3DXMATRIX matrixProj = Camera::GetProjMatrix();

    LPDIRECT3DSURFACE9 oldRt0 = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &oldRt0);

    D3DVIEWPORT9 oldViewport { };
    Common::D3DDevice()->GetViewport(&oldViewport);

    D3DVIEWPORT9 aoViewport { };
    aoViewport.X = 0;
    aoViewport.Y = 0;
    aoViewport.Width = descAO.Width;
    aoViewport.Height = descAO.Height;
    aoViewport.MinZ = 0.0f;
    aoViewport.MaxZ = 1.0f;

    LPDIRECT3DSURFACE9 surfAO = NULL;
    LPDIRECT3DSURFACE9 surfAOTemp = NULL;
    LPDIRECT3DSURFACE9 surfRenderTarget = NULL;
    LPDIRECT3DTEXTURE9 aoTextureForComposite = m_rtAoTex;

    m_rtAoTex->GetSurfaceLevel(0, &surfAO);
    m_rtAoTempTex->GetSurfaceLevel(0, &surfAOTemp);
    texTarget->GetSurfaceLevel(0, &surfRenderTarget);

    Common::D3DDevice()->SetRenderTarget(0, surfAO);
    Common::D3DDevice()->SetViewport(&aoViewport);
    Common::D3DDevice()->SetRenderTarget(1, NULL);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(255, 255, 255, 255), 1.0f, 0);

    m_fxSSAO->SetMatrix("g_matView", &matrixView);
    m_fxSSAO->SetMatrix("g_matProj", &matrixProj);
    m_fxSSAO->SetFloat("g_fNear", m_nearPlane);
    m_fxSSAO->SetFloat("g_fFar", m_farPlane);
    m_fxSSAO->SetFloat("g_posRange", m_positionRange);
    m_fxSSAO->SetFloatArray("g_invSize", reinterpret_cast<FLOAT*>(&invSize), 2);
    m_fxSSAO->SetTexture("texZ", texRenderTargetZ);
    m_fxSSAO->SetTexture("texPos", texRenderTargetPos);
    m_fxSSAO->SetTexture("texNormal", texRenderTargetNormal);
    m_fxSSAO->SetTexture("texThickness", texRenderTargetThickness);
    m_fxSSAO->SetFloat("g_sampleRadius", m_sampleRadius);
    m_fxSSAO->SetInt("g_sampleCount", m_sampleCount);
    m_fxSSAO->SetBool("g_useRandomSamplingDirection", m_randomSamplingDirectionEnabled);
    m_fxSSAO->SetBool("g_enableDepthScaledSampleDistance", m_depthScaledSampleDistanceEnabled);
    m_fxSSAO->SetFloat("g_depthCompareThreshold", 0.0f);
    m_fxSSAO->SetFloat("g_depthBiasScale", 1.0f);
    m_fxSSAO->SetFloat("g_normalBiasScale", 1.0f);

    m_fxSSAO->SetTechnique(GetCreateTechniqueName());
    m_fxSSAO->Begin(NULL, 0);
    m_fxSSAO->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO->EndPass();
    m_fxSSAO->End();

    if (m_blurEnabled)
    {
        Common::D3DDevice()->SetRenderTarget(0, surfAOTemp);
        Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(255, 255, 255, 255), 1.0f, 0);
        m_fxSSAO->SetTexture("texAO", m_rtAoTex);
        m_fxSSAO->SetTechnique(GetBlurTechniqueName());
        m_fxSSAO->Begin(NULL, 0);
        m_fxSSAO->BeginPass(0);
        DrawFullscreenQuad();
        m_fxSSAO->EndPass();
        m_fxSSAO->End();
        aoTextureForComposite = m_rtAoTempTex;
    }

    D3DSURFACE_DESC descTarget = { };
    texTarget->GetLevelDesc(0, &descTarget);
    D3DVIEWPORT9 targetViewport { };
    targetViewport.X = 0;
    targetViewport.Y = 0;
    targetViewport.Width = descTarget.Width;
    targetViewport.Height = descTarget.Height;
    targetViewport.MinZ = 0.0f;
    targetViewport.MaxZ = 1.0f;

    D3DXVECTOR2 targetInvSize(1.0f / static_cast<float>(descTarget.Width),
                              1.0f / static_cast<float>(descTarget.Height));
    D3DXVECTOR2 aoInvSize(1.0f / static_cast<float>(descAO.Width),
                          1.0f / static_cast<float>(descAO.Height));

    Common::D3DDevice()->SetRenderTarget(0, surfRenderTarget);
    Common::D3DDevice()->SetViewport(&targetViewport);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);
    m_fxSSAO->SetFloatArray("g_invSize", reinterpret_cast<FLOAT*>(&targetInvSize), 2);
    m_fxSSAO->SetFloatArray("g_aoInvSize", reinterpret_cast<FLOAT*>(&aoInvSize), 2);
    m_fxSSAO->SetTexture("texColor", renderTarget);
    m_fxSSAO->SetTexture("texAO", aoTextureForComposite);
    m_fxSSAO->SetFloat("g_shadowStrength", m_shadowStrength);
    m_fxSSAO->SetFloat("g_aoSaturationBoost", m_saturationBoost);
    m_fxSSAO->SetBool("g_enableMaxDarknessClamp", m_maxDarknessClampEnabled);
    m_fxSSAO->SetTechnique(GetCompositeTechniqueName());
    m_fxSSAO->Begin(NULL, 0);
    m_fxSSAO->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSAO->EndPass();
    m_fxSSAO->End();
    Common::D3DDevice()->SetRenderTarget(0, oldRt0);
    Common::D3DDevice()->SetViewport(&oldViewport);

    SAFE_RELEASE(surfAO);
    SAFE_RELEASE(surfAOTemp);
    SAFE_RELEASE(surfRenderTarget);
    SAFE_RELEASE(oldRt0);

}

void PostEffectSSAO::DrawFullscreenQuad()
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

void PostEffectSSAO::SetShadowStrength(const float shadowStrength)
{
    m_shadowStrength = shadowStrength;
}

void PostEffectSSAO::SetSaturationBoost(const float saturationBoost)
{
    m_saturationBoost = saturationBoost;
}

void PostEffectSSAO::SetSampleRadius(const float sampleRadius)
{
    m_sampleRadius = sampleRadius;
}

void PostEffectSSAO::SetSampleCount(const int sampleCount)
{
    m_sampleCount = NormalizeSampleCount(sampleCount);
}

void PostEffectSSAO::SetRandomSamplingDirectionEnabled(const bool enabled)
{
    m_randomSamplingDirectionEnabled = enabled;
}

void PostEffectSSAO::SetDepthScaledSampleDistanceEnabled(const bool enabled)
{
    m_depthScaledSampleDistanceEnabled = enabled;
}

void PostEffectSSAO::SetBlurEnabled(const bool enabled)
{
    m_blurEnabled = enabled;
}

void PostEffectSSAO::SetBlurKernelSize(const int kernelSize)
{
    m_blurKernelSize = NormalizeBlurKernelSize(kernelSize);
}

void PostEffectSSAO::SetTextureScaleDivisor(const int scaleDivisor)
{
    const int normalizedDivisor = NormalizeTextureScaleDivisor(scaleDivisor);
    if (m_textureScaleDivisor == normalizedDivisor)
    {
        return;
    }

    m_textureScaleDivisor = normalizedDivisor;
    if (m_isInitialized)
    {
        SAFE_RELEASE(m_rtAoTex);
        SAFE_RELEASE(m_rtAoTempTex);
        CreateResources();
    }
}

void PostEffectSSAO::SetCompositeGaussian3x3Enabled(const bool enabled)
{
    m_compositeGaussian3x3Enabled = enabled;
}

void PostEffectSSAO::SetMaxDarknessClampEnabled(const bool enabled)
{
    m_maxDarknessClampEnabled = enabled;
}

void PostEffectSSAO::SetDepthRange(const float nearPlane, const float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_positionRange = GBuffer::ComputePositionRange(nearPlane, farPlane);
}

const char* PostEffectSSAO::GetCreateTechniqueName() const
{
    if (m_sampleCount == 4)
    {
        return "TechniqueAO_Create4";
    }

    if (m_sampleCount == 8)
    {
        return "TechniqueAO_Create8";
    }

    if (m_sampleCount == 16)
    {
        return "TechniqueAO_Create16";
    }

    if (m_sampleCount == 32)
    {
        return "TechniqueAO_Create32";
    }

    return "TechniqueAO_Create64";
}

int PostEffectSSAO::NormalizeBlurKernelSize(const int kernelSize) const
{
    if (kernelSize <= 4)
    {
        return 3;
    }

    if (kernelSize <= 8)
    {
        return 5;
    }

    if (kernelSize <= 16)
    {
        return 11;
    }

    return 21;
}

const char* PostEffectSSAO::GetBlurTechniqueName() const
{
    if (m_blurKernelSize == 3)
    {
        return "TechniqueAO_Blur3x3";
    }

    if (m_blurKernelSize == 5)
    {
        return "TechniqueAO_Blur5x5";
    }

    if (m_blurKernelSize == 11)
    {
        return "TechniqueAO_Blur11x11";
    }

    return "TechniqueAO_Blur21x21";
}

int PostEffectSSAO::NormalizeSampleCount(const int sampleCount) const
{
    if (sampleCount <= 6)
    {
        return 4;
    }

    if (sampleCount <= 12)
    {
        return 8;
    }

    if (sampleCount <= 24)
    {
        return 16;
    }

    if (sampleCount <= 48)
    {
        return 32;
    }

    return 64;
}

int PostEffectSSAO::NormalizeTextureScaleDivisor(const int scaleDivisor) const
{
    if (scaleDivisor == 2)
    {
        return 2;
    }

    if (scaleDivisor == 4)
    {
        return 4;
    }

    return 1;
}

const char* PostEffectSSAO::GetCompositeTechniqueName() const
{
    if (m_compositeGaussian3x3Enabled)
    {
        return "TechniqueAO_Composite3x3Gaussian";
    }

    return "TechniqueAO_Composite";
}

UINT PostEffectSSAO::ComputeTextureSize(const int screenSize) const
{
    return static_cast<UINT>((std::max)(1, screenSize / m_textureScaleDivisor));
}

void PostEffectSSAO::OnDeviceLost()
{
    if (!m_isInitialized || m_fxSSAO == NULL)
    {
        return;
    }

    if (m_fxSSAO != NULL)
    {
        m_fxSSAO->OnLostDevice();
    }
    SAFE_RELEASE(m_rtAoTex);
    SAFE_RELEASE(m_rtAoTempTex);
}

void PostEffectSSAO::OnDeviceReset()
{
    if (!m_isInitialized || m_fxSSAO == NULL)
    {
        return;
    }

    if (m_fxSSAO != NULL)
    {
        m_fxSSAO->OnResetDevice();
    }
    CreateResources();
}

}
