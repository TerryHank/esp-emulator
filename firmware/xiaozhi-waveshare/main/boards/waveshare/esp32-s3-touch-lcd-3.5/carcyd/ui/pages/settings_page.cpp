#include "settings_page.h"

#include <stdio.h>

#include <cmath>
#include <cstdlib>

#include "../../config/app_config.h"
#include "../../core/i18n.h"
#include "../../core/platform.h"
#include "../../core/settings.h"
#include "../../core/units.h"
#include "../../core/vehicle_state.h"
#include "../components/dialog.h"
#include "../components/setting_rows.h"
#include "../theme.h"
#include "../ui.h"
#include "../widgets.h"
#include "carcyd_port.h"
#include "vesc_skin_view.h"

namespace ui {
namespace {

using core::settings::commit;
using core::settings::edit;
using core::settings::get;
namespace chg = core::settings;
using Section = SettingsPage::Section;

constexpr uint32_t kLiveUpdateMs = 1000;
struct CategoryDef {
    const char* glyph;
    core::Str title;
};
const CategoryDef kCategories[] = {
    {ICON_BRIGHTNESS, core::Str::SET_DISPLAY}, {ICON_RULER, core::Str::SET_UNITS},
    {ICON_GAUGE, core::Str::SET_GAUGES},       {ICON_BELL, core::Str::SET_WARNINGS},
    {ICON_VOLUME, core::Str::SET_SOUND},       {ICON_LINK, core::Str::SET_CONNECTION},
    {ICON_CHIP, core::Str::SET_SYSTEM},        {ICON_INFO, core::Str::SET_ABOUT}};
const char* const kSpeedUnits[] = {"km/h", "mph", nullptr};
const char* const kTempUnits[] = {"°C", "°F", nullptr};
SettingsPage* g_self = nullptr;
uint32_t segment(lv_event_t* e) {
    return lv_buttonmatrix_get_selected_button(lv_event_get_target_obj(e));
}
void percent_text(lv_obj_t* label, int32_t value) {
    char b[16];
    snprintf(b, sizeof(b), "%ld%%", (long)value);
    set_text(label, b);
}
void on_brightness(lv_event_t* e) {
    edit().brightness = lv_slider_get_value(lv_event_get_target_obj(e));
    commit(chg::kChangeDisplay);
}
void on_portrait_chat(lv_event_t* e) {
    edit().portrait_chat = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
    commit(chg::kChangeOrientation);
}
void on_volume(lv_event_t* e) {
    edit().volume = lv_slider_get_value(lv_event_get_target_obj(e));
    commit(chg::kChangeSound);
}
void on_accent(lv_event_t* e) {
    edit().accent =
        static_cast<core::Accent>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    commit(chg::kChangeTheme);
}
void on_speed_unit(lv_event_t* e) {
    edit().speed_unit = static_cast<core::SpeedUnit>(segment(e));
    commit(chg::kChangeUnits);
}
void on_temp_unit(lv_event_t* e) {
    edit().temp_unit = static_cast<core::TempUnit>(segment(e));
    commit(chg::kChangeUnits);
}
void fmt_rpm(int32_t v, char* out, uint32_t len) { snprintf(out, len, "%ld eRPM", (long)v); }
void on_redline(int32_t v, void*) {
    edit().redline_rpm = v;
    commit(chg::kChangeGauges);
}
lv_obj_t* config_values[4]{};
lv_obj_t* config_input = nullptr;
int config_field = 0;
const char* config_names[] = {"VESC 波特率", "轮胎外径 mm", "电机极对数", "电机/车轮传动比"};
void config_value_text(int field, int value, char* out, size_t len) {
    if (field > 0 && value == 0)
        snprintf(out, len, "未知");
    else if (field == 3)
        snprintf(out, len, "%.3f:1", value / 1000.0);
    else
        snprintf(out, len, "%d", value);
}
void config_ready(lv_event_t*) {
    char* end;
    double value = strtod(lv_textarea_get_text(config_input), &end);
    if (*end || !std::isfinite(value) || value < 0 || value > 1000000) {
        toast(core::Severity::Warning, "参数格式不正确");
        return;
    }
    double scaled = config_field == 3 ? value * 1000 : value;
    int stored = static_cast<int>(std::lround(scaled));
    auto c = carcyd_snapshot().config;
    if (config_field == 0)
        c.baud = stored;
    if (config_field == 1)
        c.wheel_mm = stored;
    if (config_field == 2)
        c.pole_pairs = stored;
    if (config_field == 3)
        c.gear_milli = stored;
    if (std::abs(scaled - stored) > 0.0001 || !vesc::ValidConfig(c)) {
        toast(core::Severity::Warning, "参数超出支持范围");
        return;
    }
    carcyd_save_config(config_field, stored);
    char text[32];
    config_value_text(config_field, stored, text, sizeof(text));
    set_text(config_values[config_field], text);
    dialog::close();
}
void on_config_edit(lv_event_t* e) {
    config_field = reinterpret_cast<intptr_t>(lv_event_get_user_data(e));
    auto c = carcyd_snapshot().config;
    int values[] = {c.baud, c.wheel_mm, c.pole_pairs, c.gear_milli};
    auto body = dialog::sheet(config_names[config_field]);
    config_input = lv_textarea_create(body);
    lv_obj_set_size(config_input, lv_pct(100), 44);
    lv_textarea_set_one_line(config_input, true);
    lv_textarea_set_max_length(config_input, 10);
    lv_textarea_set_accepted_chars(config_input, "0123456789.");
    char text[32];
    if (config_field == 3)
        snprintf(text, sizeof(text), "%.3f", values[3] / 1000.0);
    else
        snprintf(text, sizeof(text), "%d", values[config_field]);
    lv_textarea_set_text(config_input, text);
    lv_textarea_set_cursor_pos(config_input, LV_TEXTAREA_CURSOR_LAST);
    static const char* const map[] = {"1",  "2",          "3",    "\n", "4",
                                      "5",  "6",          "\n",   "7",  "8",
                                      "9",  "\n",         ".",    "0",  LV_SYMBOL_BACKSPACE,
                                      "\n", LV_SYMBOL_OK, nullptr};
    static constexpr auto key = static_cast<lv_buttonmatrix_ctrl_t>(1);
    static const lv_buttonmatrix_ctrl_t controls[] = {key, key, key, key, key, key, key,
                                                      key, key, key, key, key, key};
    auto keyboard = lv_keyboard_create(body);
    lv_obj_set_size(keyboard, lv_pct(100), 160);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, map, controls);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_textarea(keyboard, config_input);
    lv_obj_add_event_cb(keyboard, config_ready, LV_EVENT_READY, nullptr);
}
void on_skin(lv_event_t* e) {
    edit().dash_view = lv_dropdown_get_selected(lv_event_get_target_obj(e));
    commit(chg::kChangeLayout);
}
void on_wifi(lv_event_t*) { carcyd_open_wifi(); }
void on_restart(lv_event_t*) { platform::restart(); }
void on_factory_reset(lv_event_t*) { core::settings::restore_defaults(); }
const char* link_state_text(core::LinkState state) {
    switch (state) {
        case core::LinkState::Connected:
            return core::tr(core::Str::LINK_CONNECTED);
        case core::LinkState::Connecting:
            return core::tr(core::Str::LINK_CONNECTING);
        case core::LinkState::Error:
            return core::tr(core::Str::LINK_ERROR);
        case core::LinkState::Disconnected:
        default:
            return core::tr(core::Str::LINK_DISCONNECTED);
    }
}

}  // namespace

const char* SettingsPage::title() const { return core::tr(core::Str::PAGE_SETTINGS); }

void SettingsPage::create(lv_obj_t* parent) {
    g_self = this;
    root_ = box(parent);
    lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
    build_master();
}

void SettingsPage::build_master() {
    master_ = rows::list(root_);
    for (int i = 0; i < static_cast<int>(Section::Count); ++i) {
        summaries_[i] = rows::nav(master_, kCategories[i].glyph, core::tr(kCategories[i].title), "",
                                  category_cb, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        lv_obj_set_user_data(lv_obj_get_parent(summaries_[i]), this);
    }
    refresh_summaries();
}

void SettingsPage::refresh_summaries() {
    const auto& s = get();
    char buf[32];
    snprintf(buf, sizeof(buf), "常亮 · %u%%", s.brightness);
    set_text(summaries_[0], buf);
    set_text(summaries_[1], core::units::label(core::Quantity::Speed, s));
    set_text(summaries_[2], VescSkinView::SkinName(s.dash_view));
    set_text(summaries_[3], "VESC 故障状态");
    snprintf(buf, sizeof(buf), "%u%%", s.volume);
    set_text(summaries_[4], buf);
    set_text(summaries_[5], "UART / Wi-Fi");
    set_text(summaries_[6], "中文");
    set_text(summaries_[7], "小智 + CarCYD");
}

void SettingsPage::on_show() {
    refresh_summaries();
    last_live_ms_ = 0;
}

void SettingsPage::on_settings_changed(uint32_t changes) {
    refresh_summaries();
    if (brightness_value_)
        percent_text(brightness_value_, get().brightness);
    if (volume_value_)
        percent_text(volume_value_, get().volume);
}

SettingsPage::~SettingsPage() {
    if (g_self == this)
        g_self = nullptr;
}

void SettingsPage::category_cb(lv_event_t* e) {
    auto* self =
        static_cast<SettingsPage*>(lv_obj_get_user_data(lv_event_get_current_target_obj(e)));
    self->open(static_cast<Section>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))));
}

void SettingsPage::restore_state(int state) {
    if (state >= 0 && state < static_cast<int>(Section::Count)) {
        // Keep the scroll position when a section is rebuilt in place.
        int32_t scroll = 0;
        if (detail_ != nullptr && section_ == static_cast<Section>(state))
            scroll = lv_obj_get_scroll_y(detail_);
        open(static_cast<Section>(state));
        if (scroll > 0) {
            lv_obj_update_layout(detail_);
            lv_obj_scroll_to_y(detail_, scroll, LV_ANIM_OFF);
        }
    }
}

void SettingsPage::open(Section section) {
    close_section();
    section_ = section;
    set_visible(master_, false);

    detail_ = rows::list(root_);
    switch (section) {
        case Section::Display:
            build_display(detail_);
            break;
        case Section::Units:
            build_units(detail_);
            break;
        case Section::Gauges:
            build_gauges(detail_);
            break;
        case Section::Warnings:
            build_warnings(detail_);
            break;
        case Section::Sound:
            build_sound(detail_);
            break;
        case Section::Connection:
            build_connection(detail_);
            break;
        case Section::System:
            build_system(detail_);
            break;
        case Section::About:
            build_about(detail_);
            break;
        default:
            break;
    }
    set_title(core::tr(kCategories[static_cast<int>(section)].title));
    show_back_button(true);
    last_live_ms_ = 0;
}

void SettingsPage::close_section() {
    if (detail_ != nullptr) {
        lv_obj_delete(detail_);
        detail_ = nullptr;
    }
    ambient_label_ = night_row_ = day_title_ = brightness_value_ = night_value_ = nullptr;
    volume_value_ = link_status_ = memory_label_ = uptime_label_ = nullptr;
    for (auto& swatch : accent_swatches_)
        swatch = nullptr;
    section_ = Section::None;
}

bool SettingsPage::on_back() {
    if (detail_ == nullptr)
        return false;
    close_section();
    set_visible(master_, true);
    refresh_summaries();
    set_title(title());
    show_back_button(false);
    return true;
}

void SettingsPage::update(uint32_t now_ms) {
    if (detail_ == nullptr)
        return;
    if (last_live_ms_ != 0 && now_ms - last_live_ms_ < kLiveUpdateMs)
        return;
    last_live_ms_ = now_ms;
    update_live_values(now_ms);
}

void SettingsPage::update_live_values(uint32_t now_ms) {
    char buf[40];
    if (ambient_label_ != nullptr) {
        snprintf(buf, sizeof(buf), "%u%% · %u", platform::ambient_level(),
                 platform::light_sensor_raw());
        set_text(ambient_label_, buf);
    }
    if (link_status_ != nullptr) {
        const core::LinkState state = core::vehicle().link_state;
        set_text(link_status_, link_state_text(state));
        set_text_color(link_status_,
                       state == core::LinkState::Connected ? color::ok() : color::text_dim());
    }
    if (memory_label_ != nullptr) {
        platform::SystemInfo info;
        platform::system_info(info);
        snprintf(buf, sizeof(buf), "%lu KB (min %lu KB)",
                 static_cast<unsigned long>(info.heap_free / 1024),
                 static_cast<unsigned long>(info.heap_min_free / 1024));
        set_text(memory_label_, buf);
    }
    if (uptime_label_ != nullptr) {
        core::units::format_duration(now_ms / 1000, buf, sizeof(buf));
        set_text(uptime_label_, buf);
    }
}

// ---- Sections --------------------------------------------------------------------------------

void SettingsPage::build_display(lv_obj_t* list) {
    const auto& s = get();
    auto day =
        rows::slider(list, ICON_SUN, "屏幕亮度", 5, 100, s.brightness, on_brightness, nullptr);
    brightness_value_ = day.value;
    percent_text(day.value, s.brightness);
    rows::info(list, ICON_ROTATE, "自动旋转", "陀螺仪");
    rows::toggle(list, ICON_ROTATE, "竖屏进入小智", "横屏恢复仪表", s.portrait_chat,
                 on_portrait_chat, nullptr);
    rows::info(list, ICON_BRIGHTNESS, "自动休眠", "已关闭");
    auto r = card(list);
    lv_obj_set_size(r, lv_pct(100), 54);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    for (uint8_t i = 0; i < 5; ++i) {
        auto sw = box(r);
        lv_obj_set_size(sw, 28, 28);
        lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(sw, color::accent_of(static_cast<core::Accent>(i)), 0);
        lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, 0);
        lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(sw, on_accent, LV_EVENT_CLICKED,
                            reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    }
}

void SettingsPage::build_units(lv_obj_t* list) {
    rows::segmented(list, ICON_SPEED, "速度单位", kSpeedUnits,
                    static_cast<uint32_t>(get().speed_unit), on_speed_unit, nullptr);
    rows::segmented(list, ICON_THERMOMETER, "温度单位", kTempUnits,
                    static_cast<uint32_t>(get().temp_unit), on_temp_unit, nullptr);
}

void SettingsPage::build_gauges(lv_obj_t* list) {
    rows::section(list, "仪表皮肤");
    auto picker = lv_dropdown_create(list);
    lv_obj_set_width(picker, lv_pct(100));
    lv_obj_set_style_text_font(picker, font::body(), 0);
    auto popup = lv_dropdown_get_list(picker);
    lv_obj_set_style_text_font(popup, font::body(), 0);
    lv_dropdown_set_options(picker, "汽车圆环\n汽车六表\n数字卡片\n经典指针\n极简大字\n赛道仪表");
    lv_dropdown_set_selected(picker, get().dash_view);
    lv_obj_add_event_cb(picker, on_skin, LV_EVENT_VALUE_CHANGED, nullptr);
    rows::info(list, ICON_GAUGE, "电气转速量程", "20000 eRPM");
    rows::stepper(list, ICON_ALERT, "转速红区", 2000, 20000, 250, get().redline_rpm, fmt_rpm,
                  on_redline, nullptr);
    rows::note(list, "圆环与六表可左右滑动切换；长按数据项选择显示内容。");
}

void SettingsPage::build_warnings(lv_obj_t* list) {
    rows::info(list, ICON_BELL, "故障来源", "VESC 控制器");
    rows::note(list, "故障页显示控制器报告的当前故障。数据超过 1.5 秒未更新即显示未连接。");
}

void SettingsPage::build_sound(lv_obj_t* list) {
    auto volume =
        rows::slider(list, ICON_VOLUME, "小智音量", 0, 100, get().volume, on_volume, nullptr);
    volume_value_ = volume.value;
    percent_text(volume.value, get().volume);
}

void SettingsPage::build_connection(lv_obj_t* list) {
    auto c = carcyd_snapshot().config;
    link_status_ = rows::info(list, ICON_LINK, "连接状态", "");
    rows::info(list, ICON_CHIP, "UART1", "TX43 / RX44");
    int values[] = {c.baud, c.wheel_mm, c.pole_pairs, c.gear_milli};
    const char* glyphs[] = {ICON_LINK, ICON_RULER, ICON_GAUGE, ICON_SHIFT};
    for (int field = 0; field < 4; ++field) {
        char text[32];
        config_value_text(field, values[field], text, sizeof(text));
        config_values[field] =
            rows::nav(list, glyphs[field], config_names[field], text, on_config_edit,
                      reinterpret_cast<void*>(static_cast<intptr_t>(field)));
    }
    rows::note(list, "未知参数保留为未知；未设置完整参数时不推算车速。直驱轮毂传动比为 1:1。");
    rows::action(list, ICON_LINK, "Wi-Fi 配网", color::text(), on_wifi, nullptr);
}

void SettingsPage::build_system(lv_obj_t* list) {
    rows::info(list, ICON_LANGUAGE, "语言", "中文");
    rows::action(list, ICON_RESTART, "重启", color::text(), on_restart, nullptr);
    rows::action(list, ICON_RESTORE, "恢复仪表默认设置", color::text(), on_factory_reset, nullptr);
}

void SettingsPage::build_about(lv_obj_t* list) {
    platform::SystemInfo info;
    platform::system_info(info);
    char buf[48];

    rows::info(list, ICON_CAR_INFO, core::tr(core::Str::ABOUT_FIRMWARE), APP_NAME " " APP_VERSION);
    rows::info(list, ICON_HISTORY, core::tr(core::Str::ABOUT_BUILD), __DATE__);
    rows::info(list, ICON_CHIP, core::tr(core::Str::ABOUT_BOARD), info.board_name);
    rows::info(list, ICON_DASHBOARD, core::tr(core::Str::ABOUT_DISPLAY), info.panel_name);
    snprintf(buf, sizeof(buf), "%s · %lu MHz", info.chip_model,
             static_cast<unsigned long>(info.cpu_mhz));
    rows::info(list, ICON_CHIP, core::tr(core::Str::ABOUT_CHIP), buf);
    snprintf(buf, sizeof(buf), "%lu MB",
             static_cast<unsigned long>(info.flash_bytes / (1024 * 1024)));
    rows::info(list, ICON_DATABASE, core::tr(core::Str::ABOUT_FLASH), buf);
    memory_label_ = rows::info(list, ICON_MEMORY, core::tr(core::Str::ABOUT_MEMORY), "");
    uptime_label_ = rows::info(list, ICON_TIMER, core::tr(core::Str::ABOUT_UPTIME), "");
    snprintf(buf, sizeof(buf), "LVGL %d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
             LVGL_VERSION_PATCH);
    rows::info(list, ICON_PALETTE, core::tr(core::Str::ABOUT_GRAPHICS), buf);
}

}  // namespace ui
