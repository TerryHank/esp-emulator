#pragma once
#include <lvgl.h>
#if LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR < 6
#define CARCYD_FLAG_SETTER(name, flag)                             \
    static inline void lv_obj_set_##name(lv_obj_t* obj, bool on) { \
        if (on)                                                    \
            lv_obj_add_flag(obj, flag);                            \
        else                                                       \
            lv_obj_remove_flag(obj, flag);                         \
    }
CARCYD_FLAG_SETTER(clickable, LV_OBJ_FLAG_CLICKABLE)
CARCYD_FLAG_SETTER(scrollable, LV_OBJ_FLAG_SCROLLABLE)
CARCYD_FLAG_SETTER(event_bubble, LV_OBJ_FLAG_EVENT_BUBBLE)
CARCYD_FLAG_SETTER(gesture_bubble, LV_OBJ_FLAG_GESTURE_BUBBLE)
CARCYD_FLAG_SETTER(ignore_layout, LV_OBJ_FLAG_IGNORE_LAYOUT)
CARCYD_FLAG_SETTER(hidden, LV_OBJ_FLAG_HIDDEN)
#undef CARCYD_FLAG_SETTER
static inline bool lv_obj_is_hidden(const lv_obj_t* obj) {
    return lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
#endif
