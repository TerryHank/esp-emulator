#pragma once
#include <memory>
#include "display.h"
#include "voice_orb.h"

// Embeds CarCYD's original UI while retaining Xiaozhi's conversation widgets.
class VescDashboard {
public:
    VescDashboard(Display* display, lv_obj_t* container, lv_obj_t* chat, lv_obj_t* icons,
                  lv_obj_t* state);
    void Resize(int width, int height);
    void SetMessage(const char* role, const char* message);
    void ShowChat(bool show);
    void SetChatFont(const lv_font_t* font);
    bool IsChatPage() const { return chat_open_; }

private:
    void Refresh();
    void UpdateOrb();
    lv_obj_t* chat_layer_;
    lv_obj_t* chat_header_;
    lv_obj_t* chat_body_;
    lv_obj_t* orb_area_;
    lv_obj_t* chat_;
    lv_obj_t* state_;
    lv_obj_t* status_;
    bool chat_open_ = false;
    bool portrait_ = false;
    bool auto_chat_ = false;
    bool awaiting_reply_ = false;
    std::unique_ptr<VoiceOrb> orb_;
};
