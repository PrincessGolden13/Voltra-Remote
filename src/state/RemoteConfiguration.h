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
    // Compare effective recipes, not dormant values left in disabled controls.
    const auto match = [](Modifier x, Modifier y) {
        return x.enabled == y.enabled && (!x.enabled || sameModifier(x,y));
    };
    return (!a.magic.enabled || a.mode == b.mode) && match(a.magic,b.magic) && match(a.eccentric,b.eccentric)
        && a.unit == b.unit && a.drops.enabled == b.drops.enabled
        && (!a.drops.enabled || (a.drops.mode == b.drops.mode &&
            (a.drops.mode == DropMode::Stairs ? a.drops.step == b.drops.step :
             a.drops.drops == b.drops.drops && a.drops.percent == b.drops.percent && a.drops.holdSeconds == b.drops.holdSeconds)));
}
struct RemoteConfiguration {
    int theme = 7; // Teal; indices follow the settings picker.
    int restAlertSeconds = 0; // Off on first use and factory reset; preserve explicit saved choices.
    int dimMinutes = 15, sleepMinutes = 20;
    std::vector<Preset> presets;
};
class RemoteConfigurationStore {
public:
    virtual ~RemoteConfigurationStore() = default;
    virtual RemoteConfiguration load() = 0;
    virtual bool save(const RemoteConfiguration&) = 0;
};
}
