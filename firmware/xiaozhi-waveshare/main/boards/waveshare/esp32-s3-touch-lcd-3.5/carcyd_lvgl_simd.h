// SPDX-License-Identifier: Apache-2.0
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int carcyd_blend_fill_opaque(const void* descriptor);
int carcyd_blend_copy_opaque(const void* descriptor);
#ifdef __cplusplus
}
#endif
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565(dsc) carcyd_blend_fill_opaque(dsc)
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565(dsc) carcyd_blend_copy_opaque(dsc)
