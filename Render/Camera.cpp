
#include "Camera.h"

#include <cmath>
#include <string>

const D3DXVECTOR3 NSRender::Camera::UPWARD (0.0f, 1.0f, 0.0f);
// m_eyePosに何をセットしても視点は変わらない。視点はm_radianによって決まる。
D3DXVECTOR3 NSRender::Camera::m_eyePos(0.f, 0.f, 0.f);
D3DXVECTOR3 NSRender::Camera::m_lookAtPos(0.0f, 0.0f, 0.0f);
float NSRender::Camera::m_viewAngle = (D3DX_PI / 4);
// m_radian == D3DX_PI * 3 / 2の時（270度の時）カメラは正面を向く
float NSRender::Camera::m_radian = D3DX_PI * 3 / 2;
float NSRender::Camera::m_y = 3.f;
bool NSRender::Camera::m_sleepMode = false;
bool NSRender::Camera::m_houseMode = false;

NSRender::eCameraMode NSRender::Camera::m_eCameraMode;

int NSRender::Camera::m_counter = 0;
int NSRender::Camera::MOVE_COUNT_MAX = 240;

D3DXMATRIX NSRender::Camera::GetViewMatrix()
{
    D3DXMATRIX viewMatrix { };
    D3DXMatrixLookAtLH(&viewMatrix, &m_eyePos, &m_lookAtPos, &UPWARD);
    return viewMatrix;
}

D3DXMATRIX NSRender::Camera::GetProjMatrix()
{
    D3DXMATRIX projection_matrix { };
    D3DXMatrixPerspectiveFovLH(&projection_matrix,
                               m_viewAngle,
                               static_cast<float>(1920) / 1080, /* TODO */
                               0.1f,
                               30'000.0f);

    return projection_matrix;
}

void NSRender::Camera::SetLookAtPos(const D3DXVECTOR3& lookAtPos)
{
    m_lookAtPos = lookAtPos;
}

D3DXVECTOR3 NSRender::Camera::GetEyePos()
{
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
    static const D3DXMATRIX viewport_matrix {
        1600 / 2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -900 / 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        1600 / 2.0f, 900 / 2.0f, 0.0f, 1.0f };
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

