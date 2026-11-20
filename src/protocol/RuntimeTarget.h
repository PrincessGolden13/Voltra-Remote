#pragma once
#include "FrameParser.h"

namespace voltra {
// One recognized independent B4 sample. A recognized sample without a valid
// target explicitly clears the previous target; unknown packet shapes do not.
struct RuntimeTarget {
    std::optional<int> targetTenthsLb;
};
std::optional<RuntimeTarget> decodeRuntimeTarget(const Frame&);
}
