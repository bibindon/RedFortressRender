#pragma once
#include <d3dx9.h>

namespace NSRender
{

enum class eCameraMode
{
    BATTLE,
    TITLE,
    TITLE_TO_BATTLE,
    BATTLE_TO_TITLE,
    SLEEP,
};

class Camera
{
public:

    static D3DXMATRIX GetViewMatrix();
    static D3DXMATRIX GetProjMatrix();

    static void SetLookAtPos(const D3DXVECTOR3& lookAtPos);
    static D3DXVECTOR3 GetLookAtPos();

    static D3DXVECTOR3 GetEyePos();
    static void SetEyePos(const D3DXVECTOR3& eye);

    static float GetRadian();
    static void SetRadian(const float radian);

    static void Update();

    static POINT GetScreenPos(const D3DXVECTOR3& world);

    static void SetCameraMode(const eCameraMode arg);

    static int MOVE_COUNT_MAX;
    static void SetHouseMode(const bool arg);
private:

    const static D3DXVECTOR3 UPWARD;
    static D3DXVECTOR3 m_eyePos;
    static D3DXVECTOR3 m_lookAtPos;
    static float m_viewAngle;
    static float m_radian;
    static float m_y;
    static bool m_sleepMode;

    static eCameraMode m_eCameraMode;
    static int m_counter;

    static bool m_houseMode;
};

}

