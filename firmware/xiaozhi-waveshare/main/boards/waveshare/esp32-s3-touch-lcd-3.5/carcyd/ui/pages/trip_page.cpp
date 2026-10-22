#include "trip_page.h"

#include <cstdio>

#include "../../core/platform.h"
#include "../../core/settings.h"
#include "../../core/units.h"
#include "../components/dialog.h"
#include "../theme.h"
#include "../widgets.h"
#include "carcyd_port.h"
namespace ui {
const char* TripPage::title() const { return "行程"; }
void TripPage::create(lv_obj_t* parent) {
  root_ = column(parent, layout::kGap);
  lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(root_, layout::kPad, 0);
  lv_obj_add_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(root_, LV_DIR_VER);
  auto grid = box(root_);
  lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_gap(grid, layout::kGap, 0);
  const char* names[] = {"行驶距离", "有效采样时间", "平均速度", "最高速度", "耗电量", "回收电量"};
  const char* glyphs[] = {ICON_ROAD, ICON_TIMER, ICON_SIGMA, ICON_ARROW_UP, ICON_FLASH, ICON_BATTERY};
  int cols = layout::kScreenW > layout::kScreenH ? 3 : 2;
  int width = (layout::kContentW - 2 * layout::kPad - (cols - 1) * layout::kGap) / cols;
  for (int i = 0; i < 6; ++i) {
    auto c = card(grid);
    lv_obj_set_size(c, width, 78);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    auto head = row(c, 4);
    icon(head, glyphs[i], font::caption(), color::accent());
    label(head, font::caption(), color::text_dim(), names[i]);
    auto line = row(c, 3);
    stats_[i].value = label(line, font::value(), color::text(), "--");
    stats_[i].unit = label(line, font::caption(), color::text_dim(), "");
  }
  accel_values_ = label(root_, font::caption(), color::text_dim(), "");
  lv_obj_set_width(accel_values_, lv_pct(100));
  button(root_, ICON_RESTORE, "重置本次行程", ButtonStyle::Secondary, reset_cb, this);
}
void TripPage::on_show() { last_update_ms_ = 0; }
void TripPage::update(uint32_t now) {
  if (last_update_ms_ && now - last_update_ms_ < 250) return;
  last_update_ms_ = now;
  auto& t = carcyd_trip();
  const auto& s = core::settings::get();
  const char* units[] = {core::units::distance_label(s),
                         "",
                         core::units::label(core::Quantity::Speed, s),
                         core::units::label(core::Quantity::Speed, s),
                         "Wh",
                         "Wh"};
  double values[] = {
      core::units::distance(t.distance_km, s),
      0,
      core::units::convert(core::Quantity::Speed,
                           t.speed_time_ms ? t.distance_km * 3600000.0 / t.speed_time_ms : 0, s),
      core::units::convert(core::Quantity::Speed, t.max_kmh, s),
      t.used_wh,
      t.recovered_wh};
  for (int i = 0; i < 6; ++i) {
    char buf[32];
    bool known = t.seen && ((i != 0 && i != 2 && i != 3) || t.distance_known);
    if (!known)
      snprintf(buf, sizeof(buf), "--");
    else if (i == 1)
      core::units::format_duration(t.time_ms / 1000, buf, sizeof(buf));
    else
      snprintf(buf, sizeof(buf), i == 0 ? "%.3f" : "%.1f", values[i]);
    set_text(stats_[i].value, buf);
    set_text(stats_[i].unit, units[i]);
  }
  set_text(accel_values_, !t.seen     ? "尚未连接 VESC"
                          : t.partial ? "本次估算存在缺失数据"
                                      : "仅累计连续有效的采样区间");
}
void TripPage::reset_cb(lv_event_t* e) {
  dialog::confirm("重置本次行程", "清空本次仪表累计的距离和能量？", "重置", false, reset_confirmed,
                  lv_event_get_user_data(e));
}
void TripPage::reset_confirmed(void* user) {
  carcyd_trip().Reset();
  static_cast<TripPage*>(user)->last_update_ms_ = 0;
}
}  // namespace ui
