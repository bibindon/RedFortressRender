#include "PostEffectMotionBlurCamera.h"

#include <algorithm>
#include <cmath>

#include "Camera.h"

namespace NSRender
{

namespace
{
int ClampMotionBlurQuality(const int quality)
{
    return (std::max)(1, (std::min)(quality, 8));
}

float ClampUnit(const float value)
{
    return (std::max)(-1.0f, (std::min)(value, 1.0f));
}

D3DXVECTOR3 GetCameraDirection(const D3DXVECTOR3& eye, const D3DXVECTOR3& lookAt)
{
    D3DXVECTOR3 direction = lookAt - eye;
    const float length = D3DXVec3Length(&direction);
    if (length <= 0.0001f)
    {
        return D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    }

    D3DXVec3Normalize(&direction, &direction);
    return direction;
}

float NormalizeAngle(const float angle)
{
    float normalized = angle;
    while (normalized > D3DX_PI)
    {
        normalized -= D3DX_PI * 2.0f;
    }

    while (normalized < -D3DX_PI)
    {
        normalized += D3DX_PI * 2.0f;
    }

    return normalized;
}

void GetYawPitchFromDirection(const D3DXVECTOR3& direction, float& yaw, float& pitch)
{
    yaw = atan2f(direction.x, direction.z);
    pitch = asinf(ClampUnit(-direction.y));
}

D3DXVECTOR3 GetDirectionFromYawPitch(const float yaw, const float pitch)
{
    D3DXVECTOR3 direction(cosf(pitch) * sinf(yaw),
                          -sinf(pitch),
                          cosf(pitch) * cosf(yaw));
    D3DXVec3Normalize(&direction, &direction);
    return direction;
}
}

void PostEffectMotionBlurCamera::Initialize()
{
    HRESULT hResult = D3DXCreateEffectFromFile(Common::D3DDevice(),
                                               L"../x64/Debug/PostEffectMotionBlurCamera.cso",
                                               NULL,
                                               NULL,
                                               D3DXSHADER_DEBUG,
                                               NULL,
                                               &m_d3dEffect,
                                               NULL);
    assert(SUCCEEDED(hResult));

    D3DXMatrixIdentity(&m_prevViewProj);
    D3DXMatrixIdentity(&m_motionBlurPrevViewProj);
    m_prevFrameTick = GetTickCount64();
    CreateTexture();
    Common::AddDeviceLostResource(this);
}

void PostEffectMotionBlurCamera::Finalize()
{
    SAFE_RELEASE(m_texWork);
    SAFE_RELEASE(m_d3dEffect);
}

void PostEffectMotionBlurCamera::CreateTexture()
{
    HRESULT hResult = D3DXCreateTexture(Common::D3DDevice(),
                                        Common::ScreenW(),
                                        Common::ScreenH(),
                                        1,
                                        D3DUSAGE_RENDERTARGET,
                                        D3DFMT_A16B16G16R16F,
                                        D3DPOOL_DEFAULT,
                                        &m_texWork);
    assert(SUCCEEDED(hResult));
}

LPDIRECT3DTEXTURE9 PostEffectMotionBlurCamera::Draw(LPDIRECT3DTEXTURE9 renderTarget,
                                                    LPDIRECT3DTEXTURE9 depthTexture)
{
    const D3DXMATRIX currentViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_frameMotionScale = UpdateFrameMotionScale();
    if (!ShouldApplyMotionBlur(currentViewProj))
    {
        UpdateFrameMatrices();
        return renderTarget;
    }

    UpdateMotionBlurPrevViewProj();
    DrawFullscreenQuad(renderTarget, depthTexture, m_texWork);
    UpdateFrameMatrices();
    return m_texWork;
}

void PostEffectMotionBlurCamera::UpdateFrameMatrices()
{
    m_prevViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    m_prevEye = Camera::GetEyePos();
    m_prevLookAt = Camera::GetLookAtPos();
    m_hasPrevViewProj = true;
}

void PostEffectMotionBlurCamera::SetQuality(const int quality)
{
    m_quality = ClampMotionBlurQuality(quality);
}

int PostEffectMotionBlurCamera::GetQuality() const
{
    return m_quality;
}

bool PostEffectMotionBlurCamera::ShouldApplyMotionBlur(const D3DXMATRIX& currentViewProj)
{
    if (!m_hasPrevViewProj)
    {
        m_prevViewProj = currentViewProj;
        return false;
    }

    const D3DXVECTOR3 currentEye = Camera::GetEyePos();
    const D3DXVECTOR3 currentLookAt = Camera::GetLookAtPos();

    D3DXVECTOR3 lookAtMotion = currentLookAt - m_prevLookAt;
    const float lookAtMotionLength = D3DXVec3Length(&lookAtMotion);
    const D3DXVECTOR3 prevEyeOffset = m_prevEye - m_prevLookAt;
    const D3DXVECTOR3 currentEyeOffset = currentEye - currentLookAt;
    const float distanceMotion = fabsf(D3DXVec3Length(&currentEyeOffset) - D3DXVec3Length(&prevEyeOffset));

    const D3DXVECTOR3 prevDirection = GetCameraDirection(m_prevEye, m_prevLookAt);
    const D3DXVECTOR3 currentDirection = GetCameraDirection(currentEye, currentLookAt);
    float prevYaw = 0.0f;
    float prevPitch = 0.0f;
    float currentYaw = 0.0f;
    float currentPitch = 0.0f;
    GetYawPitchFromDirection(prevDirection, prevYaw, prevPitch);
    GetYawPitchFromDirection(currentDirection, currentYaw, currentPitch);
    const float yawMotion = NormalizeAngle(currentYaw - prevYaw);
    const float pitchMotion = currentPitch - prevPitch;
    const float rotationMotion = (std::max)(fabsf(yawMotion), fabsf(pitchMotion));

    static const float kMotionBlurTranslationThreshold = 0.001f;
    static const float kMotionBlurRotationThreshold = 0.01f;
    return (lookAtMotionLength > kMotionBlurTranslationThreshold) ||
           (distanceMotion > kMotionBlurTranslationThreshold) ||
           (rotationMotion > kMotionBlurRotationThreshold);
}

float PostEffectMotionBlurCamera::UpdateFrameMotionScale()
{
    const ULONGLONG currentTick = GetTickCount64();
    float deltaSeconds = static_cast<float>(currentTick - m_prevFrameTick) / 1000.0f;
    m_prevFrameTick = currentTick;

    if (deltaSeconds > 0.1f)
    {
        deltaSeconds = 0.1f;
    }

    static const float kMotionVectorFrameSeconds = 1.0f / 60.0f;
    return (std::min)(1.0f, kMotionVectorFrameSeconds / (std::max)(deltaSeconds, 0.0001f));
}

void PostEffectMotionBlurCamera::UpdateMotionBlurPrevViewProj()
{
    const float motionScale = m_frameMotionScale;
    const D3DXVECTOR3 currentEye = Camera::GetEyePos();
    const D3DXVECTOR3 currentLookAt = Camera::GetLookAtPos();

    const D3DXVECTOR3 targetMotion = currentLookAt - m_prevLookAt;
    const float targetMotionLength = D3DXVec3Length(&targetMotion);
    const D3DXVECTOR3 prevEyeOffset = m_prevEye - m_prevLookAt;
    const D3DXVECTOR3 currentEyeOffset = currentEye - currentLookAt;
    const float prevDistance = D3DXVec3Length(&prevEyeOffset);
    const float currentDistance = D3DXVec3Length(&currentEyeOffset);
    const float distanceMotion = currentDistance - prevDistance;

    const D3DXVECTOR3 prevDirection = GetCameraDirection(m_prevEye, m_prevLookAt);
    const D3DXVECTOR3 currentDirection = GetCameraDirection(currentEye, currentLookAt);
    float prevYaw = 0.0f;
    float prevPitch = 0.0f;
    float currentYaw = 0.0f;
    float currentPitch = 0.0f;
    GetYawPitchFromDirection(prevDirection, prevYaw, prevPitch);
    GetYawPitchFromDirection(currentDirection, currentYaw, currentPitch);
    const float yawMotion = NormalizeAngle(currentYaw - prevYaw);
    const float pitchMotion = currentPitch - prevPitch;
    const float rotationMotion = (std::max)(fabsf(yawMotion), fabsf(pitchMotion));

    static const float kMotionBlurTranslationThreshold = 0.001f;
    static const float kMotionBlurRotationThreshold = 0.01f;
    static const float kRotationOnlyBlurScale = 0.3f;
    const bool hasTranslationMotion =
        (targetMotionLength > kMotionBlurTranslationThreshold) ||
        (fabsf(distanceMotion) > kMotionBlurTranslationThreshold);
    const bool hasRotationMotion = (rotationMotion > kMotionBlurRotationThreshold);

    m_motionBlurScaleThisFrame = hasTranslationMotion ? 1.0f : kRotationOnlyBlurScale;

    const D3DXVECTOR3 prevTargetForBlur = currentLookAt - targetMotion * motionScale;
    const float prevDistanceForBlur = currentDistance - distanceMotion * motionScale;
    const float prevYawForBlur = currentYaw - yawMotion * motionScale;
    const float prevPitchForBlur = currentPitch - pitchMotion * motionScale;
    const D3DXVECTOR3 prevDirectionForBlur = hasRotationMotion ?
        GetDirectionFromYawPitch(prevYawForBlur, prevPitchForBlur) :
        currentDirection;
    const D3DXVECTOR3 prevEyeForBlur = prevTargetForBlur - prevDirectionForBlur * prevDistanceForBlur;
    const D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);

    D3DXMATRIX prevView { };
    D3DXMatrixLookAtLH(&prevView, &prevEyeForBlur, &prevTargetForBlur, &up);
    m_motionBlurPrevViewProj = prevView * Camera::GetProjMatrix();
}

void PostEffectMotionBlurCamera::DrawFullscreenQuad(LPDIRECT3DTEXTURE9 texSource,
                                                    LPDIRECT3DTEXTURE9 depthTexture,
                                                    LPDIRECT3DTEXTURE9 texTarget)
{
    D3DXMATRIX currentViewProj = Camera::GetViewMatrix() * Camera::GetProjMatrix();
    D3DXMATRIX invCurrentViewProj { };
    D3DXMATRIX identity { };
    D3DXMatrixIdentity(&identity);
    if (D3DXMatrixInverse(&invCurrentViewProj, NULL, &currentViewProj) == NULL)
    {
        invCurrentViewProj = identity;
    }

    if (!m_hasPrevViewProj)
    {
        m_prevViewProj = currentViewProj;
        m_motionBlurPrevViewProj = currentViewProj;
    }

    const D3DXVECTOR4 texelSize(1.0f / static_cast<float>(Common::ScreenW()),
                                1.0f / static_cast<float>(Common::ScreenH()),
                                static_cast<float>(Common::ScreenW()),
                                static_cast<float>(Common::ScreenH()));

    const float blurScale = 2.0f * m_motionBlurScaleThisFrame;
    const float maxBlurPixels = static_cast<float>(m_quality) * 6.0f;
    const int sampleCount = (std::min)(21, 5 + m_quality * 2);

    m_d3dEffect->SetTexture("texture1", texSource);
    m_d3dEffect->SetTexture("depthTexture", depthTexture);
    m_d3dEffect->SetMatrix("g_matInvCurrentViewProj", &invCurrentViewProj);
    m_d3dEffect->SetMatrix("g_matPrevViewProj", &m_motionBlurPrevViewProj);
    m_d3dEffect->SetFloat("g_fBlurScale", blurScale);
    m_d3dEffect->SetFloat("g_fMaxBlurPixels", maxBlurPixels);
    m_d3dEffect->SetInt("g_iSampleCount", sampleCount);
    m_d3dEffect->SetInt("g_iMotionBlurEnabled", 1);
    m_d3dEffect->SetInt("g_iDebugGridEnabled", 0);
    m_d3dEffect->SetVector("g_vTexelSize", &texelSize);
    m_d3dEffect->SetFloat("g_fNear", Camera::GetNear());
    m_d3dEffect->SetFloat("g_fFar", Camera::GetFar());

    LPDIRECT3DSURFACE9 pSceneRT = NULL;
    texTarget->GetSurfaceLevel(0, &pSceneRT);
    Common::D3DDevice()->SetRenderTarget(0, pSceneRT);
    SAFE_RELEASE(pSceneRT);

    Common::D3DDevice()->SetVertexShader(NULL);
    m_d3dEffect->SetTechnique("Technique1");

    ScreenVertex quad[4] { };

    quad[0].x   = -0.5f;
    quad[0].y   = -0.5f;
    quad[0].z   = 0.0f;
    quad[0].rhw = 1.0f;
    quad[0].u   = 0.0f;
    quad[0].v   = 0.0f;

    quad[1].x   = -0.5f + Common::ScreenW();
    quad[1].y   = -0.5f;
    quad[1].z   = 0.0f;
    quad[1].rhw = 1.0f;
    quad[1].u   = 1.0f;
    quad[1].v   = 0.0f;

    quad[2].x   = -0.5f;
    quad[2].y   = -0.5f + Common::ScreenH();
    quad[2].z   = 0.0f;
    quad[2].rhw = 1.0f;
    quad[2].u   = 0.0f;
    quad[2].v   = 1.0f;

    quad[3].x   = -0.5f + Common::ScreenW();
    quad[3].y   = -0.5f + Common::ScreenH();
    quad[3].z   = 0.0f;
    quad[3].rhw = 1.0f;
    quad[3].u   = 1.0f;
    quad[3].v   = 1.0f;

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

void PostEffectMotionBlurCamera::OnDeviceLost()
{
    m_d3dEffect->OnLostDevice();
    SAFE_RELEASE(m_texWork);
}

void PostEffectMotionBlurCamera::OnDeviceReset()
{
    m_d3dEffect->OnResetDevice();
    CreateTexture();
}

}
