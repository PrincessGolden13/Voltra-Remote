#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace voltra {
// Presentation policy in logical detents, independent of GPIO/pulse mapping.
class RotaryAdjustment {
public:
    void reset() { *this = {}; }
    int delta(int detents, int current, uint64_t now, int ticksPerWholeUnit) {
        if (!detents) return 0;
        const int direction = detents > 0 ? 1 : -1;
        const int count = std::abs(detents);
        // Tunable feel: rest/reversal starts fine; a brief burst cannot jump
        // directly to the highest gear. Fine input retains every logical detent.
        constexpr uint64_t restMs = 300, rampMs = 120;
        constexpr int rampDetents = 3;
        constexpr double filterMs = 160.0;
        constexpr double enterRate[] = {0, 10, 18, 32}; // detents per second
        constexpr double leaveRate[] = {0, 8, 14, 26};
        if (!lastAt_ || now <= lastAt_ || now-lastAt_ >= restMs || direction != direction_) {
            reset(); direction_ = direction; levelAt_ = now;
        } else {
            const auto elapsed = now-lastAt_;
            const double rawRate = count*1000.0/elapsed;
            const double alpha = elapsed/(elapsed+filterMs);
            rate_ = rate_ == 0 ? rawRate : rate_ + alpha*(rawRate-rate_);
            // Filter acceleration, but let a deliberate slow turn brake at once.
            const double speed = std::min(rawRate,rate_);
            detentsInLevel_ += count;
            int desired = level_;
            while (desired > 0 && speed < leaveRate[desired]) --desired;
            while (desired < 3 && speed >= enterRate[desired+1]) ++desired;
            if (desired < level_ || (desired > level_ && now-levelAt_ >= rampMs && detentsInLevel_ >= rampDetents)) {
                level_ = desired < level_ ? desired : level_+1;
                levelAt_ = now; detentsInLevel_ = 0;
            }
        }
        lastAt_ = now;
        const int quantum = level_ == 0 ? 1 : level_ == 1 ? 2 : (level_ == 2 ? 5 : 10)*ticksPerWholeUnit;
        // First accelerated step reaches the next grid mark in the direction of
        // travel. Positive modulo also handles negative stairs amounts correctly.
        const int remainder = (current%quantum+quantum)%quantum;
        const int first = direction > 0 ? quantum-remainder : -(remainder ? remainder : quantum);
        return first + direction*(count-1)*quantum;
    }
private:
    uint64_t lastAt_ = 0, levelAt_ = 0;
    double rate_ = 0;
    int direction_ = 0, level_ = 0, detentsInLevel_ = 0;
};
}
