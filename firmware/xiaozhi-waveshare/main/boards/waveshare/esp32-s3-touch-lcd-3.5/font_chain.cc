#include "font_chain.h"
extern "C" {
LV_FONT_DECLARE(carcyd_zh_12); LV_FONT_DECLARE(carcyd_zh_14); LV_FONT_DECLARE(carcyd_zh_16);
LV_FONT_DECLARE(carcyd_zh_20); LV_FONT_DECLARE(carcyd_zh_28);
}
const lv_font_t* chinese_font(const lv_font_t* original, int size) {
    static lv_font_t fonts[5]{};
    static bool initialized[5]{};
    const lv_font_t* fallback[] = {&carcyd_zh_12, &carcyd_zh_14, &carcyd_zh_16, &carcyd_zh_20, &carcyd_zh_28};
    int index = size == 12 ? 0 : size == 14 ? 1 : size == 16 ? 2 : size == 20 ? 3 : 4;
    if (!initialized[index]) {
        fonts[index] = *original;
        fonts[index].fallback = fallback[index];
        initialized[index] = true;
    }
    return &fonts[index];
}
