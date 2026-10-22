// SPDX-License-Identifier: Apache-2.0
#include "carcyd_lvgl_simd.h"
#include "lvgl.h"
#include "src/draw/sw/blend/lv_draw_sw_blend_private.h"

typedef struct {
    uint32_t opa;
    void* dest;
    uint32_t width, height, dest_stride;
    const void* source;
    uint32_t source_stride;
    const lv_opa_t* mask;
    uint32_t mask_stride;
} CarcydEspBlend;

_Static_assert(sizeof(CarcydEspBlend) == 36, "ESP32-S3 assembly descriptor ABI");
extern int lv_color_blend_to_rgb565_esp(const CarcydEspBlend* descriptor);
extern int lv_rgb565_blend_normal_to_rgb565_esp(const CarcydEspBlend* descriptor);

int carcyd_blend_fill_opaque(const void* descriptor) {
    const lv_draw_sw_blend_fill_dsc_t* d = descriptor;
    if (d->mask_buf || d->opa < LV_OPA_MAX || d->dest_w <= 0 || d->dest_h <= 0)
        return LV_RESULT_INVALID;
    lv_color32_t color = lv_color_to_32(d->color, LV_OPA_COVER);
    CarcydEspBlend esp = {d->opa, d->dest_buf, d->dest_w, d->dest_h, d->dest_stride,
                          &color, 0,           NULL,      0};
    return lv_color_blend_to_rgb565_esp(&esp);
}

int carcyd_blend_copy_opaque(const void* descriptor) {
    const lv_draw_sw_blend_image_dsc_t* d = descriptor;
    if (d->mask_buf || d->opa < LV_OPA_MAX || d->blend_mode != LV_BLEND_MODE_NORMAL ||
        d->src_color_format != LV_COLOR_FORMAT_RGB565 || d->dest_w <= 0 || d->dest_h <= 0)
        return LV_RESULT_INVALID;
    CarcydEspBlend esp = {d->opa,     d->dest_buf,   d->dest_w, d->dest_h, d->dest_stride,
                          d->src_buf, d->src_stride, NULL,      0};
    return lv_rgb565_blend_normal_to_rgb565_esp(&esp);
}
