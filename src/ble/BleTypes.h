#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace voltra {
using Bytes = std::vector<uint8_t>;
using DeviceId = std::string; // Opaque transport ID, never a global singleton.
enum class TransportResult { Accepted, Disconnected, Unsupported, Failed };
struct Characteristic { std::string uuid; bool canNotify = false, canWrite = false; };
struct Advertisement { DeviceId id; std::string name; std::vector<std::string> services; };
}
