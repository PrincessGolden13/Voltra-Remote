#pragma once
#include "state/TrainingState.h"
#include <map>

namespace voltra {
// Local presentation only. Black closes its statistics segment on each base
// change. A user dropset spans those segments until actual between-set rest or
// unload. An extended-cable RP hold remains within the active device workout.
// Never feed these counters back into the protocol or automatic drop triggers.
class DropSetProgress {
public:
    bool visible = false, active = false, valid = true;
    int completedSets = 0, reps = 0, lastReps = 0;

    void observe(const TrainingReport& r, bool enabled, uint64_t now) {
        const bool newMotion = r.motionRevision != motionRevision_;
        const bool newRep = r.repRevision != repRevision_;
        motionRevision_ = r.motionRevision; repRevision_ = r.repRevision;
        const bool motionFresh = r.motionRevision && now >= r.motionAtMs && now-r.motionAtMs <= 500;
        const bool starts = r.loaded && r.setActive.value_or(true) && newMotion && motionFresh &&
            r.setNumber && (r.moving || r.rep > 0);
        if (!active && starts) {
            if (!enabled) { visible = false; return; }
            // Preserve prior ordinary sets when entering drops, but never use
            // Black's weight-segment total after we have started grouping.
            if (!visible) { completedSets = r.completedSets.value_or(0); lastReps = r.lastSetReps.value_or(0); }
            visible = active = valid = true; reps = 0; segments_.clear(); lastMotionSet_.reset();
        }
        if (!active) return;
        if (newMotion && motionFresh && r.setNumber) {
            // AA81's set ID is one byte. Bound memory even on corrupt streams;
            // unknown counts stay unknown rather than inventing missing reps.
            constexpr size_t maximumSegments = 256;
            if ((lastMotionSet_ && lastMotionSet_ != r.setNumber && segments_.count(*r.setNumber)) ||
                (!segments_.count(*r.setNumber) && segments_.size() >= maximumSegments)) valid = false;
            else add(*r.setNumber, r.rep);
            lastMotionSet_ = r.setNumber;
        }
        // Late/duplicate completion reports can only amend an observed segment.
        // An old completion retained across a new load cannot start a segment.
        if (newRep && r.repSet && segments_.count(*r.repSet) && now >= r.repAtMs && now-r.repAtMs <= 2000)
            add(*r.repSet, r.completedRep);
        if (!r.loaded || r.resting==true) {
            active = false;
            if (reps > 0) { ++completedSets; lastReps = reps; }
        }
    }
private:
    uint64_t motionRevision_ = 0, repRevision_ = 0;
    std::optional<int> lastMotionSet_;
    std::map<int, uint32_t> segments_;
    void add(int set, uint32_t count) {
        if (!valid || count > 10000) { valid = false; return; }
        auto& previous = segments_[set];
        if (count > previous) { reps += int(count-previous); previous = count; }
    }
};
}
