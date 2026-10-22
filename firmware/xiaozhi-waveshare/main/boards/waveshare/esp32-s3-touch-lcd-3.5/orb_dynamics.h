// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

struct OrbDynamics {
    enum class State { Idle, Listening, Thinking, Speaking };
    float level = 0;
    float listening = 0;
    float thinking = 0;
    float speaking = 0;
    float phase = 0;
    float breath = 0;

    void Update(State state, uint8_t amplitude, float seconds) {
        const float dt = std::clamp(seconds, 0.0f, 0.1f);
        const float target = amplitude / 255.0f;
        const float envelope_alpha = -std::expm1(-dt / (target > level ? .035f : .14f));
        const float mode_alpha = -std::expm1(-dt / .18f);
        level += (target - level) * envelope_alpha;
        listening += ((state == State::Listening ? 1.0f : 0.0f) - listening) * mode_alpha;
        thinking += ((state == State::Thinking ? 1.0f : 0.0f) - thinking) * mode_alpha;
        speaking += ((state == State::Speaking ? 1.0f : 0.0f) - speaking) * mode_alpha;
        // Integrate speed, rather than multiplying absolute uptime by a new speed
        // on each state switch. Colour centers therefore never teleport.
        constexpr float turn = 6.283185307f;
        phase = std::fmod(phase + dt * (.9f + .4f * listening + 1.1f * thinking + 1.6f * speaking),
                          turn);
        breath = std::fmod(breath + dt * 1.8f, turn);
    }
};
