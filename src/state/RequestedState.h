#pragma once
#include <cstdint>
#include <optional>
namespace voltra {
enum class CommandStatus { None, Draft, Pending, Confirmed, Failed, Timeout };
struct RequestedState {
    std::optional<uint16_t> weightLbs;
    CommandStatus status = CommandStatus::None;
};
}
