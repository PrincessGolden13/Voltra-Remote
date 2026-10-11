#pragma once
#include "FrameParser.h"
#include "state/VoltraState.h"
namespace voltra {
namespace parameter {
constexpr uint16_t baseWeight = 0x3e86, chains = 0x3e87, eccentric = 0x3e88;
constexpr uint16_t fitnessMode = 0x3e89, workoutState = 0x4fb0;
constexpr uint16_t battery = 0x4e2d, legacyBattery = 0x1b5d, coworkerRemoteStatus = 0x521f;
constexpr uint8_t asyncUpdate = 0x10;
}
// Atomic decode: unknown widths/truncation invalidate this whole update.
// Reads/ACKs and other telemetry are deliberately not guessed.
bool applyAsyncParameters(const Frame&, uint64_t nowMs, VoltraState&);
}
