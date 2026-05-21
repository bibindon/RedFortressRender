
#include "Camera.h"

#include <cmath>
#include <string>
#include <algorithm>

#include "Common.h"

const D3DXVECTOR3 NSRender::Camera::UPWARD (0.0f, 1.0f, 0.0f);

// m_eyePosに何をセットしても視点は変わらない。視点はm_radianによって決まる。
D3DXVECTOR3 NSRender::Camera::m_eyePos(0.f, 1.f, -2.f);
D3DXVECTOR3 NSRender::Camera::m_lookAtPos(0.0f, 0.0f, 0.0f);
float NSRender::Camera::m_viewAngle = (D3DX_PI / 4);
float NSRender::Camera::m_nearPlane = 0.1f;
float NSRender::Camera::m_farPlane = 30'000.0f;

// m_radian == D3DX_PI * 3 / 2の時（270度の時）カメラは正面を向く
float NSRender::Camera::m_radian = D3DX_PI * 3 / 2;
float NSRender::Camera::m_y = 3.f;
bool NSRender::Camera::m_sleepMode = false;
bool NSRender::Camera::m_houseMode = false;
float NSRender::Camera::m_shakeDurationSeconds = 1.0f;
float NSRender::Camera::m_shakeIntensity = 0.12f;
bool NSRender::Camera::m_shakeActive = false;
ULONGLONG NSRender::Camera::m_shakeStartTick = 0;
bool NSRender::Camera::m_shakeFrameActive = false;
D3DXVECTOR3 NSRender::Camera::m_shakeOffset(0.0f, 0.0f, 0.0f);

NSRender::eCameraMode NSRender::Camera::m_eCameraMode;

int NSRender::Camera::m_counter = 0;
int NSRender::Camera::MOVE_COUNT_MAX = 240;

D3DXMATRIX NSRender::Camera::GetViewMatrix()
{
    const D3DXVECTOR3 eyePos = GetEyePos();
    const D3DXVECTOR3 lookAtPos = GetLookAtPos();
    D3DXMATRIX viewMatrix { };
    D3DXMatrixLookAtLH(&viewMatrix, &eyePos, &lookAtPos, &UPWARD);
    return viewMatrix;
}

D3DXMATRIX NSRender::Camera::GetProjMatrix()
{
    D3DXMATRIX projection_matrix { };
    D3DXMatrixPerspectiveFovLH(&projection_matrix,
                               m_viewAngle,
                               static_cast<float>(1920) / 1080, /* TODO */
                               m_nearPlane,
                               m_farPlane);

    return projection_matrix;
}

void NSRender::Camera::SetLookAtPos(const D3DXVECTOR3& lookAtPos)
{
    m_lookAtPos = lookAtPos;
}

D3DXVECTOR3 NSRender::Camera::GetLookAtPos()
{
    if (m_shakeFrameActive)
    {
        return m_lookAtPos + m_shakeOffset;
    }

    return m_lookAtPos;
}

D3DXVECTOR3 NSRender::Camera::GetEyePos()
{
    if (m_shakeFrameActive)
    {
        return m_eyePos + m_shakeOffset;
    }

    return m_eyePos;
}

void NSRender::Camera::SetEyePos(const D3DXVECTOR3& eye)
{
    m_eyePos = eye;
}

float NSRender::Camera::GetRadian()
{
    return m_radian;
}

void NSRender::Camera::SetRadian(const float radian)
{
    m_radian = radian;
}

void NSRender::Camera::Update()
{
    if (m_eCameraMode == eCameraMode::SLEEP)
    {
        // do nothing
    }
    else if (m_eCameraMode == eCameraMode::TITLE)
    {
        m_eyePos.x = -4000.f;
        m_eyePos.z = -1000.f;
        m_eyePos.y = 300.f;
    }
    else if (m_eCameraMode == eCameraMode::BATTLE)
    {
    }
}

POINT NSRender::Camera::GetScreenPos(const D3DXVECTOR3& world)
{
    const D3DXMATRIX view_matrix { GetViewMatrix() };
    const D3DXMATRIX projection_matrix { GetProjMatrix() };
    static const D3DXMATRIX viewport_matrix
    {
        Common::ScreenW() / 2.0f,
        0.0f,
        0.0f,
        0.0f,

        0.0f,
        -Common::ScreenH() / 2.0f,
        0.0f,
        0.0f,

        0.0f,
        0.0f,
        1.0f,
        0.0f,

        Common::ScreenW() / 2.0f,
        Common::ScreenH() / 2.0f,
        0.0f,
        1.0f
    };

    D3DXMATRIX matrix { };
    D3DXMatrixTranslation(&matrix, world.x, world.y, world.z);
    matrix = matrix * view_matrix * projection_matrix * viewport_matrix;

    POINT p { };
    if (matrix._44 < 0.f)
    {
        p.x = -1;
        p.y = -1;
    }
    else
    {
        p.x = static_cast<int>(matrix._41 / matrix._44);
        p.y = static_cast<int>(matrix._42 / matrix._44);
    }
    return p;
}

void NSRender::Camera::SetCameraMode(const eCameraMode mode)
{
    m_eCameraMode = mode;
    m_counter = 0;
}

void NSRender::Camera::SetHouseMode(const bool arg)
{
    m_houseMode = arg;
}

void NSRender::Camera::SetNear(const float nearPlane)
{
    m_nearPlane = nearPlane;
}

void NSRender::Camera::SetFar(const float farPlane)
{
    m_farPlane = farPlane;
}

void NSRender::Camera::SetClipPlanes(const float nearPlane, const float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
}

float NSRender::Camera::GetNear()
{
    return m_nearPlane;
}

float NSRender::Camera::GetFar()
{
    return m_farPlane;
}

void NSRender::Camera::SetShakeDuration(const float durationSeconds)
{
    m_shakeDurationSeconds = (std::max)(0.1f, (std::min)(durationSeconds, 5.0f));
}

void NSRender::Camera::SetShakeIntensity(const float intensity)
{
    m_shakeIntensity = (std::max)(0.0f, (std::min)(intensity, 1.0f));
}

void NSRender::Camera::TriggerShake()
{
    m_shakeActive = true;
    m_shakeStartTick = GetTickCount64();
}

void NSRender::Camera::BeginShakeFrame()
{
    m_shakeFrameActive = false;
    m_shakeOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    if (!m_shakeActive)
    {
        return;
    }

    const ULONGLONG now = GetTickCount64();
    const float elapsedSeconds = static_cast<float>(now - m_shakeStartTick) / 1000.0f;
    if (elapsedSeconds >= m_shakeDurationSeconds)
    {
        m_shakeActive = false;
        return;
    }

    const float normalizedTime = elapsedSeconds / m_shakeDurationSeconds;
    const float damping = 1.0f - normalizedTime;
    const float amplitude = m_shakeIntensity * damping;
    m_shakeOffset = D3DXVECTOR3(std::sin(elapsedSeconds * 55.0f) * amplitude,
                                std::cos(elapsedSeconds * 83.0f) * amplitude * 0.55f,
                                std::sin(elapsedSeconds * 34.0f) * amplitude * 0.20f);
    m_shakeFrameActive = true;
}

void NSRender::Camera::EndShakeFrame()
{
    m_shakeFrameActive = false;
    m_shakeOffset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
}

