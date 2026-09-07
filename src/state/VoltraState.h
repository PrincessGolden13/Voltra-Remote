#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace voltra {
enum class Knowledge { Unknown, No, Yes };
enum class LoadState { Unknown, Unloaded, Loaded };
enum class TrainingMode { Unknown, WeightTraining, Other };

template <typename T> struct Observed {
    std::optional<T> value;
    uint64_t receivedAtMs = 0;
    bool fresh(uint64_t now, uint64_t maxAgeMs) const {
        return value.has_value() && now >= receivedAtMs && now - receivedAtMs <= maxAgeMs;
    }
};

// Catalog-backed raw observation only. Bit layout and live role are unverified.
struct TwinState {
    Observed<uint32_t> remoteStatusRaw;
    Knowledge active = Knowledge::Unknown;
};

struct VoltraCapabilities {
    Knowledge weightTraining = Knowledge::Unknown;
    Knowledge chains = Knowledge::Unknown;
    Knowledge eccentric = Knowledge::Unknown;
    Knowledge inverseChains = Knowledge::Unknown;
    std::optional<uint16_t> effectiveMaxWeightLbs;
    // No production capability inference is installed in the foundation.
};

struct VoltraState {
    bool connected = false;
    bool handshakeValid = false;
    Observed<uint16_t> weightLbs;
    Observed<uint16_t> chainsLbs;
    Observed<int16_t> eccentricLbs;
    Observed<uint16_t> fitnessModeRaw;
    Observed<uint8_t> workoutStateRaw;
    Observed<uint8_t> batteryPercent;
    TwinState twin;
    VoltraCapabilities capabilities;
    std::string firmware; // Empty until reported; never a capability shortcut.
};
}
