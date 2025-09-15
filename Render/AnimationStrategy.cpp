/*
#include "AnimationStrategy.h"
#include "Util.h"

void NSRender::no_animation::update()
{
    // do nothing
};

 NSRender::normal_animation::normal_animation(LPD3DXANIMATIONCONTROLLER controller)
        : default_animation_ { },
        animation_time_ { },
        playing_animation_ { }
{
    SAFE_RELEASE(animation_controller_);
    animation_controller_ = controller;
    DWORD animation_count { animation_controller_->GetNumAnimationSets() };

    std::vector<LPD3DXANIMATIONSET> animation_sets(animation_count);

    animation_sets_.swap(animation_sets);

    for (DWORD i { 0 }; i < animation_count; ++i)
    {
        LPD3DXANIMATIONSET temp_animation_set { nullptr };
        animation_controller_->GetAnimationSet(i, &temp_animation_set);
        SAFE_RELEASE(animation_sets_.at(i));
        animation_sets_.at(i) = temp_animation_set;
    }
}

void NSRender::normal_animation::set_animation(const std::wstring& animation_set)
{
    std::vector<LPD3DXANIMATIONSET>::const_iterator kit;

    kit = std::find_if(animation_sets_.cbegin(),
                       animation_sets_.cend(),
                       [&](const LPD3DXANIMATIONSET& a)
                       {
                           return Util::WstringToUtf8(animation_set) == a->GetName();
                       });

    if (animation_sets_.cend() == kit)
    {
        // TODO return error
    //    THROW_WITH_TRACE(_T("An illegal animation set was sent.: ") + animation_set);
        return;
    }

    animation_controller_->SetTrackAnimationSet(0, *kit);
    animation_controller_->SetTrackPosition(0, -1.001f / 60);
    animation_time_ = 0.f;

    if (animation_configs_.find(animation_set) == animation_configs_.end())
    {
        return;
    }
    if (animation_set != default_animation_ &&
        !animation_configs_.at(animation_set).loop_)
    {
        is_playing_ = true;
        playing_animation_ = animation_set;
    }
};

void NSRender::normal_animation::update()
{
    animation_time_ += Common::ANIMATION_SPEED;
    animation_controller_->SetTrackPosition(0, 0.f);
    animation_controller_->AdvanceTime(animation_time_, nullptr);
    if (is_playing_)
    {
        float duration { animation_configs_.at(playing_animation_).duration_ };
        if (animation_time_ + 0.001f >= duration)
        {
            set_animation(default_animation_);
            is_playing_ = false;
            animation_time_ = 0;
        }
    }
};

void NSRender::normal_animation::set_default_animation(const std::wstring& animation_name)
{
    default_animation_ = animation_name;
    set_animation(default_animation_);
}

void NSRender::normal_animation::set_animation_config(const std::wstring& animation_name,
                                                      const bool& loop,
                                                      const float& duration)
{
    animation_configs_.emplace(animation_name, animation_config { loop, duration });
}

bool NSRender::normal_animation::is_playing()
{
    return is_playing_;
}

void NSRender::normal_animation::SetTrackPos(const DOUBLE& pos)
{
    animation_time_ = pos;
}
*/
