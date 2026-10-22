#pragma once

#include <cstdlib>

class ScreenOrientation {
public:
    // Calibrated on this board: upright landscape is -Y; a clockwise turn is -X.
    static int Detect(int x, int y) {
        const int ax = std::abs(x);
        const int ay = std::abs(y);
        if (ax < 8192 && ay < 8192) {
            return -1;  // Near-flat: gravity does not define a screen direction.
        }
        if (ax * 4 > ay * 5) {
            return x < 0 ? 90 : 270;
        }
        if (ay * 4 > ax * 5) {
            return y < 0 ? 0 : 180;
        }
        return -1;  // Keep the current direction around the diagonal boundary.
    }

    static int CameraRotation(int screen_degrees) { return (270 + screen_degrees) % 360; }

    int Update(int x, int y) {
        int candidate = Detect(x, y);
        if (candidate < 0 || candidate == current_) {
            pending_ = -1;
            samples_ = 0;
            return -1;
        }
        if (candidate != pending_) {
            pending_ = candidate;
            samples_ = 0;
        }
        if (++samples_ < 3) {
            return -1;
        }
        current_ = candidate;
        pending_ = -1;
        samples_ = 0;
        return current_;
    }

private:
    int current_ = 0;
    int pending_ = -1;
    int samples_ = 0;
};
