#include "PostEffectSSAO.h"

#include <stdexcept>

#include "Camera.h"
#include "Util.h"

namespace NSRender
{
namespace
{
void ThrowIfSsaoCallFailed(const HRESULT result, const char* operation)
{
    if (FAILED(result))
    {
        throw std::runtime_error(operation);
    }
}

void ThrowIfSsaoResourceIsNull(const void* resource, const char* operation)
{
    if (resource == nullptr)
    {
        throw std::runtime_error(operation);
    }
}
}

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
    ThrowIfSsaoCallFailed(hResult, "PostEffectSSAO failed to create its effect.");
    ThrowIfSsaoResourceIsNull(m_fxSSAO, "PostEffectSSAO effect creation returned a null effect.");

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
    ThrowIfSsaoCallFailed(hResult, "PostEffectSSAO failed to create its AO render-target texture.");
    ThrowIfSsaoResourceIsNull(m_rtAoTex, "PostEffectSSAO AO texture creation returned a null texture.");

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                textureWidth,
                                textureHeight,
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_rtAoTempTex);
    ThrowIfSsaoCallFailed(hResult, "PostEffectSSAO failed to create its temporary AO render-target texture.");
    ThrowIfSsaoResourceIsNull(m_rtAoTempTex,
                              "PostEffectSSAO temporary AO texture creation returned a null texture.");
}

void PostEffectSSAO::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                           LPDIRECT3DTEXTURE9 texTarget,
                           LPDIRECT3DTEXTURE9 texRenderTargetZ,
                           LPDIRECT3DTEXTURE9 texRenderTargetPosition,
                           LPDIRECT3DTEXTURE9 texRenderTargetNormal,
                           LPDIRECT3DTEXTURE9 texRenderTargetThickness)
{
    if (!m_isInitialized || m_fxSSAO == NULL)
    {
        throw std::runtime_error("PostEffectSSAO was drawn before successful initialization.");
    }

    ThrowIfSsaoResourceIsNull(renderTarget, "PostEffectSSAO source texture is null.");
    ThrowIfSsaoResourceIsNull(texTarget, "PostEffectSSAO target texture is null.");
    ThrowIfSsaoResourceIsNull(texRenderTargetZ, "PostEffectSSAO depth texture is null.");
    ThrowIfSsaoResourceIsNull(texRenderTargetPosition, "PostEffectSSAO position texture is null.");
    ThrowIfSsaoResourceIsNull(texRenderTargetNormal, "PostEffectSSAO normal texture is null.");
    ThrowIfSsaoResourceIsNull(texRenderTargetThickness, "PostEffectSSAO thickness texture is null.");
    ThrowIfSsaoResourceIsNull(m_rtAoTex, "PostEffectSSAO AO texture is null.");
    ThrowIfSsaoResourceIsNull(m_rtAoTempTex, "PostEffectSSAO temporary AO texture is null.");

    D3DSURFACE_DESC descAO = { };
    ThrowIfSsaoCallFailed(m_rtAoTex->GetLevelDesc(0, &descAO),
                          "PostEffectSSAO failed to get the AO texture description.");
    D3DXVECTOR2 invSize(1.0f / static_cast<float>(descAO.Width),
                        1.0f / static_cast<float>(descAO.Height));

    D3DXMATRIX matrixView = Camera::GetViewMatrix();
    D3DXMATRIX matrixProj = Camera::GetProjMatrix();

    LPDIRECT3DSURFACE9 oldRt0 = NULL;
    ThrowIfSsaoCallFailed(Common::D3DDevice()->GetRenderTarget(0, &oldRt0),
                          "PostEffectSSAO failed to get the current render target.");
    ThrowIfSsaoResourceIsNull(oldRt0, "PostEffectSSAO current render target is null.");

    D3DVIEWPORT9 oldViewport { };
    ThrowIfSsaoCallFailed(Common::D3DDevice()->GetViewport(&oldViewport),
                          "PostEffectSSAO failed to get the current viewport.");

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

    ThrowIfSsaoCallFailed(m_rtAoTex->GetSurfaceLevel(0, &surfAO),
                          "PostEffectSSAO failed to get the AO render-target surface.");
    ThrowIfSsaoCallFailed(m_rtAoTempTex->GetSurfaceLevel(0, &surfAOTemp),
                          "PostEffectSSAO failed to get the temporary AO render-target surface.");
    ThrowIfSsaoCallFailed(texTarget->GetSurfaceLevel(0, &surfRenderTarget),
                          "PostEffectSSAO failed to get the output render-target surface.");
    ThrowIfSsaoResourceIsNull(surfAO, "PostEffectSSAO AO render-target surface is null.");
    ThrowIfSsaoResourceIsNull(surfAOTemp, "PostEffectSSAO temporary AO render-target surface is null.");
    ThrowIfSsaoResourceIsNull(surfRenderTarget, "PostEffectSSAO output render-target surface is null.");

    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetRenderTarget(0, surfAO),
                          "PostEffectSSAO failed to set the AO render target.");
    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetViewport(&aoViewport),
                          "PostEffectSSAO failed to set the AO viewport.");
    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetRenderTarget(1, NULL),
                          "PostEffectSSAO failed to clear the secondary render target.");
    ThrowIfSsaoCallFailed(
        Common::D3DDevice()->Clear(0,
                                   NULL,
                                   D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                   D3DCOLOR_RGBA(255, 255, 255, 255),
                                   1.0f,
                                   0),
        "PostEffectSSAO failed to clear the AO render target.");

    ThrowIfSsaoCallFailed(m_fxSSAO->SetMatrix("g_matView", &matrixView),
                          "PostEffectSSAO failed to set g_matView.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetMatrix("g_matProj", &matrixProj),
                          "PostEffectSSAO failed to set g_matProj.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_fNear", m_nearPlane),
                          "PostEffectSSAO failed to set g_fNear.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_fFar", m_farPlane),
                          "PostEffectSSAO failed to set g_fFar.");
    ThrowIfSsaoCallFailed(
        m_fxSSAO->SetFloatArray("g_invSize", reinterpret_cast<FLOAT*>(&invSize), 2),
        "PostEffectSSAO failed to set g_invSize.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texZ", texRenderTargetZ),
                           "PostEffectSSAO failed to set texZ.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texPosition", texRenderTargetPosition),
                          "PostEffectSSAO failed to set texPosition.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texNormal", texRenderTargetNormal),
                          "PostEffectSSAO failed to set texNormal.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texThickness", texRenderTargetThickness),
                          "PostEffectSSAO failed to set texThickness.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_sampleRadius", m_sampleRadius),
                          "PostEffectSSAO failed to set g_sampleRadius.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetInt("g_sampleCount", m_sampleCount),
                          "PostEffectSSAO failed to set g_sampleCount.");
    ThrowIfSsaoCallFailed(
        m_fxSSAO->SetBool("g_useRandomSamplingDirection", m_randomSamplingDirectionEnabled),
        "PostEffectSSAO failed to set g_useRandomSamplingDirection.");
    ThrowIfSsaoCallFailed(
        m_fxSSAO->SetBool("g_enableDepthScaledSampleDistance", m_depthScaledSampleDistanceEnabled),
        "PostEffectSSAO failed to set g_enableDepthScaledSampleDistance.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_depthCompareThreshold", 0.0f),
                          "PostEffectSSAO failed to set g_depthCompareThreshold.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_depthBiasScale", 1.0f),
                          "PostEffectSSAO failed to set g_depthBiasScale.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_normalBiasScale", 1.0f),
                          "PostEffectSSAO failed to set g_normalBiasScale.");

    ThrowIfSsaoCallFailed(m_fxSSAO->SetTechnique(GetCreateTechniqueName()),
                          "PostEffectSSAO failed to set the AO creation technique.");
    ThrowIfSsaoCallFailed(m_fxSSAO->Begin(NULL, 0), "PostEffectSSAO failed to begin its AO effect.");
    ThrowIfSsaoCallFailed(m_fxSSAO->BeginPass(0), "PostEffectSSAO failed to begin its AO pass.");
    DrawFullscreenQuad();
    ThrowIfSsaoCallFailed(m_fxSSAO->EndPass(), "PostEffectSSAO failed to end its AO pass.");
    ThrowIfSsaoCallFailed(m_fxSSAO->End(), "PostEffectSSAO failed to end its AO effect.");

    if (m_blurEnabled)
    {
        if (m_separableBlurEnabled)
        {
            ThrowIfSsaoCallFailed(Common::D3DDevice()->SetRenderTarget(0, surfAOTemp),
                                  "PostEffectSSAO failed to set the horizontal-blur render target.");
            ThrowIfSsaoCallFailed(
                Common::D3DDevice()->Clear(0,
                                           NULL,
                                           D3DCLEAR_TARGET,
                                           D3DCOLOR_RGBA(255, 255, 255, 255),
                                           1.0f,
                                           0),
                "PostEffectSSAO failed to clear the horizontal-blur render target.");
            ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texAO", m_rtAoTex),
                                  "PostEffectSSAO failed to set texAO for horizontal blur.");
            ThrowIfSsaoCallFailed(m_fxSSAO->SetTechnique(GetHorizontalBlurTechniqueName()),
                                  "PostEffectSSAO failed to set the horizontal-blur technique.");
            ThrowIfSsaoCallFailed(m_fxSSAO->Begin(NULL, 0),
                                  "PostEffectSSAO failed to begin its horizontal-blur effect.");
            ThrowIfSsaoCallFailed(m_fxSSAO->BeginPass(0),
                                  "PostEffectSSAO failed to begin its horizontal-blur pass.");
            DrawFullscreenQuad();
            ThrowIfSsaoCallFailed(m_fxSSAO->EndPass(),
                                  "PostEffectSSAO failed to end its horizontal-blur pass.");
            ThrowIfSsaoCallFailed(m_fxSSAO->End(),
                                  "PostEffectSSAO failed to end its horizontal-blur effect.");

            ThrowIfSsaoCallFailed(Common::D3DDevice()->SetRenderTarget(0, surfAO),
                                  "PostEffectSSAO failed to set the vertical-blur render target.");
            ThrowIfSsaoCallFailed(
                Common::D3DDevice()->Clear(0,
                                           NULL,
                                           D3DCLEAR_TARGET,
                                           D3DCOLOR_RGBA(255, 255, 255, 255),
                                           1.0f,
                                           0),
                "PostEffectSSAO failed to clear the vertical-blur render target.");
            ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texAO", m_rtAoTempTex),
                                  "PostEffectSSAO failed to set texAO for vertical blur.");
            ThrowIfSsaoCallFailed(m_fxSSAO->SetTechnique(GetVerticalBlurTechniqueName()),
                                  "PostEffectSSAO failed to set the vertical-blur technique.");
            ThrowIfSsaoCallFailed(m_fxSSAO->Begin(NULL, 0),
                                  "PostEffectSSAO failed to begin its vertical-blur effect.");
            ThrowIfSsaoCallFailed(m_fxSSAO->BeginPass(0),
                                  "PostEffectSSAO failed to begin its vertical-blur pass.");
            DrawFullscreenQuad();
            ThrowIfSsaoCallFailed(m_fxSSAO->EndPass(),
                                  "PostEffectSSAO failed to end its vertical-blur pass.");
            ThrowIfSsaoCallFailed(m_fxSSAO->End(),
                                  "PostEffectSSAO failed to end its vertical-blur effect.");
            aoTextureForComposite = m_rtAoTex;
        }
        else
        {
            ThrowIfSsaoCallFailed(Common::D3DDevice()->SetRenderTarget(0, surfAOTemp),
                                  "PostEffectSSAO failed to set the blur render target.");
            ThrowIfSsaoCallFailed(
                Common::D3DDevice()->Clear(0,
                                           NULL,
                                           D3DCLEAR_TARGET,
                                           D3DCOLOR_RGBA(255, 255, 255, 255),
                                           1.0f,
                                           0),
                "PostEffectSSAO failed to clear the blur render target.");
            ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texAO", m_rtAoTex),
                                  "PostEffectSSAO failed to set texAO for blur.");
            ThrowIfSsaoCallFailed(m_fxSSAO->SetTechnique(GetBlurTechniqueName()),
                                  "PostEffectSSAO failed to set the blur technique.");
            ThrowIfSsaoCallFailed(m_fxSSAO->Begin(NULL, 0),
                                  "PostEffectSSAO failed to begin its blur effect.");
            ThrowIfSsaoCallFailed(m_fxSSAO->BeginPass(0),
                                  "PostEffectSSAO failed to begin its blur pass.");
            DrawFullscreenQuad();
            ThrowIfSsaoCallFailed(m_fxSSAO->EndPass(),
                                  "PostEffectSSAO failed to end its blur pass.");
            ThrowIfSsaoCallFailed(m_fxSSAO->End(),
                                  "PostEffectSSAO failed to end its blur effect.");
            aoTextureForComposite = m_rtAoTempTex;
        }
    }

    D3DSURFACE_DESC descTarget = { };
    ThrowIfSsaoCallFailed(texTarget->GetLevelDesc(0, &descTarget),
                          "PostEffectSSAO failed to get the target texture description.");
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

    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetRenderTarget(0, surfRenderTarget),
                          "PostEffectSSAO failed to set the composite render target.");
    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetViewport(&targetViewport),
                          "PostEffectSSAO failed to set the composite viewport.");
    ThrowIfSsaoCallFailed(
        Common::D3DDevice()->Clear(0,
                                   NULL,
                                   D3DCLEAR_TARGET,
                                   D3DCOLOR_RGBA(0, 0, 0, 255),
                                   1.0f,
                                   0),
        "PostEffectSSAO failed to clear the composite render target.");
    ThrowIfSsaoCallFailed(
        m_fxSSAO->SetFloatArray("g_invSize", reinterpret_cast<FLOAT*>(&targetInvSize), 2),
        "PostEffectSSAO failed to set composite g_invSize.");
    ThrowIfSsaoCallFailed(
        m_fxSSAO->SetFloatArray("g_aoInvSize", reinterpret_cast<FLOAT*>(&aoInvSize), 2),
        "PostEffectSSAO failed to set g_aoInvSize.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texColor", renderTarget),
                          "PostEffectSSAO failed to set texColor.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetTexture("texAO", aoTextureForComposite),
                          "PostEffectSSAO failed to set composite texAO.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_shadowStrength", m_shadowStrength),
                          "PostEffectSSAO failed to set g_shadowStrength.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetFloat("g_aoSaturationBoost", m_saturationBoost),
                          "PostEffectSSAO failed to set g_aoSaturationBoost.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetBool("g_enableMaxDarknessClamp", m_maxDarknessClampEnabled),
                          "PostEffectSSAO failed to set g_enableMaxDarknessClamp.");
    ThrowIfSsaoCallFailed(m_fxSSAO->SetTechnique(GetCompositeTechniqueName()),
                          "PostEffectSSAO failed to set the composite technique.");
    ThrowIfSsaoCallFailed(m_fxSSAO->Begin(NULL, 0),
                          "PostEffectSSAO failed to begin its composite effect.");
    ThrowIfSsaoCallFailed(m_fxSSAO->BeginPass(0),
                          "PostEffectSSAO failed to begin its composite pass.");
    DrawFullscreenQuad();
    ThrowIfSsaoCallFailed(m_fxSSAO->EndPass(),
                          "PostEffectSSAO failed to end its composite pass.");
    ThrowIfSsaoCallFailed(m_fxSSAO->End(),
                          "PostEffectSSAO failed to end its composite effect.");
    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetRenderTarget(0, oldRt0),
                          "PostEffectSSAO failed to restore the render target.");
    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetViewport(&oldViewport),
                          "PostEffectSSAO failed to restore the viewport.");

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
    ThrowIfSsaoCallFailed(Common::D3DDevice()->CreateVertexDeclaration(decl, &vertexDecl),
                          "PostEffectSSAO failed to create its fullscreen vertex declaration.");
    ThrowIfSsaoResourceIsNull(vertexDecl,
                              "PostEffectSSAO fullscreen vertex declaration creation returned null.");
    ThrowIfSsaoCallFailed(Common::D3DDevice()->SetVertexDeclaration(vertexDecl),
                          "PostEffectSSAO failed to set its fullscreen vertex declaration.");
    ThrowIfSsaoCallFailed(
        Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,
                                              2,
                                              vertices,
                                              sizeof(FullscreenVertex)),
        "PostEffectSSAO failed to draw its fullscreen quad.");
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

void PostEffectSSAO::SetSeparableBlurEnabled(const bool enabled)
{
    m_separableBlurEnabled = enabled;
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

const char* PostEffectSSAO::GetHorizontalBlurTechniqueName() const
{
    if (m_blurKernelSize == 3)
    {
        return "TechniqueAO_Blur3x1";
    }

    if (m_blurKernelSize == 5)
    {
        return "TechniqueAO_Blur5x1";
    }

    if (m_blurKernelSize == 11)
    {
        return "TechniqueAO_Blur11x1";
    }

    return "TechniqueAO_Blur21x1";
}

const char* PostEffectSSAO::GetVerticalBlurTechniqueName() const
{
    if (m_blurKernelSize == 3)
    {
        return "TechniqueAO_Blur1x3";
    }

    if (m_blurKernelSize == 5)
    {
        return "TechniqueAO_Blur1x5";
    }

    if (m_blurKernelSize == 11)
    {
        return "TechniqueAO_Blur1x11";
    }

    return "TechniqueAO_Blur1x21";
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
        ThrowIfSsaoCallFailed(m_fxSSAO->OnLostDevice(),
                              "PostEffectSSAO failed to process device loss.");
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
        ThrowIfSsaoCallFailed(m_fxSSAO->OnResetDevice(),
                              "PostEffectSSAO failed to process device reset.");
    }
    CreateResources();
}

}
