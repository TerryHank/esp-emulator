// SPDX-License-Identifier: MIT
#include "voice_orb.h"
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <cmath>
#include <cstdlib>

VoiceOrb::VoiceOrb(lv_obj_t* parent) {
    pixels_ = static_cast<uint16_t*>(heap_caps_aligned_alloc(64, kSize * kSize * sizeof(uint16_t),
                                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!pixels_) {
        ESP_LOGE("VoiceOrb", "PSRAM canvas allocation failed");
        return;
    }
    canvas_ = lv_canvas_create(parent);
    lv_canvas_set_buffer(canvas_, pixels_, kSize, kSize, LV_COLOR_FORMAT_RGB565);
    lv_obj_remove_flag(canvas_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(canvas_, LV_OBJ_FLAG_HIDDEN);
    Draw(lv_tick_get());
    timer_ = lv_timer_create(
        [](lv_timer_t* t) {
            auto self = static_cast<VoiceOrb*>(lv_timer_get_user_data(t));
            if (self->update_)
                self->update_();
            const int64_t started = esp_timer_get_time();
            self->Draw(lv_tick_get());
            if (self->measured_frames_ < 120) {
                const int64_t duration = esp_timer_get_time() - started;
                self->render_us_ += duration;
                self->max_render_us_ = std::max(self->max_render_us_, duration);
                if (++self->measured_frames_ == 120)
                    ESP_LOGI("VoiceOrb", "render avg=%lld us max=%lld us, frame budget=32000 us",
                             (long long)(self->render_us_ / 120), (long long)self->max_render_us_);
            }
        },
        32, this);
    lv_timer_pause(timer_);
    ESP_LOGI("VoiceOrb", "128x128 RGB565 canvas, 32768 bytes PSRAM, ~31 FPS target");
}

VoiceOrb::~VoiceOrb() {
    if (timer_)
        lv_timer_delete(timer_);
    if (canvas_)
        lv_obj_delete(canvas_);
    heap_caps_free(pixels_);
}

void VoiceOrb::SetVisible(bool visible) {
    if (!canvas_)
        return;
    if (visible) {
        lv_obj_remove_flag(canvas_, LV_OBJ_FLAG_HIDDEN);
        previous_tick_ = lv_tick_get();
        lv_timer_resume(timer_);
    } else {
        lv_timer_pause(timer_);
        lv_obj_add_flag(canvas_, LV_OBJ_FLAG_HIDDEN);
    }
}

void VoiceOrb::SetState(State state, uint8_t level) {
    state_ = state;
    level_ = level;
}

static inline uint8_t clip8(int v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); }

static inline uint16_t rgb565(int r, int g, int b) {
    return (uint16_t)(((clip8(r) & 0xF8) << 8) | ((clip8(g) & 0xFC) << 3) |
                      ((clip8(b) & 0xF8) >> 3));
}

void VoiceOrb::Draw(uint32_t ticks) {
    const uint32_t elapsed = ticks - previous_tick_;
    previous_tick_ = ticks;
    dynamics_.Update(state_, level_, elapsed / 1000.0f);
    const auto& motion = dynamics_;
    const int bg_r = 6, bg_g = 8, bg_b = 12;
    const uint16_t bg = rgb565(bg_r, bg_g, bg_b);
    const float phase = motion.phase;
    const float radius =
        std::clamp(47.0f + 2.0f * sinf(motion.breath) +
                       motion.level * (2.0f + 6.0f * motion.listening + 10.0f * motion.speaking),
                   38.0f, 60.0f);
    const int radius_q8 = static_cast<int>(radius * 256);
    int c1x = (int)(-18 + 15 * sinf(phase));
    int c1y = (int)(-14 + 12 * cosf(phase));
    int c2x = (int)(17 + 15 * cosf(phase + 2.1f));
    int c2y = (int)(-8 + 12 * sinf(phase + 2.1f));
    int c3x = (int)(5 + 21 * sinf(phase + 4.2f));
    int c3y = (int)(20 + 8 * cosf(phase + 4.2f));

    // Precompute sine once per column/row, not once per pixel.
    int8_t stripe[kSize];
    int8_t bulge_x[kSize];
    int8_t bulge_y[kSize];
    for (int x = 0; x < kSize; ++x) {
        stripe[x] = (int8_t)(9.0f * sinf((x - kSize / 2) * 0.085f + phase));
        bulge_x[x] = (int8_t)(3.0f * sinf(x * 0.10f + phase));
    }
    for (int y = 0; y < kSize; ++y) {
        bulge_y[y] = (int8_t)(3.0f * cosf(y * 0.12f - phase));
    }

    const int blend =
        static_cast<int>(255 * motion.level * (motion.speaking + .5f * motion.listening) +
                         30 * std::max(0.0f, 1 - motion.speaking - motion.listening));
    for (int y = 0; y < kSize; ++y) {
        int dy = y - kSize / 2;
        for (int x = 0; x < kSize; ++x) {
            int dx = x - kSize / 2;
            int dist2 = dx * dx + dy * dy;
            const int rr = radius_q8 + (bulge_x[x] + bulge_y[y]) * blend;
            const int outer = rr + 8 * 256;
            const int dist_q8 = dist2 * 256;
            const int rr_squared = rr * rr / 256;
            const int outer_squared = outer * outer / 256;
            if (dist_q8 > outer_squared) {
                pixels_[y * kSize + x] = bg;
                continue;
            }
            // Fractional radius keeps the edge moving smoothly between whole pixels.
            int edge = clip8(128 + (rr_squared - dist_q8) / (11 * 256));
            int halo = clip8((outer_squared - dist_q8) * 22 / (outer_squared - rr_squared + 256));
            int d1 = (dx - c1x) * (dx - c1x) + (dy - c1y) * (dy - c1y);
            int d2 = (dx - c2x) * (dx - c2x) + (dy - c2y) * (dy - c2y);
            int d3 = (dx - c3x) * (dx - c3x) + (dy - c3y) * (dy - c3y);
            int cyan = clip8(245 - d1 / 9);
            int pink = clip8(235 - d2 / 10);
            int blue = clip8(238 - d3 / 12);
            int band = clip8(170 - abs(dy - stripe[x] - dx / 7) * 14);
            int center = clip8(200 - dist2 / 13);

            int r = 13 + pink * 135 / 255 + blue * 65 / 255 + band * 60 / 255 + center * 22 / 255;
            int g = 14 + cyan * 175 / 255 + blue * 60 / 255 + band * 45 / 255 + center * 25 / 255;
            int b = 48 + cyan * 95 / 255 + pink * 110 / 255 + blue * 105 / 255 + band * 70 / 255;
            g += static_cast<int>(11 * motion.listening);
            r += static_cast<int>(blend * motion.speaking / 7);
            b += static_cast<int>(17 * motion.thinking);

            // Edge alpha composited over navy background; no separate alpha plane needed.
            int alpha = edge < 20 ? halo : edge;
            r = bg_r + (r - bg_r) * alpha / 255;
            g = bg_g + (g - bg_g) * alpha / 255;
            b = bg_b + (b - bg_b) * alpha / 255;
            pixels_[y * kSize + x] = rgb565(r, g, b);
        }
    }
    lv_draw_buf_flush_cache(lv_canvas_get_draw_buf(canvas_), nullptr);
    lv_obj_invalidate(canvas_);
}
