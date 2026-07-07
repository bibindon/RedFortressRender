#include "AnimController.h"
#include <algorithm>

#include "Common.h"

namespace
{

std::wstring Utf8ToWideString(const char* text)
{
    if (text == nullptr || text[0] == '\0')
    {
        return L"";
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (required <= 1)
    {
        return L"";
    }

    std::wstring result(required - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, -1, &result[0], required);
    return result;
}

void ScaleAnimSettingForDirectX64(NSRender::AnimSetting& animSetting)
{
    animSetting.m_startPos /= 80;
    animSetting.m_duration /= 80;
}

} // anonymous namespace

NSRender::AnimController::AnimController()
{
    m_animSpeed = Common::ANIMATION_SPEED;

    // 64bitの場合80倍になってしまうので1/80にする
    //m_animSpeed /= 80;
    m_animSpeed /= 160;
}

void NSRender::AnimController::Init(const LPD3DXANIMATIONCONTROLLER controller,
                                    const AnimSetMap& animSetMap)
{
    m_controller = controller;
    m_animSettingMap = animSetMap;
    m_animationSetIndexMap.clear();
    std::wstring firstAnimationSetName;

    if (m_controller != nullptr)
    {
        const UINT animationSetCount = m_controller->GetNumAnimationSets();
        for (UINT i = 0; i < animationSetCount; ++i)
        {
            LPD3DXANIMATIONSET animationSet = nullptr;
            const HRESULT hr = m_controller->GetAnimationSet(i, &animationSet);
            if (FAILED(hr) || animationSet == nullptr)
            {
                continue;
            }

            std::wstring animationName = Utf8ToWideString(animationSet->GetName());
            if (animationName.empty())
            {
                animationName = std::to_wstring(i);
            }
            if (i == 0)
            {
                firstAnimationSetName = animationName;
            }

            m_animationSetIndexMap[animationName] = i;

            if (m_animSettingMap.find(animationName) == m_animSettingMap.end())
            {
                AnimSetting animSetting;
                animSetting.m_startPos = 0.f;
                animSetting.m_duration = static_cast<float>((std::max)(animationSet->GetPeriod(), 0.0001));
                animSetting.m_loop = true;
                animSetting.m_stopEnd = false;
                m_animSettingMap[animationName] = animSetting;
            }

            SAFE_RELEASE(animationSet);
        }
    }

    // DirectX9を64bitでビルドすると80倍速になってしまうため調節する
    for (auto& animSetting : m_animSettingMap)
    {
        ScaleAnimSettingForDirectX64(animSetting.second);
    }

    if (!firstAnimationSetName.empty())
    {
        m_animName = firstAnimationSetName;
        SetTrackAnimationSetByName(m_animName);
    }
    else if (!m_animSettingMap.empty())
    {
        m_animName = m_animSettingMap.begin()->first;
    }
}

void NSRender::AnimController::SetAnim(const std::wstring& animName, const DOUBLE& pos)
{
    m_animName = animName;
    SetTrackAnimationSetByName(m_animName);
    if (pos >= 0.f)
    {
        m_animationTime = 0.f;
    }
}

void NSRender::AnimController::SetAnimSettings(const AnimSetMap& animSetMap)
{
    m_animSettingMap = animSetMap;

    for (auto& animSetting : m_animSettingMap)
    {
        ScaleAnimSettingForDirectX64(animSetting.second);
    }
}

void NSRender::AnimController::Update()
{
    if (m_controller == nullptr)
    {
        return;
    }

    const auto animSetting = m_animSettingMap.find(m_animName);
    if (animSetting == m_animSettingMap.end())
    {
        return;
    }

    float workAnimTime = 0.f;
    workAnimTime = m_animationTime + m_animSpeed;

    // 通常の更新処理。アニメを進める
    if (workAnimTime < animSetting->second.m_duration)
    {
        m_animationTime += m_animSpeed;
        m_controller->SetTrackPosition(0, animSetting->second.m_startPos);
        m_controller->AdvanceTime(m_animationTime, nullptr);
    }
    else
    {
        //-------------------------------------------------------------
        // アニメーションを最後まで実行した場合。
        // 最初に戻るか、ループするか、最後の状態で止まる。
        // 例：
        // 攻撃→最初に戻る
        // 歩く→ループ
        // 死亡→最後の状態で止まる
        //-------------------------------------------------------------

        // 最初に戻る
        if (animSetting->second.m_loop == false &&
            animSetting->second.m_stopEnd == false)
        {
            m_animName = m_animSettingMap.begin()->first;
            m_animationTime = 0.f;
            SetTrackAnimationSetByName(m_animName);
            m_controller->SetTrackPosition(0, m_animSettingMap[m_animName].m_startPos);
            m_controller->AdvanceTime(m_animationTime, nullptr);
        }
        // ループ
        else if (animSetting->second.m_loop)
        {
            m_animationTime = 0.f;
            m_controller->SetTrackPosition(0, animSetting->second.m_startPos);
            m_controller->AdvanceTime(m_animationTime, nullptr);
        }
        // 最後の状態で止まる
        else if (animSetting->second.m_stopEnd)
        {
            m_controller->SetTrackPosition(0, animSetting->second.m_startPos);
            m_controller->AdvanceTime(m_animationTime, nullptr);
        }
    }
}

void NSRender::AnimController::Finalize()
{
    SAFE_RELEASE(m_controller);
}

void NSRender::AnimController::SetAnimSpeed(const float speed)
{
    m_animSpeed = speed / 80;
}

void NSRender::AnimController::SetTrackAnimationSetByName(const std::wstring& animName)
{
    const auto found = m_animationSetIndexMap.find(animName);
    if (found == m_animationSetIndexMap.end())
    {
        return;
    }

    SetTrackAnimationSetByIndex(found->second);
}

void NSRender::AnimController::SetTrackAnimationSetByIndex(UINT index)
{
    if (m_controller == nullptr)
    {
        return;
    }

    LPD3DXANIMATIONSET animationSet = nullptr;
    const HRESULT hr = m_controller->GetAnimationSet(index, &animationSet);
    if (FAILED(hr) || animationSet == nullptr)
    {
        return;
    }

    m_controller->SetTrackAnimationSet(0, animationSet);
    SAFE_RELEASE(animationSet);
}
