/**
 * @file trip_page.h
 * Trip computer backed by continuous, valid VESC samples.
 */
#pragma once

#include "page.h"

namespace ui {

class TripPage final : public Page {
 public:
  void create(lv_obj_t* parent) override;
  const char* title() const override;
  void on_show() override;
  void update(uint32_t now_ms) override;

 private:
  struct Stat {
    lv_obj_t* value;
    lv_obj_t* unit;
  };

  static void reset_cb(lv_event_t* e);
  static void reset_confirmed(void* user);

  Stat stats_[6] = {};
  lv_obj_t* accel_values_ = nullptr;
  uint32_t last_update_ms_ = 0;
};

}  // namespace ui
