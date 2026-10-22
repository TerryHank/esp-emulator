// SPDX-License-Identifier: MIT
#pragma once
#include <lvgl.h>
#include <functional>
#include "orb_dynamics.h"

// Renderer adapted from the user-supplied ESP32S3_LVGL_Canvas_Orb (MIT).
class VoiceOrb {
public:
    using State = OrbDynamics::State;
    explicit VoiceOrb(lv_obj_t* parent);
    ~VoiceOrb();
    void SetVisible(bool visible);
    void SetState(State state, uint8_t level);
    void SetUpdateCallback(std::function<void()> callback) { update_ = std::move(callback); }
    lv_obj_t* Canvas() const { return canvas_; }

private:
    static constexpr int kSize = 128;
    void Draw(uint32_t ticks);
    lv_obj_t* canvas_ = nullptr;
    lv_timer_t* timer_ = nullptr;
    uint16_t* pixels_ = nullptr;
    State state_ = State::Idle;
    uint8_t level_ = 0;
    OrbDynamics dynamics_;
    uint32_t previous_tick_ = 0;
    std::function<void()> update_;
    uint32_t measured_frames_ = 0;
    int64_t render_us_ = 0;
    int64_t max_render_us_ = 0;
};
