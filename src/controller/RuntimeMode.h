#pragma once
namespace voltra {
// Host-owned switching: hardware reboots between separately linked images.
// UI never selects a physical transport or forwards synthetic commands to one.
class RuntimeMode {
public:
    virtual ~RuntimeMode() = default;
    virtual bool simulated() const = 0;
    virtual bool switchMode() = 0;
};
}
