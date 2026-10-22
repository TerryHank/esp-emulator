// SPDX-License-Identifier: GPL-3.0-or-later
// CarCYD ring/mini-gauge adaptation; source attribution in skin_licenses/README.md.
#pragma once

#include <lvgl.h>
#include <array>
#include <functional>
#include <optional>
#include "vesc_protocol.h"

class VescSkinView {
public:
    enum Skin { kCarCluster, kCarSix, kDigital, kAnalog, kMinimal, kRace, kSkinCount };
    enum Signal {
        kVoltage,
        kBatteryCurrent,
        kPower,
        kMotorTemp,
        kControllerTemp,
        kMotorCurrent,
        kDuty,
        kErpm,
        kRpm,
        kSpeed,
        kSignalCount
    };
    explicit VescSkinView(lv_obj_t* parent);
    lv_obj_t* root() const { return root_; }
    void SetSkin(int skin);
    int skin() const { return skin_; }
    void Resize(int width, int height);
    void Update(const vesc::Snapshot& state);
    void SetChanged(std::function<void(int)> callback) { changed_ = std::move(callback); }
    static const char* SkinName(int skin);
    static const char* SignalName(int signal);
    static const char* SignalUnit(int signal);
    static const char* FaultText(uint8_t fault);
    static std::optional<double> ReadSignal(const vesc::Snapshot& state, int signal);

private:
    struct Metric {
        lv_obj_t* root = nullptr;
        lv_obj_t* value = nullptr;
        lv_obj_t* caption = nullptr;
        lv_obj_t* unit = nullptr;
        lv_obj_t* arc = nullptr;
        int signal = kVoltage;
    };
    void Build();
    void Layout();
    void ShowPicker(int slot);
    void AddMetric(int index, int signal, bool gauge);
    lv_obj_t* Arc(lv_obj_t* parent, int start, int sweep, int width);
    static lv_obj_t* Box(lv_obj_t* parent);
    static lv_obj_t* Label(lv_obj_t* parent, const lv_font_t* font);
    static void Text(lv_obj_t* label, const char* text);
    lv_obj_t* root_;
    lv_obj_t* hero_ = nullptr;
    lv_obj_t* arc_ = nullptr;
    lv_obj_t* scale_ = nullptr;
    lv_obj_t* speed_ = nullptr;
    lv_obj_t* unit_ = nullptr;
    lv_obj_t* rpm_ = nullptr;
    lv_obj_t* picker_ = nullptr;
    lv_obj_t* needle_ = nullptr;
    lv_obj_t* bar_ = nullptr;
    lv_point_precise_t needle_points_[2]{};
    std::array<Metric, 6> metrics_{};
    vesc::Snapshot state_;
    std::function<void(int)> changed_;
    int skin_ = kCarCluster, width_ = 472, height_ = 196, metric_count_ = 0;
    int picked_slot_ = 0;
    int dial_diameter_ = 196;
    double ring_max_ = 20000;
};
