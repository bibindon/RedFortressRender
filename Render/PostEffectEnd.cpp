#include "PostEffectEnd.h"

namespace NSRender
{

void PostEffectEnd::Initialize()
{
    HRESULT hResult = E_FAIL;

    hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                       L"../x64/Debug/PostEffectEnd.cso",
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &m_d3dEffect,
                                       NULL);
    assert(SUCCEEDED(hResult));
    Common::AddDeviceLostResource(this);
}

void PostEffectEnd::Finalize()
{
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectEnd::Draw(LPDIRECT3DTEXTURE9 renderTarget)
{
    LPDIRECT3DSURFACE9 pBackBuffer = NULL;
    Common::D3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    Common::D3DDevice()->SetRenderTarget(0, pBackBuffer);
    SAFE_RELEASE(pBackBuffer);

    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    Common::D3DDevice()->BeginScene();

    DrawScreenQuad(renderTarget,
                   -0.5f,
                   -0.5f,
                   -0.5f + static_cast<float>(Common::ScreenW()),
                   -0.5f + static_cast<float>(Common::ScreenH()),
                   "Copy");

    Common::D3DDevice()->EndScene();
}

void PostEffectEnd::DrawSingleChannel(LPDIRECT3DTEXTURE9 renderTarget)
{
    LPDIRECT3DSURFACE9 pBackBuffer = NULL;
    Common::D3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    Common::D3DDevice()->SetRenderTarget(0, pBackBuffer);
    SAFE_RELEASE(pBackBuffer);

    Common::D3DDevice()->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
    Common::D3DDevice()->BeginScene();

    DrawScreenQuad(renderTarget,
                   -0.5f,
                   -0.5f,
                   -0.5f + static_cast<float>(Common::ScreenW()),
                   -0.5f + static_cast<float>(Common::ScreenH()),
                   "CopySingleChannel");

    Common::D3DDevice()->EndScene();
}

void PostEffectEnd::DrawOverlay(LPDIRECT3DTEXTURE9 renderTarget,
                                const int x,
                                const int y,
                                const int width,
                                const int height)
{
    if (renderTarget == NULL)
    {
        return;
    }

    Common::D3DDevice()->BeginScene();

    DrawScreenQuad(renderTarget,
                   -0.5f + static_cast<float>(x),
                   -0.5f + static_cast<float>(y),
                   -0.5f + static_cast<float>(x + width),
                   -0.5f + static_cast<float>(y + height),
                   "Copy");

    Common::D3DDevice()->EndScene();
}

void PostEffectEnd::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
}

void PostEffectEnd::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();
}

void PostEffectEnd::DrawScreenQuad(LPDIRECT3DTEXTURE9 texTarget,
                                   const float left,
                                   const float top,
                                   const float right,
                                   const float bottom,
                                   const std::string& technique)
{
    ScreenVertex quad[4] { };

    quad[0].x = left;
    quad[0].y = top;
    quad[0].z = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u = 0.0f;
    quad[0].v = 0.0f;

    quad[1].x = right;
    quad[1].y = top;
    quad[1].z = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u = 1.0f;
    quad[1].v = 0.0f;

    quad[2].x = left;
    quad[2].y = bottom;
    quad[2].z = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u = 0.0f;
    quad[2].v = 1.0f;

    quad[3].x = right;
    quad[3].y = bottom;
    quad[3].z = 0.0f;
    quad[3].rhw = 1.0f;
    quad[3].u = 1.0f;
    quad[3].v = 1.0f;

    m_d3dEffect->SetTechnique(technique.c_str());
    m_d3dEffect->SetTexture("g_SrcTex", texTarget);

    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, FALSE);
    Common::D3DDevice()->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    m_d3dEffect->Begin(NULL, 0);
    m_d3dEffect->BeginPass(0);
    Common::D3DDevice()->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(ScreenVertex));
    m_d3dEffect->EndPass();
    m_d3dEffect->End();
    Common::D3DDevice()->SetRenderState(D3DRS_ZENABLE, TRUE);
}

}
