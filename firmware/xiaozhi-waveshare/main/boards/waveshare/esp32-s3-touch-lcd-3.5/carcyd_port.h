#pragma once
#include "carcyd/core/types.h"
#include "vesc_protocol.h"
#include "vesc_trip.h"

void carcyd_show_chat(bool show);
bool carcyd_chat_visible();
void carcyd_open_wifi();
bool carcyd_signal_available(core::SignalId id);
vesc::Snapshot carcyd_snapshot();
VescTrip& carcyd_trip();
void carcyd_save_config(int field, int value);
