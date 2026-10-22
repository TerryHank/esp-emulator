#pragma once

#include <string>
#include <vector>
#include "display.h"

class TouchWifiSetup {
public:
    explicit TouchWifiSetup(Display* display) : display_(display) {}
    void Show();
    void Hide();
    void Back();

private:
    struct Network {
        std::string ssid;
        int rssi;
        bool encrypted;
    };
    Display* display_;
    lv_obj_t* root_ = nullptr;
    lv_obj_t* list_ = nullptr;
    lv_obj_t* status_ = nullptr;
    lv_obj_t* password_ = nullptr;
    lv_obj_t* keyboard_ = nullptr;
    std::vector<Network> networks_;
    size_t selected_ = 0;
    bool busy_ = false;

    void ShowNetworks();
    void Scan();
    void ShowPassword(size_t index);
    void Connect();
    lv_obj_t* Button(lv_obj_t* parent, const char* text, lv_event_cb_t callback);
    static std::string Request(const char* method, const char* path, std::string body = "");
};
