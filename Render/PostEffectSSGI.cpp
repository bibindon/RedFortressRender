#include "PostEffectSSGI.h"

#include "Camera.h"

#include "Util.h"

namespace NSRender
{

void PostEffectSSGI::Initialize()
{
    if (m_isInitialized)
    {
        return;
    }

    const std::wstring effectPath = Util::GetExeDir() + L"PostEffectSSGI.cso";
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               effectPath.c_str(),
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_fxSSGI,
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

void PostEffectSSGI::Finalize()
{
    if (m_isRegisteredForDeviceReset)
    {
        Common::RemoveDeviceLostResource(this);
        m_isRegisteredForDeviceReset = false;
    }

    SAFE_RELEASE(m_rtGiTex);
    SAFE_RELEASE(m_rtGiTempTex);
    SAFE_RELEASE(m_fxSSGI);
    m_isInitialized = false;
}

void PostEffectSSGI::CreateResources()
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
                                        &m_rtGiTex);
    assert(SUCCEEDED(hResult));

    hResult = D3DXCreateTexture(Common::D3DDevice(),
                                textureWidth,
                                textureHeight,
                                1,
                                D3DUSAGE_RENDERTARGET,
                                D3DFMT_A16B16G16R16F,
                                D3DPOOL_DEFAULT,
                                &m_rtGiTempTex);
    assert(SUCCEEDED(hResult));
}

void PostEffectSSGI::Draw(LPDIRECT3DTEXTURE9 texSource,
                          LPDIRECT3DTEXTURE9 texTarget,
                          LPDIRECT3DTEXTURE9 texRenderTargetZ,
                          LPDIRECT3DTEXTURE9 texRenderTargetPos,
                          LPDIRECT3DTEXTURE9 texRenderTargetNormal,
                          LPDIRECT3DTEXTURE9 texRenderTargetThickness)
{
    UNREFERENCED_PARAMETER(texRenderTargetPos);

    if (!m_isInitialized || m_fxSSGI == NULL)
    {
        return;
    }

    D3DSURFACE_DESC descGi = { };
    m_rtGiTex->GetLevelDesc(0, &descGi);
    D3DXVECTOR2 giInvSize(1.0f / static_cast<float>(descGi.Width),
                          1.0f / static_cast<float>(descGi.Height));

    D3DXMATRIX matrixView = Camera::GetViewMatrix();
    D3DXMATRIX matrixProj = Camera::GetProjMatrix();

    LPDIRECT3DSURFACE9 oldRt0 = NULL;
    Common::D3DDevice()->GetRenderTarget(0, &oldRt0);

    D3DVIEWPORT9 oldViewport = { };
    Common::D3DDevice()->GetViewport(&oldViewport);

    D3DVIEWPORT9 giViewport = { };
    giViewport.X = 0;
    giViewport.Y = 0;
    giViewport.Width = descGi.Width;
    giViewport.Height = descGi.Height;
    giViewport.MinZ = 0.0f;
    giViewport.MaxZ = 1.0f;

    LPDIRECT3DSURFACE9 surfGi = NULL;
    LPDIRECT3DSURFACE9 surfGiTemp = NULL;
    LPDIRECT3DSURFACE9 surfRenderTarget = NULL;
    LPDIRECT3DTEXTURE9 giTextureForComposite = m_rtGiTex;

    m_rtGiTex->GetSurfaceLevel(0, &surfGi);
    m_rtGiTempTex->GetSurfaceLevel(0, &surfGiTemp);
    texTarget->GetSurfaceLevel(0, &surfRenderTarget);

    Common::D3DDevice()->SetRenderTarget(0, surfGi);
    Common::D3DDevice()->SetViewport(&giViewport);
    Common::D3DDevice()->SetRenderTarget(1, NULL);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);

    m_fxSSGI->SetMatrix("g_matView", &matrixView);
    m_fxSSGI->SetMatrix("g_matProj", &matrixProj);
    m_fxSSGI->SetFloat("g_fNear", m_nearPlane);
    m_fxSSGI->SetFloat("g_fFar", m_farPlane);
    m_fxSSGI->SetFloatArray("g_invSize", reinterpret_cast<FLOAT*>(&giInvSize), 2);
    m_fxSSGI->SetTexture("texZ", texRenderTargetZ);
    m_fxSSGI->SetTexture("texNormal", texRenderTargetNormal);
    m_fxSSGI->SetTexture("texThickness", texRenderTargetThickness);
    m_fxSSGI->SetTexture("texColor", texSource);
    m_fxSSGI->SetFloat("g_sampleRadius", m_sampleRadius);
    m_fxSSGI->SetInt("g_sampleCount", m_sampleCount);
    m_fxSSGI->SetBool("g_enableDepthScaledSampleDistance", m_depthScaledSampleDistanceEnabled);
    m_fxSSGI->SetBool("g_useThickness", m_useThicknessEnabled);
    m_fxSSGI->SetTechnique(GetCreateTechniqueName());
    m_fxSSGI->Begin(NULL, 0);
    m_fxSSGI->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSGI->EndPass();
    m_fxSSGI->End();

    if (m_blurEnabled)
    {
        if (m_separableBlurEnabled)
        {
            Common::D3DDevice()->SetRenderTarget(0, surfGiTemp);
            Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);
            m_fxSSGI->SetTexture("texGI", m_rtGiTex);
            m_fxSSGI->SetTechnique(GetHorizontalBlurTechniqueName());
            m_fxSSGI->Begin(NULL, 0);
            m_fxSSGI->BeginPass(0);
            DrawFullscreenQuad();
            m_fxSSGI->EndPass();
            m_fxSSGI->End();

            Common::D3DDevice()->SetRenderTarget(0, surfGi);
            Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);
            m_fxSSGI->SetTexture("texGI", m_rtGiTempTex);
            m_fxSSGI->SetTechnique(GetVerticalBlurTechniqueName());
            m_fxSSGI->Begin(NULL, 0);
            m_fxSSGI->BeginPass(0);
            DrawFullscreenQuad();
            m_fxSSGI->EndPass();
            m_fxSSGI->End();
            giTextureForComposite = m_rtGiTex;
        }
        else
        {
            Common::D3DDevice()->SetRenderTarget(0, surfGiTemp);
            Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);
            m_fxSSGI->SetTexture("texGI", m_rtGiTex);
            m_fxSSGI->SetTechnique(GetBlurTechniqueName());
            m_fxSSGI->Begin(NULL, 0);
            m_fxSSGI->BeginPass(0);
            DrawFullscreenQuad();
            m_fxSSGI->EndPass();
            m_fxSSGI->End();
            giTextureForComposite = m_rtGiTempTex;
        }
    }

    D3DSURFACE_DESC descTarget = { };
    texTarget->GetLevelDesc(0, &descTarget);

    D3DVIEWPORT9 targetViewport = { };
    targetViewport.X = 0;
    targetViewport.Y = 0;
    targetViewport.Width = descTarget.Width;
    targetViewport.Height = descTarget.Height;
    targetViewport.MinZ = 0.0f;
    targetViewport.MaxZ = 1.0f;

    D3DXVECTOR2 targetInvSize(1.0f / static_cast<float>(descTarget.Width),
                              1.0f / static_cast<float>(descTarget.Height));

    Common::D3DDevice()->SetRenderTarget(0, surfRenderTarget);
    Common::D3DDevice()->SetViewport(&targetViewport);
    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);
    m_fxSSGI->SetFloatArray("g_invSize", reinterpret_cast<FLOAT*>(&targetInvSize), 2);
    m_fxSSGI->SetTexture("texColor", texSource);
    m_fxSSGI->SetTexture("texGI", giTextureForComposite);
    m_fxSSGI->SetFloat("g_indirectLightStrength", m_indirectLightStrength);
    m_fxSSGI->SetFloat("g_indirectLightMaxContribution", m_indirectLightMaxContribution);
    m_fxSSGI->SetTechnique("TechniqueGI_Composite");
    m_fxSSGI->Begin(NULL, 0);
    m_fxSSGI->BeginPass(0);
    DrawFullscreenQuad();
    m_fxSSGI->EndPass();
    m_fxSSGI->End();

    Common::D3DDevice()->SetRenderTarget(0, oldRt0);
    Common::D3DDevice()->SetViewport(&oldViewport);

    SAFE_RELEASE(surfGi);
    SAFE_RELEASE(surfGiTemp);
    SAFE_RELEASE(surfRenderTarget);
    SAFE_RELEASE(oldRt0);
}

void PostEffectSSGI::DrawFullscreenQuad()
{
    static FullscreenVertex vertices[4] =
    {
        { -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f },
        { -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f }
    };

    static D3DVERTEXELEMENT9 decl[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    LPDIRECT3DVERTEXDECLARATION9 vertexDecl = NULL;
    Common::D3DDevice()->CreateVertexDeclaration(decl, &vertexDecl);
    Common::D3DDevice()->SetVertexDeclaration(vertexDecl);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(FullscreenVertex));
    SAFE_RELEASE(vertexDecl);
}

void PostEffectSSGI::SetSampleRadius(const float sampleRadius)
{
    m_sampleRadius = sampleRadius;
}

void PostEffectSSGI::SetSampleCount(const int sampleCount)
{
    m_sampleCount = NormalizeSampleCount(sampleCount);
}

void PostEffectSSGI::SetDepthScaledSampleDistanceEnabled(const bool enabled)
{
    m_depthScaledSampleDistanceEnabled = enabled;
}

void PostEffectSSGI::SetBlurEnabled(const bool enabled)
{
    m_blurEnabled = enabled;
}

void PostEffectSSGI::SetSeparableBlurEnabled(const bool enabled)
{
    m_separableBlurEnabled = enabled;
}

void PostEffectSSGI::SetBlurKernelSize(const int kernelSize)
{
    m_blurKernelSize = NormalizeBlurKernelSize(kernelSize);
}

void PostEffectSSGI::SetDepthRange(const float nearPlane, const float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

void PostEffectSSGI::SetTextureScaleDivisor(const int scaleDivisor)
{
    const int normalizedDivisor = NormalizeTextureScaleDivisor(scaleDivisor);
    if (m_textureScaleDivisor == normalizedDivisor)
    {
        return;
    }

    m_textureScaleDivisor = normalizedDivisor;
    if (m_isInitialized)
    {
        SAFE_RELEASE(m_rtGiTex);
        SAFE_RELEASE(m_rtGiTempTex);
        CreateResources();
    }
}

void PostEffectSSGI::SetIndirectLightStrength(const float strength)
{
    m_indirectLightStrength = strength;
}

void PostEffectSSGI::SetIndirectLightMaxContribution(const float maxContribution)
{
    m_indirectLightMaxContribution = maxContribution;
}

void PostEffectSSGI::SetUseThicknessEnabled(const bool enabled)
{
    m_useThicknessEnabled = enabled;
}

const char* PostEffectSSGI::GetCreateTechniqueName() const
{
    if (m_sampleCount == 4)
    {
        return "TechniqueGI_Create4";
    }

    if (m_sampleCount == 8)
    {
        return "TechniqueGI_Create8";
    }

    if (m_sampleCount == 16)
    {
        return "TechniqueGI_Create16";
    }

    if (m_sampleCount == 32)
    {
        return "TechniqueGI_Create32";
    }

    return "TechniqueGI_Create64";
}

int PostEffectSSGI::NormalizeBlurKernelSize(const int kernelSize) const
{
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

const char* PostEffectSSGI::GetBlurTechniqueName() const
{
    if (m_blurKernelSize == 5)
    {
        return "TechniqueGI_Blur5x5";
    }

    if (m_blurKernelSize == 11)
    {
        return "TechniqueGI_Blur11x11";
    }

    return "TechniqueGI_Blur21x21";
}

const char* PostEffectSSGI::GetHorizontalBlurTechniqueName() const
{
    if (m_blurKernelSize == 5)
    {
        return "TechniqueGI_Blur5x1";
    }

    if (m_blurKernelSize == 11)
    {
        return "TechniqueGI_Blur11x1";
    }

    return "TechniqueGI_Blur21x1";
}

const char* PostEffectSSGI::GetVerticalBlurTechniqueName() const
{
    if (m_blurKernelSize == 5)
    {
        return "TechniqueGI_Blur1x5";
    }

    if (m_blurKernelSize == 11)
    {
        return "TechniqueGI_Blur1x11";
    }

    return "TechniqueGI_Blur1x21";
}

int PostEffectSSGI::NormalizeSampleCount(const int sampleCount) const
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

int PostEffectSSGI::NormalizeTextureScaleDivisor(const int scaleDivisor) const
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

UINT PostEffectSSGI::ComputeTextureSize(const int screenSize) const
{
    return static_cast<UINT>((std::max)(1, screenSize / m_textureScaleDivisor));
}

void PostEffectSSGI::OnDeviceLost()
{
    if (!m_isInitialized || m_fxSSGI == NULL)
    {
        return;
    }

    m_fxSSGI->OnLostDevice();
    SAFE_RELEASE(m_rtGiTex);
    SAFE_RELEASE(m_rtGiTempTex);
}

void PostEffectSSGI::OnDeviceReset()
{
    if (!m_isInitialized || m_fxSSGI == NULL)
    {
        return;
    }

    m_fxSSGI->OnResetDevice();
    CreateResources();
}

}
