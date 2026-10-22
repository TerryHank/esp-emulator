// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from muki01/Car-Dashboard-ESP32-CYD ring_gauge/mini_gauge/dashboard_page.
// MiniBike layouts implement Cyberj812/MiniBike-Stats' documented skin designs.
#include "vesc_skin_view.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

extern "C" {
LV_FONT_DECLARE(ui_font_speed);
LV_FONT_DECLARE(ui_font_28);
LV_FONT_DECLARE(ui_font_12);
LV_FONT_DECLARE(vesc_zh_18);
}

namespace {
constexpr uint32_t kAccents[] = {0x1fcbf2, 0x1fcbf2, 0x00bcd4, 0xff9800, 0xffffff, 0xf44336};
constexpr const char* kNames[] = {"汽车圆环", "汽车六表", "数字卡片",
                                  "经典指针", "极简大字", "赛道仪表"};
constexpr const char* kSignals[] = {"电池电压", "电池电流", "实时功率", "电机温度", "电调温度",
                                    "电机电流", "占空比",   "电气转速", "电机转速", "行驶速度"};
constexpr const char* kUnits[] = {"伏", "安", "瓦",    "℃",     "℃",
                                  "安", "%",  "转/分", "转/分", "公里/时"};
constexpr double kGaugeMax[] = {100, 100, 10000, 150, 150, 300, 100, 20000, 1000, 120};
}  // namespace

const char* VescSkinView::SkinName(int skin) {
    return kNames[std::clamp(skin, 0, static_cast<int>(kSkinCount) - 1)];
}

const char* VescSkinView::SignalName(int signal) {
    return kSignals[std::clamp(signal, 0, static_cast<int>(kSignalCount) - 1)];
}

const char* VescSkinView::SignalUnit(int signal) {
    return kUnits[std::clamp(signal, 0, static_cast<int>(kSignalCount) - 1)];
}

const char* VescSkinView::FaultText(uint8_t code) {
    static const char* names[] = {"无故障",
                                  "电压过高",
                                  "电压过低",
                                  "驱动器故障",
                                  "过电流",
                                  "电调过热",
                                  "电机过热",
                                  "栅极驱动电压过高",
                                  "栅极驱动电压过低",
                                  "主控电压过低",
                                  "看门狗复位",
                                  "编码器通信故障",
                                  "编码器信号过弱",
                                  "编码器信号过强",
                                  "存储器损坏",
                                  "电流传感器一偏差过大",
                                  "电流传感器二偏差过大",
                                  "电流传感器三偏差过大",
                                  "电流不平衡",
                                  "制动故障",
                                  "旋变跟踪故障",
                                  "旋变信号退化",
                                  "旋变信号丢失",
                                  "应用配置损坏",
                                  "电机配置损坏",
                                  "编码器未检测到磁场",
                                  "编码器磁场过强",
                                  "相位滤波故障",
                                  "编码器故障",
                                  "低压输出故障"};
    return code < sizeof(names) / sizeof(names[0]) ? names[code] : "未知故障";
}

std::optional<double> VescSkinView::ReadSignal(const vesc::Snapshot& state, int signal) {
    if (!state.online)
        return std::nullopt;
    switch (signal) {
        case kVoltage:
            return state.values.voltage;
        case kBatteryCurrent:
            return state.values.input_a;
        case kPower:
            return state.values.voltage * state.values.input_a;
        case kMotorTemp:
            return state.values.motor_c;
        case kControllerTemp:
            return state.values.fet_c;
        case kMotorCurrent:
            return state.values.motor_a;
        case kDuty:
            return state.values.duty * 100;
        case kErpm:
            return state.values.erpm;
        case kRpm:
            if (state.config.pole_pairs > 0)
                return std::abs(static_cast<double>(state.values.erpm)) / state.config.pole_pairs;
            return std::nullopt;
        case kSpeed:
            return state.SpeedKmh();
        default:
            return std::nullopt;
    }
}

lv_obj_t* VescSkinView::Box(lv_obj_t* parent) {
    auto object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(object, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return object;
}

lv_obj_t* VescSkinView::Label(lv_obj_t* parent, const lv_font_t* font) {
    auto label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xf5f7fa), 0);
    lv_obj_add_flag(label, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return label;
}

void VescSkinView::Text(lv_obj_t* label, const char* text) {
    if (std::strcmp(lv_label_get_text(label), text))
        lv_label_set_text(label, text);
}

lv_obj_t* VescSkinView::Arc(lv_obj_t* parent, int start, int sweep, int width) {
    auto arc = lv_arc_create(parent);
    lv_obj_remove_style_all(arc);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_arc_set_rotation(arc, start);
    lv_arc_set_bg_angles(arc, 0, sweep);
    lv_arc_set_range(arc, 0, 1000);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1a212b), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(kAccents[skin_]), LV_PART_INDICATOR);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return arc;
}

VescSkinView::VescSkinView(lv_obj_t* parent) {
    root_ = Box(parent);
    lv_obj_set_width(root_, lv_pct(100));
    lv_obj_set_height(root_, 0);
    lv_obj_set_flex_grow(root_, 1);
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        root_,
        [](lv_event_t* event) {
            auto self = static_cast<VescSkinView*>(lv_event_get_user_data(event));
            auto input = lv_indev_active();
            if (!input)
                return;
            auto direction = lv_indev_get_gesture_dir(input);
            if (self->skin_ <= kCarSix && (direction == LV_DIR_LEFT || direction == LV_DIR_RIGHT)) {
                self->SetSkin(self->skin_ == kCarCluster ? kCarSix : kCarCluster);
                if (self->changed_)
                    self->changed_(self->skin_);
            }
        },
        LV_EVENT_GESTURE, this);
    lv_obj_add_event_cb(
        root_,
        [](lv_event_t* event) {
            static_cast<VescSkinView*>(lv_event_get_user_data(event))->Layout();
        },
        LV_EVENT_SIZE_CHANGED, this);
    Build();
}

void VescSkinView::SetSkin(int skin) {
    if (skin < 0 || skin >= kSkinCount || skin == skin_)
        return;
    skin_ = skin;
    Build();
}

void VescSkinView::AddMetric(int index, int signal, bool gauge) {
    auto& item = metrics_[index];
    item.signal = signal;
    item.root = Box(root_);
    lv_obj_add_flag(item.root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(item.root, this);
    lv_obj_add_event_cb(
        item.root,
        [](lv_event_t* event) {
            auto self = static_cast<VescSkinView*>(
                lv_obj_get_user_data(static_cast<lv_obj_t*>(lv_event_get_current_target(event))));
            self->ShowPicker(
                static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
        },
        LV_EVENT_LONG_PRESSED, reinterpret_cast<void*>(static_cast<intptr_t>(index)));
    if (gauge)
        item.arc = Arc(item.root, 150, 240, 6);
    if (skin_ == kDigital || skin_ == kRace) {
        lv_obj_set_style_bg_color(item.root, lv_color_hex(0x131922), 0);
        lv_obj_set_style_bg_opa(item.root, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(item.root, 8, 0);
    }
    item.value = Label(item.root, &ui_font_28);
    item.unit = Label(item.root, &vesc_zh_18);
    item.caption = Label(item.root, &vesc_zh_18);
    lv_obj_set_width(item.caption, lv_pct(100));
    lv_label_set_long_mode(item.caption, LV_LABEL_LONG_DOT);
    Text(item.value, "--");
    Text(item.caption, kSignals[signal]);
    Text(item.unit, kUnits[signal]);
}

void VescSkinView::Build() {
    ring_max_ = 20000;
    lv_obj_clean(root_);
    metrics_ = {};
    picker_ = nullptr;
    arc_ = scale_ = needle_ = bar_ = nullptr;
    hero_ = Box(root_);
    if (skin_ == kCarCluster || skin_ == kAnalog) {
        arc_ = Arc(hero_, 135, 270, 12);
        scale_ = lv_scale_create(hero_);
        lv_obj_remove_style_all(scale_);
        lv_obj_remove_flag(scale_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(scale_, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_scale_set_label_show(scale_, true);
        lv_scale_set_mode(scale_, LV_SCALE_MODE_ROUND_INNER);
        lv_scale_set_angle_range(scale_, 270);
        lv_scale_set_rotation(scale_, 135);
        lv_scale_set_range(scale_, 0, skin_ == kCarCluster ? 20 : 120);
        lv_scale_set_total_tick_count(scale_, 21);
        lv_scale_set_major_tick_every(scale_, 5);
        lv_obj_set_style_length(scale_, 7, LV_PART_INDICATOR);
        lv_obj_set_style_line_color(scale_, lv_color_hex(0xc0ccd9), LV_PART_INDICATOR);
        lv_obj_set_style_line_width(scale_, 2, LV_PART_INDICATOR);
        lv_obj_set_style_text_font(scale_, &ui_font_12, LV_PART_INDICATOR);
        lv_obj_set_style_text_color(scale_, lv_color_hex(0xe9eef4), LV_PART_INDICATOR);
        lv_obj_set_style_length(scale_, 3, LV_PART_ITEMS);
        lv_obj_set_style_line_color(scale_, lv_color_hex(0x8b97a7), LV_PART_ITEMS);
        lv_obj_set_style_line_width(scale_, 1, LV_PART_ITEMS);
        if (skin_ == kAnalog) {
            needle_ = lv_line_create(hero_);
            lv_obj_set_style_line_color(needle_, lv_color_hex(0xffb020), 0);
            lv_obj_set_style_line_width(needle_, 2, 0);
            lv_obj_set_style_line_rounded(needle_, true, 0);
        }
    }
    speed_ = Label(hero_, &ui_font_speed);
    unit_ = Label(hero_, &vesc_zh_18);
    rpm_ = Label(hero_, &vesc_zh_18);
    if (skin_ == kMinimal)
        lv_obj_add_flag(rpm_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_align(rpm_, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(rpm_, LV_LABEL_LONG_DOT);
    if (skin_ == kDigital || skin_ == kRace) {
        bar_ = lv_bar_create(hero_);
        lv_obj_set_height(bar_, 8);
        lv_bar_set_range(bar_, 0, 100);
        lv_obj_set_style_bg_color(bar_, lv_color_hex(kAccents[skin_]), LV_PART_INDICATOR);
    }
    const int car[] = {kMotorTemp, kVoltage, kPower, kControllerTemp};
    const int six[] = {kVoltage, kBatteryCurrent, kPower, kMotorTemp, kControllerTemp, kDuty};
    const int digital[] = {kVoltage, kBatteryCurrent, kPower, kControllerTemp};
    const int race[] = {kMotorTemp, kControllerTemp, kDuty, kVoltage};
    metric_count_ = skin_ == kCarSix ? 6 : skin_ == kMinimal ? 2 : 4;
    for (int i = 0; i < metric_count_; ++i) {
        int signal = skin_ == kCarSix    ? six[i]
                     : skin_ == kDigital ? digital[i]
                     : skin_ == kRace    ? race[i]
                     : skin_ == kMinimal ? (i ? kPower : kVoltage)
                                         : car[i];
        AddMetric(i, signal, skin_ == kCarSix);
    }
    if (skin_ == kCarSix)
        lv_obj_add_flag(hero_, LV_OBJ_FLAG_HIDDEN);
    Layout();
    Update(state_);
}

void VescSkinView::Resize(int width, int height) {
    width_ = width;
    height_ = height;
    Layout();
}

void VescSkinView::Layout() {
    int w = lv_obj_get_width(root_), h = lv_obj_get_height(root_);
    if (w <= 0 || h <= 0) {
        w = width_;
        h = height_;
    }
    bool portrait = h > w;
    bool round = skin_ == kCarCluster || skin_ == kAnalog;
    int d = round ? std::min(w - 16, portrait ? h - 124 : std::min(h - 2, w - 176))
                  : std::min(w - 16, h - 4);
    d = std::max(d, 100);
    if (round) {
        dial_diameter_ = d;
        lv_obj_set_size(hero_, d, d);
        lv_obj_align(hero_, portrait ? LV_ALIGN_TOP_MID : LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(arc_, d, d);
        lv_obj_set_size(scale_, d - 32, d - 32);
        lv_obj_center(scale_);
    } else {
        lv_obj_set_size(hero_, portrait ? w : (skin_ == kMinimal ? w : w / 2),
                        portrait ? h / 2 : h - 4);
        lv_obj_align(hero_, portrait ? LV_ALIGN_TOP_MID : LV_ALIGN_LEFT_MID, 0, 0);
    }
    lv_obj_align(speed_, LV_ALIGN_CENTER, 0, -12);
    lv_obj_align(unit_, LV_ALIGN_CENTER, 0, 28);
    lv_obj_set_width(rpm_, lv_pct(100));
    lv_obj_align(rpm_, LV_ALIGN_BOTTOM_MID, 0, -2);
    if (bar_) {
        lv_obj_set_width(bar_, lv_pct(82));
        lv_obj_align(bar_, LV_ALIGN_BOTTOM_MID, 0, -30);
    }
    for (int i = 0; i < metric_count_; ++i) {
        auto& item = metrics_[i];
        int x, y, mw, mh;
        if (skin_ == kCarSix) {
            int columns = portrait ? 2 : 3, rows = portrait ? 3 : 2;
            mw = w / columns;
            mh = h / rows;
            x = i % columns * mw;
            y = i / columns * mh;
        } else if (round && !portrait) {
            mw = std::max(90, (w - d) / 2 + 4);
            mh = 60;
            x = i % 2 ? w - mw : 0;
            y = i < 2 ? 0 : h - mh;
        } else if (portrait || skin_ == kMinimal) {
            mw = w / 2 - 4;
            mh = std::max(52, (h - (round ? d : h / 2)) / 2 - 4);
            if (skin_ == kMinimal)
                mh = 56;
            x = (i % 2) * (w / 2);
            y = h - (metric_count_ == 2 ? mh : 2 * (mh + 4)) + (i / 2) * (mh + 4);
        } else {
            mw = w / 4 - 4;
            mh = h / 2 - 6;
            x = w / 2 + (i % 2) * (w / 4);
            y = (i / 2) * (h / 2) + 2;
        }
        lv_obj_set_pos(item.root, x, y);
        lv_obj_set_size(item.root, mw, mh);
        if (item.arc) {
            int size = std::min(mw - 4, mh);
            lv_obj_set_size(item.arc, size, size);
            lv_obj_align(item.arc, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_align(item.value, LV_ALIGN_CENTER, 0, -12);
            lv_obj_align(item.unit, LV_ALIGN_CENTER, 0, 12);
            lv_obj_align(item.caption, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_obj_set_style_text_align(item.caption, LV_TEXT_ALIGN_CENTER, 0);
        } else {
            lv_obj_align(item.caption, LV_ALIGN_TOP_LEFT, 4, 2);
            lv_obj_align(item.value, LV_ALIGN_BOTTOM_LEFT, 4, -3);
            lv_obj_align(item.unit, LV_ALIGN_BOTTOM_RIGHT, -4, -6);
        }
    }
}

void VescSkinView::Update(const vesc::Snapshot& state) {
    state_ = state;
    char text[100];
    auto speed = state.SpeedKmh();
    auto hero_value = skin_ == kRace ? ReadSignal(state, kPower) : speed;
    if (hero_value)
        std::snprintf(text, sizeof(text), skin_ == kRace ? "%.0f" : "%.1f", *hero_value);
    else
        std::strcpy(text, "--");
    Text(speed_, text);
    Text(unit_, skin_ == kRace ? "功率 · 瓦" : "公里/时");
    if (skin_ == kCarCluster) {
        if (state.online)
            std::snprintf(text, sizeof(text), "电气转速 %.1f千/分", state.values.erpm / 1000.0);
        else
            std::strcpy(text, "电气转速 --");
        double maximum = std::max(
            20000.0, std::ceil(std::abs(static_cast<double>(state.values.erpm)) / 10000) * 10000);
        if (maximum != ring_max_) {
            ring_max_ = maximum;
            lv_scale_set_range(scale_, 0, static_cast<int>(maximum / 1000));
        }
        lv_arc_set_value(
            arc_,
            state.online
                ? std::clamp(static_cast<int>(std::abs(static_cast<double>(state.values.erpm)) /
                                              ring_max_ * 1000),
                             0, 1000)
                : 0);
    } else if (skin_ == kRace) {
        if (speed)
            std::snprintf(text, sizeof(text), "车速 %.1f 公里/时", *speed);
        else
            std::strcpy(text, "车速 --");
    } else {
        std::snprintf(
            text, sizeof(text), "%s",
            state.online ? (state.config.HasGeometry() ? "" : "车速参数待设置") : "等待电调数据");
    }
    Text(rpm_, text);
    if (skin_ == kAnalog) {
        double value = speed.value_or(0);
        lv_arc_set_value(arc_, std::clamp(static_cast<int>(value / 120 * 1000), 0, 1000));
        int d = dial_diameter_;
        double angle = (135 + std::clamp(value / 120, 0.0, 1.0) * 270) * 3.141592653589793 / 180;
        needle_points_[0] = {static_cast<lv_value_precise_t>(d / 2),
                             static_cast<lv_value_precise_t>(d / 2)};
        needle_points_[1] = {
            static_cast<lv_value_precise_t>(d / 2 + std::cos(angle) * (d / 2 - 18)),
            static_cast<lv_value_precise_t>(d / 2 + std::sin(angle) * (d / 2 - 18))};
        lv_line_set_points(needle_, needle_points_, 2);
        if (speed)
            lv_obj_remove_flag(needle_, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(needle_, LV_OBJ_FLAG_HIDDEN);
    }
    if (bar_)
        lv_bar_set_value(
            bar_,
            state.online ? std::clamp(static_cast<int>(std::abs(state.values.duty) * 100), 0, 100)
                         : 0,
            LV_ANIM_OFF);
    for (int i = 0; i < metric_count_; ++i) {
        auto& item = metrics_[i];
        auto value = ReadSignal(state, item.signal);
        if (value)
            std::snprintf(text, sizeof(text),
                          item.signal == kPower || item.signal == kErpm || item.signal == kRpm
                              ? "%.0f"
                              : "%.1f",
                          *value);
        else
            std::strcpy(text, "--");
        Text(item.value, text);
        if (item.arc) {
            double maximum = kGaugeMax[item.signal];
            double normalized = value ? std::abs(*value) / maximum : 0;
            lv_arc_set_value(item.arc, std::clamp(static_cast<int>(normalized * 1000), 0, 1000));
        }
    }
}

void VescSkinView::ShowPicker(int slot) {
    if (picker_)
        return;
    picked_slot_ = slot;
    picker_ = Box(root_);
    lv_obj_set_size(picker_, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(picker_, lv_color_hex(0x0c1016), 0);
    lv_obj_set_style_bg_opa(picker_, LV_OPA_COVER, 0);
    lv_obj_add_flag(picker_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(picker_, LV_DIR_VER);
    lv_obj_set_flex_flow(picker_, LV_FLEX_FLOW_COLUMN);
    for (int i = 0; i <= kSignalCount; ++i) {
        auto row = Box(picker_);
        lv_obj_set_size(row, lv_pct(100), 40);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        auto label = Label(row, &vesc_zh_18);
        Text(label, i == kSignalCount ? "取消" : SignalName(i));
        lv_obj_center(label);
        lv_obj_set_user_data(row, this);
        lv_obj_add_event_cb(
            row,
            [](lv_event_t* event) {
                auto self = static_cast<VescSkinView*>(lv_obj_get_user_data(
                    static_cast<lv_obj_t*>(lv_event_get_current_target(event))));
                int signal =
                    static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
                if (signal < kSignalCount) {
                    auto& item = self->metrics_[self->picked_slot_];
                    item.signal = signal;
                    Text(item.caption, kSignals[signal]);
                    Text(item.unit, kUnits[signal]);
                    self->Update(self->state_);
                }
                lv_obj_delete(self->picker_);
                self->picker_ = nullptr;
            },
            LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    }
}
