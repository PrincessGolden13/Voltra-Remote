#pragma once
#include "TrainingState.h"
#include <vector>
namespace voltra {
// Stored modifier recipes never contain base weight or a load request.
struct Preset {
    MagicMode mode = MagicMode::Chains;
    Modifier magic, eccentric;
    WeightUnit unit = WeightUnit::Pounds;
    DropSettings drops;
};
inline bool sameModifier(Modifier a, Modifier b) { return a.enabled == b.enabled && a.value == b.value && a.percent == b.percent; }
inline bool samePreset(const Preset& a, const Preset& b) {
    return a.mode == b.mode && sameModifier(a.magic,b.magic) && sameModifier(a.eccentric,b.eccentric)
        && a.unit == b.unit && a.drops.enabled == b.drops.enabled
        && a.drops.mode == b.drops.mode && a.drops.step == b.drops.step && a.drops.drops == b.drops.drops
        && a.drops.percent == b.drops.percent && a.drops.holdSeconds == b.drops.holdSeconds;
}
struct RemoteConfiguration {
    int theme = 7; // Teal; indices follow the settings picker.
    int restAlertSeconds = 0, dimMinutes = 15, sleepMinutes = 20;
    std::vector<Preset> presets;
};
class RemoteConfigurationStore {
public:
    virtual ~RemoteConfigurationStore() = default;
    virtual RemoteConfiguration load() = 0;
    virtual bool save(const RemoteConfiguration&) = 0;
};
}
