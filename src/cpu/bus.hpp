#pragma once

#include <cstdint>

// External (20-bit) address space, to be implemented by the specific machine (memory, MMIO, etc.).
class Bus {
public:
    virtual ~Bus() = default;

    virtual uint8_t read(uint32_t addr) = 0;
    virtual void write(uint32_t addr, uint8_t data) = 0;

    // Internal RAM addresses 0xF0-0xFF may be mapped to external devices. These callbacks
    // are called when the CPU reads/writes those addresses.
    // The latched value is the value that was last written to that address.
    virtual uint8_t readIO(uint8_t addr, uint8_t latched) = 0;
    virtual void writeIO(uint8_t addr, uint8_t value) = 0;

    // Returns the accumulated wait states (extra CPU cycles) taken to access slower devices
    // on the bus.
    int takeWaitStates() {
        int ws = waitStates;
        waitStates = 0;
        return ws;
    }

    uint32_t read24(uint32_t addr) {
        return
            read((addr + 0) & 0xFFFFF) << 0 |
            read((addr + 1) & 0xFFFFF) << 8 |
            read((addr + 2) & 0xFFFFF) << 16;
    }

protected:
    int waitStates = 0;
};
