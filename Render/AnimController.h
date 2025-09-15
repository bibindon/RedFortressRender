#pragma once

#include <d3dx9.h>
#include <string>
#include <map>

namespace NSRender
{
struct AnimSetting;

using AnimSetMap = std::map<std::wstring, AnimSetting>;

struct AnimSetting
{
    float m_startPos { 0.f };
    float m_duration { 1.f };
    bool m_loop { true };
    bool m_stopEnd = false;
};

// DirectX9は64bitでビルドするとアニメーションが80倍速になってしまうことに超注意
class AnimController
{
public:
    AnimController();
    ~AnimController();
    void Init(const LPD3DXANIMATIONCONTROLLER controller, const AnimSetMap& animSetMap);
    void SetAnim(const std::wstring& animName, const DOUBLE& pos = -1.f);
    void SetAnimSettings(const AnimSetMap& animSetMap);
    void Update();
    void Finalize();
    void SetAnimSpeed(const float speed);

private:
    LPD3DXANIMATIONCONTROLLER m_controller { nullptr };
    float m_animationTime { 0.f };
    AnimSetMap m_animSettingMap;
    std::wstring m_animName { };
    float m_animSpeed = 0.f;

};
}

