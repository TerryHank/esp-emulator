// SPDX-License-Identifier: GPL-3.0-or-later
#include "vesc_dashboard.h"
#include <esp_chip_info.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <nvs.h>
#include <cmath>
#include <ctime>
#include "application.h"
#include "audio/audio_codec.h"
#include "backlight.h"
#include "carcyd/core/alerts.h"
#include "carcyd/core/data_link.h"
#include "carcyd/core/log.h"
#include "carcyd/core/platform.h"
#include "carcyd/core/settings.h"
#include "carcyd/ui/theme.h"
#include "carcyd/ui/ui.h"
#include "carcyd/ui/widgets.h"
#include "carcyd_port.h"
#include "vesc_service.h"
#include "voice_audio_meter.h"
#include "wifi_board.h"

namespace {
VescDashboard* dashboard = nullptr;
VescTrip trip;

class CarStorage final : public core::Storage {
public:
    bool read(const char* key, void* data, size_t length) override {
        nvs_handle_t handle;
        if (nvs_open("carcyd_vesc", NVS_READONLY, &handle) != ESP_OK)
            return false;
        size_t actual = length;
        bool ok = nvs_get_blob(handle, key, data, &actual) == ESP_OK && actual == length;
        nvs_close(handle);
        return ok;
    }
    size_t size(const char* key) override {
        nvs_handle_t handle;
        if (nvs_open("carcyd_vesc", NVS_READONLY, &handle) != ESP_OK)
            return 0;
        size_t length = 0;
        nvs_get_blob(handle, key, nullptr, &length);
        nvs_close(handle);
        return length;
    }
    bool write(const char* key, const void* data, size_t length) override {
        nvs_handle_t handle;
        if (nvs_open("carcyd_vesc", NVS_READWRITE, &handle) != ESP_OK)
            return false;
        bool ok = nvs_set_blob(handle, key, data, length) == ESP_OK && nvs_commit(handle) == ESP_OK;
        nvs_close(handle);
        return ok;
    }
    void erase_all() override {
        nvs_handle_t handle;
        if (nvs_open("carcyd_vesc", NVS_READWRITE, &handle) == ESP_OK) {
            nvs_erase_all(handle);
            nvs_commit(handle);
            nvs_close(handle);
        }
    }
} storage;

class VescLink final : public core::DataLink {
public:
    const char* name() const override { return "VESC UART"; }
    void begin(core::VehicleState& state, uint32_t now) override { poll(state, now); }
    void poll(core::VehicleState& state, uint32_t now) override {
        auto data = VescService::Instance().GetSnapshot();
        trip.Update(data);
        state.simulated = false;
        state.link_state = data.online ? core::LinkState::Connected : core::LinkState::Disconnected;
        state.invalidate_all();
        if (data.online) {
            using S = core::SignalId;
            if (auto speed = data.SpeedKmh())
                state.set(S::Speed, *speed, now);
            state.set(S::Rpm, std::abs(static_cast<double>(data.values.erpm)), now);
            state.set(S::CoolantTemp, data.values.motor_c, now);
            state.set(S::OilTemp, data.values.fet_c, now);
            state.set(S::BatteryVoltage, data.values.voltage, now);
            state.set(S::FuelLevel, data.values.voltage * data.values.input_a, now);
            state.set(S::EngineLoad, data.values.duty * 100, now);
            state.set(S::Maf, data.values.motor_a, now);
            state.set(S::FuelRate, data.values.input_a, now);
        }
        auto& dtc = state.dtc;
        const uint8_t count = data.online && data.values.fault ? 1 : 0;
        if (dtc.mil_known != data.online || dtc.count != count ||
            (count && dtc.codes[0].code != data.values.fault)) {
            dtc.mil_known = data.online;
            dtc.mil_on = count != 0;
            dtc.count = count;
            if (count)
                dtc.codes[0] = {data.values.fault, core::DtcStatus::Stored};
            dtc.scanned = data.online;
            dtc.last_scan_ms = now;
            ++dtc.revision;
        }
        time_t epoch = time(nullptr);
        state.clock_valid = epoch > 1700000000;
        if (state.clock_valid) {
            tm local{};
            localtime_r(&epoch, &local);
            state.clock_minutes = local.tm_hour * 60 + local.tm_min;
        }
    }
    bool request_dtc_read(core::VehicleState& state, uint32_t now) override {
        poll(state, now);
        state.dtc.last_result = state.link_state == core::LinkState::Connected
                                    ? core::DtcResult::ReadOk
                                    : core::DtcResult::NotConnected;
        state.dtc.last_scan_ms = now;
        ++state.dtc.revision;
        return true;
    }
    bool request_dtc_clear(core::VehicleState&, uint32_t) override { return false; }
} vesc_link;

void settings_changed(uint32_t changes, void*) {
    auto s = core::settings::get();
    if (changes & core::settings::kChangeDisplay) {
        Application::GetInstance().Schedule(
            [s]() { Board::GetInstance().GetBacklight()->SetBrightness(s.brightness, true); });
    }
    if (changes & core::settings::kChangeSound) {
        Application::GetInstance().Schedule(
            [s]() { Board::GetInstance().GetAudioCodec()->SetOutputVolume(s.volume); });
    }
}
}  // namespace

vesc::Snapshot carcyd_snapshot() { return VescService::Instance().GetSnapshot(); }
VescTrip& carcyd_trip() { return trip; }
bool carcyd_signal_available(core::SignalId id) {
    using S = core::SignalId;
    return id == S::Speed || id == S::Rpm || id == S::CoolantTemp || id == S::OilTemp ||
           id == S::BatteryVoltage || id == S::FuelLevel || id == S::EngineLoad || id == S::Maf ||
           id == S::FuelRate;
}
void carcyd_show_chat(bool show) {
    if (dashboard)
        dashboard->ShowChat(show);
}
bool carcyd_chat_visible() { return dashboard && dashboard->IsChatPage(); }
void carcyd_local_command(const std::string& action) {
    DisplayLockGuard lock(Board::GetInstance().GetDisplay());
    ESP_LOGI("LocalVoice", "Executed: %s", action.c_str());
    if (action == "chat")
        carcyd_show_chat(true);
    else if (action == "dashboard" || action == "voltage") {
        ui::navigate(ui::PageId::Dashboard);
        if (action == "voltage") {
            auto data = carcyd_snapshot();
            char text[32];
            if (data.online)
                snprintf(text, sizeof(text), "%.1f V", data.values.voltage);
            else
                snprintf(text, sizeof(text), "未连接 VESC");
            ui::toast(core::Severity::Info, "电池电压", text);
        }
    } else if (action == "settings")
        ui::navigate(ui::PageId::Settings);
    else if (action == "stop") {
        auto& app = Application::GetInstance();
        if (app.GetDeviceState() == kDeviceStateSpeaking ||
            app.GetDeviceState() == kDeviceStateListening)
            app.ToggleChatState();
    }
}
void carcyd_open_wifi() {
    carcyd_show_chat(false);
    Application::GetInstance().Schedule(
        []() { static_cast<WifiBoard&>(Board::GetInstance()).EnterWifiConfigMode(); });
}
void carcyd_save_config(int field, int value) {
    auto config = carcyd_snapshot().config;
    if (field == 0)
        config.baud = value;
    if (field == 1)
        config.wheel_mm = value;
    if (field == 2)
        config.pole_pairs = value;
    if (field == 3)
        config.gear_milli = value;
    Application::GetInstance().Schedule([config]() { VescService::Instance().SaveConfig(config); });
}

namespace platform {
uint32_t millis() { return esp_timer_get_time() / 1000; }
void system_info(SystemInfo& out) {
    esp_chip_info_t info{};
    esp_chip_info(&info);
    out = {"ESP32-S3",
           static_cast<uint8_t>(info.revision),
           static_cast<uint8_t>(info.cores),
           240,
           16 * 1024 * 1024,
           esp_get_free_heap_size(),
           esp_get_minimum_free_heap_size(),
           heap_caps_get_total_size(MALLOC_CAP_8BIT),
           esp_get_idf_version(),
           "Waveshare S3 3.5",
           "480x320 / 320x480"};
}
[[noreturn]] void restart() {
    core::settings::flush();
    esp_restart();
}
[[noreturn]] void factory_reset() {
    storage.erase_all();
    esp_restart();
}
uint16_t light_sensor_raw() { return 0; }
uint8_t ambient_level() { return 0; }
uint8_t backlight_level() { return Board::GetInstance().GetBacklight()->brightness(); }
void play(Sound) {}  // Xiaozhi retains the full duplex audio path.
bool touch_read_raw(TouchSample&) { return false; }
void touch_apply_calibration(const core::TouchCalibration&) {}
void touch_input_enable(bool) {}
}  // namespace platform

VescDashboard::VescDashboard(Display*, lv_obj_t*, lv_obj_t* chat, lv_obj_t*, lv_obj_t* state)
    : chat_(chat), state_(state) {
    core::log::init([](const char* text) { ESP_LOGI("CarCYD", "%s", text); }, platform::millis,
                    core::log::Level::Info);
    core::settings::load(storage);
    core::settings::edit().demo_mode = false;
    core::settings::edit().auto_brightness = false;
    core::settings::edit().language = core::Language::English;  // Chinese translation slot.
    core::settings::add_listener(settings_changed, nullptr);
    core::link::set_active(&vesc_link, platform::millis());
    ui::init(lv_display_get_default());
    ui::start(false);

    chat_layer_ = ui::column(lv_layer_top(), 0);
    lv_obj_remove_flag(chat_layer_, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    chat_header_ = ui::box(chat_layer_);
    lv_obj_set_size(chat_header_, lv_pct(100), 44);
    chat_body_ = ui::box(chat_layer_);
    lv_obj_set_width(chat_body_, lv_pct(100));
    lv_obj_set_height(chat_body_, 0);
    lv_obj_set_flex_grow(chat_body_, 1);
    lv_obj_set_style_bg_color(chat_layer_, ui::color::bg(), 0);
    lv_obj_set_style_bg_opa(chat_layer_, LV_OPA_COVER, 0);
    status_ = ui::label(chat_header_, ui::font::body(), ui::color::text(), "小智");
    lv_obj_set_pos(status_, 10, 10);
    auto close = ui::icon_button(
        chat_header_, ICON_CLOSE, 32, [](lv_event_t*) { carcyd_show_chat(false); }, nullptr);
    lv_obj_align(close, LV_ALIGN_TOP_RIGHT, -4, 4);
    orb_area_ = ui::box(chat_body_);
    orb_ = std::make_unique<VoiceOrb>(orb_area_);
    if (orb_->Canvas())
        lv_obj_center(orb_->Canvas());
    lv_obj_set_parent(chat_, chat_body_);
    lv_obj_remove_flag(chat_, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_remove_flag(chat_, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_remove_flag(chat_, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_set_style_min_width(chat_, 0, 0);
    lv_obj_set_style_min_height(chat_, 0, 0);
    lv_obj_remove_flag(chat_, LV_OBJ_FLAG_HIDDEN);
    orb_->SetUpdateCallback([this]() { UpdateOrb(); });
    dashboard = this;
    Resize(lv_display_get_horizontal_resolution(nullptr),
           lv_display_get_vertical_resolution(nullptr));
    ShowChat(false);
    lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<VescDashboard*>(lv_timer_get_user_data(timer))->Refresh();
        },
        200, this);
    ESP_LOGI("CarCYD", "Native 480x320 UI; idle dimming and shutdown disabled");
}

void VescDashboard::Resize(int width, int height) {
    bool changed = ui::layout::kScreenW != width || ui::layout::kScreenH != height;
    ui::layout::kScreenW = width;
    ui::layout::kScreenH = height;
    ui::layout::kRailW = width > height ? 72 : 56;
    ui::layout::kStatusH = 36;
    ui::layout::kContentW = width - ui::layout::kRailW;
    ui::layout::kContentH = height - ui::layout::kStatusH;
    ui::layout::kNavItemH = height / 5;
    lv_obj_set_pos(chat_layer_, ui::layout::kRailW, 0);
    lv_obj_set_size(chat_layer_, ui::layout::kContentW, height);
    // The body owns layout and clipping; scrolling the history cannot move the orb.
    const bool stacked = height > width;
    lv_obj_set_flex_flow(chat_body_, stacked ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(chat_body_, 0, 0);
    lv_obj_set_style_pad_column(chat_body_, 0, 0);
    const int orb_extent = orb_->Canvas() ? 140 : 0;
    lv_obj_set_size(orb_area_, stacked ? lv_pct(100) : orb_extent,
                    stacked ? orb_extent : lv_pct(100));
    lv_obj_set_flex_grow(orb_area_, 0);
    lv_obj_set_size(chat_, stacked ? lv_pct(100) : 0, stacked ? 0 : lv_pct(100));
    lv_obj_set_flex_grow(chat_, 1);
    const bool portrait = height > width;
    if (!core::settings::get().portrait_chat)
        auto_chat_ = false;
    if (portrait != portrait_) {
        if (portrait && core::settings::get().portrait_chat &&
            Application::GetInstance().GetDeviceState() != kDeviceStateWifiConfiguring) {
            auto_chat_ = true;
            ShowChat(true);
            ESP_LOGI("CarCYD", "Portrait: opened Xiaozhi");
        } else if (!portrait && auto_chat_) {
            auto_chat_ = false;
            ShowChat(false);
            ESP_LOGI("CarCYD", "Landscape: restored dashboard");
        }
    }
    portrait_ = portrait;
    if (changed)
        ui::request_rebuild();
}
void VescDashboard::ShowChat(bool show) {
    chat_open_ = show;
    orb_->SetVisible(show);
    ui::set_content_visible(!show);
    if (show) {
        lv_obj_remove_flag(chat_layer_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(chat_layer_);
    } else
        lv_obj_add_flag(chat_layer_, LV_OBJ_FLAG_HIDDEN);
}
void VescDashboard::Refresh() {
    const uint32_t now = platform::millis();
    core::link::poll(now);
    core::alerts().update(core::vehicle(), core::settings::get(), now);
    core::settings::service(now);
    if (chat_open_) {
        ui::set_text(status_, lv_label_get_text(state_));
    }
}
void VescDashboard::UpdateOrb() {
    auto& app = Application::GetInstance();
    auto state = app.GetDeviceState();
    auto orb_state = VoiceOrb::State::Idle;
    if (state == kDeviceStateSpeaking) {
        orb_state = VoiceOrb::State::Speaking;
        awaiting_reply_ = false;
    } else if (state == kDeviceStateConnecting ||
               (state == kDeviceStateListening && awaiting_reply_ && !app.IsVoiceDetected())) {
        orb_state = VoiceOrb::State::Thinking;
    } else if (state == kDeviceStateListening) {
        orb_state = VoiceOrb::State::Listening;
    } else {
        awaiting_reply_ = false;
    }
    auto& meters = GetVoiceAudioMeters();
    const uint32_t now = platform::millis();
    const uint8_t level = orb_state == VoiceOrb::State::Speaking    ? meters.playback.Read(now)
                          : orb_state == VoiceOrb::State::Listening ? meters.microphone.Read(now)
                                                                    : 0;
    orb_->SetState(orb_state, level);
}

void VescDashboard::SetMessage(const char* role, const char* message) {
    if (!role || !message || !*message)
        return;
    if (strcmp(role, "user") == 0)
        awaiting_reply_ = true;
    if (strcmp(role, "assistant") == 0)
        awaiting_reply_ = false;
}

void VescDashboard::SetChatFont(const lv_font_t* font) {
    // Dashboard captions contain a subset of Chinese glyphs. Conversations use
    // Xiaozhi's complete font, whose lifetime is owned by the current theme.
    lv_obj_set_style_text_font(chat_, font, 0);
    lv_obj_set_style_bg_color(chat_, ui::color::bg(), 0);
    lv_obj_set_style_bg_opa(chat_, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(status_, font, 0);
}
