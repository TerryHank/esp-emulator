#pragma once
#include <algorithm>
#include "vesc_protocol.h"

// Per-power-cycle estimates. Never integrate across missing/stale samples or changed calibration.
class VescTrip {
public:
    void Reset() { *this = {}; }
    void Update(const vesc::Snapshot& state) {
        if (!state.online) {
            if (seen)
                partial = true;
            continuous_ = false;
            return;
        }
        seen = true;
        if (continuous_ && state.sequence == previous_.sequence)
            return;
        if (continuous_) {
            auto dt =
                state.sample_ms >= previous_.sample_ms ? state.sample_ms - previous_.sample_ms : 0;
            bool calibrated = state.config.wheel_mm == previous_.config.wheel_mm &&
                              state.config.pole_pairs == previous_.config.pole_pairs &&
                              state.config.gear_milli == previous_.config.gear_milli;
            if (dt > 0 && dt <= vesc::kStaleAfterMs) {
                time_ms += dt;
                auto before = previous_.SpeedKmh(), after = state.SpeedKmh();
                if (calibrated && before && after) {
                    distance_known = true;
                    distance_km += (*before + *after) / 2 * dt / 3600000.0;
                    speed_time_ms += dt;
                    max_kmh = std::max({max_kmh, *before, *after});
                } else
                    partial = true;
                auto energy = state.values.watt_hours - previous_.values.watt_hours;
                auto charged =
                    state.values.watt_hours_charged - previous_.values.watt_hours_charged;
                if (energy >= 0 && charged >= 0) {
                    used_wh += energy;
                    recovered_wh += charged;
                } else
                    partial = true;
            } else
                partial = true;
        }
        previous_ = state;
        continuous_ = true;
    }
    bool seen = false, partial = false, distance_known = false;
    uint64_t time_ms = 0, speed_time_ms = 0;
    double distance_km = 0, used_wh = 0, recovered_wh = 0, max_kmh = 0;

private:
    vesc::Snapshot previous_;
    bool continuous_ = false;
};
