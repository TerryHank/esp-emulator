#include "touch_wifi_setup.h"
#include "application.h"
#include "board.h"
#include "lvgl_theme.h"

#include <esp_log.h>
#include <cJSON.h>
#include <wifi_manager.h>
#include <algorithm>
#include <cstring>

static const char* TAG = "TouchWifiSetup";
static const char* const kNumberMap[] = {"1", "2", "3", "\n", "4",   "5",  "6", "\n",
                                         "7", "8", "9", "\n", "abc", "1#", "0", LV_SYMBOL_BACKSPACE,
                                         ""};
static constexpr auto kNumberKey = static_cast<lv_buttonmatrix_ctrl_t>(
    1 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG | LV_BUTTONMATRIX_CTRL_NO_REPEAT);
static constexpr auto kWideNumberKey = static_cast<lv_buttonmatrix_ctrl_t>(
    2 | LV_BUTTONMATRIX_CTRL_CLICK_TRIG | LV_BUTTONMATRIX_CTRL_NO_REPEAT);
static const lv_buttonmatrix_ctrl_t kNumberControls[] = {
    kNumberKey, kNumberKey, kNumberKey, kNumberKey, kNumberKey,     kNumberKey,    kNumberKey,
    kNumberKey, kNumberKey, kNumberKey, kNumberKey, kWideNumberKey, kWideNumberKey};

std::string TouchWifiSetup::Request(const char* method, const char* path, std::string body) {
    auto http = Board::GetInstance().GetNetwork()->CreateHttp();
    http->SetTimeout(35000);
    http->SetHeader("Content-Type", "application/json");
    http->SetContent(std::move(body));
    // Use the existing local portal for scanning, validation and credential storage.
    auto url = WifiManager::GetInstance().GetApWebUrl() + path;
    std::string response;
    if (http->Open(method, url) && http->GetStatusCode() == 200) {
        response = http->ReadAll();
    }
    http->Close();
    return response;
}

lv_obj_t* TouchWifiSetup::Button(lv_obj_t* parent, const char* text, lv_event_cb_t callback) {
    auto button = lv_button_create(parent);
    lv_obj_set_height(button, 38);
    lv_obj_set_style_pad_all(button, 4, 0);
    auto label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, this);
    return button;
}

void TouchWifiSetup::Show() {
    DisplayLockGuard lock(display_);
    if (root_) {
        return;
    }
    auto theme = static_cast<LvglTheme*>(display_->GetTheme());
    // The CarCYD page is rebuilt on IMU rotation; keep setup alive above that screen.
    root_ = lv_obj_create(lv_layer_top());
    lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(root_, 6, 0);
    lv_obj_set_style_pad_row(root_, 4, 0);
    lv_obj_set_style_radius(root_, 0, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_text_font(root_, theme->text_font()->font(), 0);
    lv_obj_set_style_text_color(root_, theme->text_color(), 0);
    lv_obj_set_style_bg_color(root_, theme->background_color(), 0);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    ESP_LOGI(TAG, "Showing touch WiFi setup");
    ShowNetworks();
    Scan();
}

void TouchWifiSetup::Hide() {
    DisplayLockGuard lock(display_);
    if (root_) {
        if (keyboard_)
            lv_keyboard_set_textarea(keyboard_, nullptr);
        lv_obj_delete(root_);
        root_ = nullptr;
        list_ = status_ = password_ = keyboard_ = nullptr;
        ESP_LOGI(TAG, "Touch WiFi setup closed");
    }
}

void TouchWifiSetup::Back() {
    DisplayLockGuard lock(display_);
    if (root_ && password_ && !busy_) {
        lv_keyboard_set_textarea(keyboard_, nullptr);
        ShowNetworks();
        ESP_LOGI(TAG, "BOOT returned to WiFi list");
    }
}

void TouchWifiSetup::ShowNetworks() {
    lv_obj_clean(root_);
    password_ = keyboard_ = nullptr;
    auto header = lv_obj_create(root_);
    lv_obj_set_size(header, lv_pct(100), 42);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    auto title = lv_label_create(header);
    lv_label_set_text(title, "选择 Wi-Fi (2.4 GHz)");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);
    auto refresh = Button(header, "刷新", [](lv_event_t* event) {
        static_cast<TouchWifiSetup*>(lv_event_get_user_data(event))->Scan();
    });
    lv_obj_align(refresh, LV_ALIGN_RIGHT_MID, 0, 0);

    status_ = lv_label_create(root_);
    lv_obj_set_width(status_, lv_pct(100));
    lv_label_set_text(status_, networks_.empty() ? "正在扫描附近网络…" : "");
    if (!networks_.empty())
        lv_obj_add_flag(status_, LV_OBJ_FLAG_HIDDEN);
    list_ = lv_obj_create(root_);
    lv_obj_set_width(list_, lv_pct(100));
    lv_obj_set_flex_grow(list_, 1);
    lv_obj_set_flex_flow(list_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list_, 4, 0);
    lv_obj_set_scroll_dir(list_, LV_DIR_VER);
    for (size_t i = 0; i < networks_.size(); ++i) {
        auto button = lv_button_create(list_);
        lv_obj_set_size(button, lv_pct(100), 46);
        lv_obj_set_user_data(button, this);
        auto label = lv_label_create(button);
        lv_obj_set_width(label, lv_pct(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        lv_label_set_text_fmt(label, "%s  %d dBm%s", networks_[i].ssid.c_str(), networks_[i].rssi,
                              networks_[i].encrypted ? "" : " (开放)");
        lv_obj_center(label);
        lv_obj_add_event_cb(
            button,
            [](lv_event_t* event) {
                auto self = static_cast<TouchWifiSetup*>(
                    lv_obj_get_user_data(lv_event_get_target_obj(event)));
                if (!self->busy_) {
                    self->ShowPassword(reinterpret_cast<size_t>(lv_event_get_user_data(event)));
                }
            },
            LV_EVENT_CLICKED, reinterpret_cast<void*>(i));
    }
}

void TouchWifiSetup::Scan() {
    if (busy_) {
        return;
    }
    busy_ = true;
    lv_obj_remove_flag(status_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(status_, "正在扫描附近网络…");
    if (xTaskCreate(
            [](void* context) {
                auto self = static_cast<TouchWifiSetup*>(context);
                std::vector<Network> networks;
                for (int attempt = 0; attempt < 4; ++attempt) {
                    auto response = Request("GET", "/scan");
                    auto json = cJSON_Parse(response.c_str());
                    auto aps = cJSON_GetObjectItem(json, "aps");
                    cJSON* ap;
                    cJSON_ArrayForEach (ap, aps) {
                        auto ssid = cJSON_GetObjectItem(ap, "ssid");
                        auto rssi = cJSON_GetObjectItem(ap, "rssi");
                        auto auth = cJSON_GetObjectItem(ap, "authmode");
                        if (cJSON_IsString(ssid) && ssid->valuestring[0] && cJSON_IsNumber(rssi) &&
                            cJSON_IsNumber(auth) &&
                            std::none_of(networks.begin(), networks.end(),
                                         [&](const Network& network) {
                                             return network.ssid == ssid->valuestring;
                                         })) {
                            networks.push_back(
                                {ssid->valuestring, rssi->valueint, auth->valueint != 0});
                        }
                    }
                    cJSON_Delete(json);
                    if (!networks.empty()) {
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(800));
                }
                Application::GetInstance().Schedule(
                    [self, networks = std::move(networks)]() mutable {
                        DisplayLockGuard lock(self->display_);
                        self->busy_ = false;
                        if (!self->root_) {
                            return;
                        }
                        self->networks_ = std::move(networks);
                        self->ShowNetworks();
                        if (self->networks_.empty()) {
                            lv_label_set_text(self->status_, "未找到网络，请稍后点击刷新");
                        }
                        ESP_LOGI(TAG, "Listed %u WiFi networks", (unsigned)self->networks_.size());
                    });
                vTaskDelete(nullptr);
            },
            "wifi_ui_scan", 6144, this, 1, nullptr) != pdPASS) {
        busy_ = false;
        lv_label_set_text(status_, "扫描未启动，请点击刷新重试");
    }
}

void TouchWifiSetup::ShowPassword(size_t index) {
    selected_ = index;
    lv_obj_clean(root_);
    list_ = nullptr;
    auto title = lv_label_create(root_);
    lv_obj_set_width(title, lv_pct(100));
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_label_set_text_fmt(title, "连接：%s", networks_[selected_].ssid.c_str());
    auto actions = lv_obj_create(root_);
    lv_obj_set_size(actions, lv_pct(100), 48);
    lv_obj_set_style_pad_all(actions, 0, 0);
    lv_obj_set_style_border_width(actions, 0, 0);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(actions, LV_OBJ_FLAG_SCROLLABLE);
    password_ = lv_textarea_create(actions);
    lv_obj_set_size(password_, 0, 44);
    lv_obj_set_flex_grow(password_, 1);
    lv_textarea_set_one_line(password_, true);
    lv_textarea_set_password_mode(password_, true);
    lv_textarea_set_max_length(password_, 64);
    lv_textarea_set_placeholder_text(
        password_, networks_[selected_].encrypted ? "输入 Wi-Fi 密码" : "开放网络，密码留空");
    Button(actions, "显示", [](lv_event_t* event) {
        auto self = static_cast<TouchWifiSetup*>(lv_event_get_user_data(event));
        if (!self->busy_) {
            bool hidden = !lv_textarea_get_password_mode(self->password_);
            lv_textarea_set_password_mode(self->password_, hidden);
            lv_label_set_text(lv_obj_get_child(lv_event_get_target_obj(event), 0),
                              hidden ? "显示" : "隐藏");
        }
    });
    Button(actions, "连接", [](lv_event_t* event) {
        static_cast<TouchWifiSetup*>(lv_event_get_user_data(event))->Connect();
    });
    status_ = lv_label_create(root_);
    lv_obj_set_width(status_, lv_pct(100));
    lv_label_set_text(status_, "");
    lv_obj_add_flag(status_, LV_OBJ_FLAG_HIDDEN);
    keyboard_ = lv_keyboard_create(root_);
    lv_obj_set_width(keyboard_, lv_pct(100));
    lv_obj_set_flex_grow(keyboard_, 1);
    lv_obj_set_style_text_font(keyboard_, LV_FONT_DEFAULT, 0);
    lv_keyboard_set_map(keyboard_, LV_KEYBOARD_MODE_NUMBER, kNumberMap, kNumberControls);
    lv_keyboard_set_textarea(keyboard_, password_);
    lv_obj_remove_event_cb(keyboard_, lv_keyboard_def_event_cb);
    lv_obj_add_event_cb(
        keyboard_,
        [](lv_event_t* event) {
            auto self = static_cast<TouchWifiSetup*>(lv_event_get_user_data(event));
            if (self->busy_)
                return;
            auto keyboard = lv_event_get_target_obj(event);
            auto button = lv_buttonmatrix_get_selected_button(keyboard);
            if (button == LV_BUTTONMATRIX_BUTTON_NONE)
                return;
            const char* text = lv_buttonmatrix_get_button_text(keyboard, button);
            auto mode = lv_keyboard_get_mode(keyboard);
            if ((mode == LV_KEYBOARD_MODE_TEXT_LOWER || mode == LV_KEYBOARD_MODE_TEXT_UPPER) &&
                strcmp(text, "1#") == 0) {
                lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
            } else {
                lv_keyboard_def_event_cb(event);
            }
        },
        LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(
        keyboard_,
        [](lv_event_t* event) {
            static_cast<TouchWifiSetup*>(lv_event_get_user_data(event))->Connect();
        },
        LV_EVENT_READY, this);
    ESP_LOGI(TAG, "Password entry opened");
}

void TouchWifiSetup::Connect() {
    if (busy_) {
        return;
    }
    busy_ = true;
    auto json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ssid", networks_[selected_].ssid.c_str());
    cJSON_AddStringToObject(json, "password", lv_textarea_get_text(password_));
    auto text = cJSON_PrintUnformatted(json);
    auto job = new std::pair<TouchWifiSetup*, std::string>(this, text);
    cJSON_free(text);
    cJSON_Delete(json);
    lv_obj_remove_flag(status_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(status_, "正在连接…");
    lv_obj_add_state(keyboard_, LV_STATE_DISABLED);
    if (xTaskCreate(
            [](void* context) {
                auto job = static_cast<std::pair<TouchWifiSetup*, std::string>*>(context);
                auto self = job->first;
                auto response = Request("POST", "/submit", std::move(job->second));
                delete job;
                auto result = cJSON_Parse(response.c_str());
                bool success = cJSON_IsTrue(cJSON_GetObjectItem(result, "success"));
                cJSON_Delete(result);
                ESP_LOGI(TAG, "Touch WiFi submission: %s", success ? "success" : "failed");
                Application::GetInstance().Schedule([self, success]() {
                    DisplayLockGuard lock(self->display_);
                    self->busy_ = false;
                    if (self->root_ && self->password_) {
                        lv_obj_remove_state(self->keyboard_, LV_STATE_DISABLED);
                        lv_label_set_text(self->status_, success ? "已保存，正在联网…"
                                                                 : "连接失败，请检查密码后重试");
                    }
                });
                if (success) {
                    WifiManager::GetInstance().StopConfigAp();
                }
                vTaskDelete(nullptr);
            },
            "wifi_ui_connect", 6144, job, 1, nullptr) != pdPASS) {
        delete job;
        busy_ = false;
        lv_obj_remove_state(keyboard_, LV_STATE_DISABLED);
        lv_label_set_text(status_, "连接未启动，请重试");
    }
}
