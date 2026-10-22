#pragma once

#include <algorithm>
#include <cstring>
#include "application.h"
#include "display/lcd_display.h"
#include "lvgl_theme.h"
#include "vesc_dashboard.h"

class WaveshareDisplay : public SpiLcdDisplay {
public:
    using SpiLcdDisplay::SpiLcdDisplay;

    void SetupUI() override {
        if (setup_ui_called_) {
            return;
        }
        SpiLcdDisplay::SetupUI();
        DisplayLockGuard lock(this);
        lv_tick_set_cb([]() -> uint32_t { return esp_timer_get_time() / 1000; });
        dashboard_ =
            std::make_unique<VescDashboard>(this, container_, content_, top_bar_, status_label_);
        dashboard_->SetChatFont(static_cast<LvglTheme*>(current_theme_)->text_font()->font());
        lv_obj_set_parent(low_battery_popup_, lv_layer_top());
        // Legacy emoji widgets stay on the inactive original screen; the orb owns this panel.
        lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
    }

    void HideChat() {
        DisplayLockGuard lock(this);
        if (dashboard_)
            dashboard_->ShowChat(false);
    }

    void SetChatMessage(const char* role, const char* message) override {
        SpiLcdDisplay::SetChatMessage(role, message);
        DisplayLockGuard lock(this);
        if (dashboard_) {
            dashboard_->SetMessage(role, message);
            FitChatToWidth(false);
        }
    }

    void SetEmotion(const char*) override {
        if (!setup_ui_called_)
            return;
        DisplayLockGuard lock(this);
        // The canvas replaces emoji playback; don't decode hidden GIFs alongside it.
        if (gif_controller_) {
            gif_controller_->Stop();
            gif_controller_.reset();
        }
        lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
    }

    void SetTheme(Theme* theme) override {
        DisplayLockGuard lock(this);
        SpiLcdDisplay::SetTheme(theme);
        auto font = static_cast<LvglTheme*>(theme)->text_font()->font();
        lv_obj_set_style_text_font(status_label_, font, 0);
        lv_obj_set_style_text_font(notification_label_, font, 0);
        if (dashboard_) {
            dashboard_->SetChatFont(font);
            FitChatToWidth();
        }
    }

    void SetRotation(int degrees) {
        DisplayLockGuard lock(this);
        auto rotation = static_cast<lv_display_rotation_t>(degrees / 90);
        if (!setup_ui_called_ || lv_display_get_rotation(display_) == rotation) {
            return;
        }
        lv_obj_update_layout(content_);
        bool follow_latest = lv_obj_get_scroll_bottom(content_) <= 8;
        for (auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
            if (lv_indev_get_display(input) == display_) {
                lv_indev_reset(input, nullptr);
            }
        }
        // LVGL rotates the calibrated base touch coordinates together with the screen.
        lv_display_set_rotation(display_, rotation);
        width_ = lv_display_get_horizontal_resolution(display_);
        height_ = lv_display_get_vertical_resolution(display_);
        lv_obj_set_size(container_, width_, height_);
        lv_obj_set_width(top_bar_, width_);
        lv_obj_set_width(status_bar_, width_);
        lv_obj_set_width(status_label_, width_ * 8 / 10);
        lv_obj_set_width(notification_label_, width_ * 8 / 10);
        lv_obj_set_width(low_battery_popup_, width_ * 9 / 10);
        if (dashboard_) {
            dashboard_->Resize(width_, height_);
        }

        FitChatToWidth();
        lv_obj_update_layout(content_);
        if (follow_latest && lv_obj_get_child_count(content_) > 0) {
            lv_obj_scroll_to_view(lv_obj_get_child(content_, -1), LV_ANIM_OFF);
        }
        lv_obj_invalidate(lv_screen_active());
    }

    void SetPreviewImage(std::unique_ptr<LvglImage> image) override {
        SpiLcdDisplay::SetPreviewImage(std::move(image));
        DisplayLockGuard lock(this);
        FitChatToWidth(false);
    }

private:
    void FitChatToWidth(bool all = true) {
        lv_obj_update_layout(content_);
        int chat_width = lv_obj_get_content_width(content_);
        uint32_t count = lv_obj_get_child_count(content_);
        for (uint32_t i = all || count == 0 ? 0 : count - 1; i < count; ++i) {
            auto row = lv_obj_get_child(content_, i);
            auto bubble = row;
            if (!lv_obj_get_user_data(row) && lv_obj_get_child_count(row) > 0) {
                lv_obj_set_width(row, chat_width);
                bubble = lv_obj_get_child(row, 0);
            }
            auto role = static_cast<const char*>(lv_obj_get_user_data(bubble));
            if (!role || lv_obj_get_child_count(bubble) == 0) {
                continue;
            }
            auto item = lv_obj_get_child(bubble, 0);
            lv_obj_set_style_bg_color(
                bubble,
                lv_color_mix(lv_color_hex(strcmp(role, "user") == 0 ? 0x09232a : 0x131922),
                             lv_color_hex(0x06080c), LV_OPA_70),
                0);
            lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(bubble, 1, 0);
            lv_obj_set_style_border_color(bubble, lv_color_hex(0x232c39), 0);
            lv_obj_set_style_radius(bubble, 8, 0);
            if (strcmp(role, "image") == 0) {
                auto image = static_cast<const lv_image_dsc_t*>(lv_image_get_src(item));
                if (image && image->header.w && image->header.h) {
                    int preview_height = lv_obj_get_content_height(content_) * 80 / 100;
                    int scale = std::min({256, chat_width * 70 / 100 * 256 / (int)image->header.w,
                                          preview_height * 256 / (int)image->header.h});
                    lv_image_set_scale(item, scale);
                    lv_obj_set_size(bubble, image->header.w * scale / 256 + 16,
                                    image->header.h * scale / 256 + 16);
                }
            } else {
                lv_obj_set_style_text_color(item, lv_color_hex(0xf5f7fa), 0);
                lv_obj_set_width(item, LV_SIZE_CONTENT);
                lv_obj_update_layout(item);
                int width = std::max<int32_t>(20, lv_obj_get_width(item));
                lv_obj_set_width(item, std::min(width, chat_width * 85 / 100 - 16));
            }
        }
    }

    std::unique_ptr<VescDashboard> dashboard_;
};
