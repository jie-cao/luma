// Timeline-lite skeleton: holds current time and can evaluate curves later.
#pragma once

#include <cstdint>

namespace luma {

class TimelineLite {
public:
    void set_time(float t) { time_ = t; }
    float time() const { return time_; }

    // Future: Evaluate curves/tracks; here we only keep the time cursor.
    void step(float dt) { time_ += dt; }

private:
    float time_{0.0f};
};

}  // namespace luma

