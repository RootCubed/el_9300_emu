#pragma once

#include "cpu/sc62015.hpp"
#include "machine/memory.hpp"

#include <string>

// The complete EL-9300: SC62015 CPU, memory map, keyboard and LCD.
class EL9300 {
public:
    // Main clock: 2.304 MHz ceramic resonator.
    static constexpr uint32_t CLOCK_HZ = 2304000;

    // CPU speed in CPU cycles per second.
    static constexpr uint32_t CYCLES_PER_SECOND = CLOCK_HZ / SC62015::CLOCK_PERIODS_PER_CYCLE;

    // Sub-timer period (0.5 s) in CPU cycles.
    static constexpr uint32_t SUB_TIMER_PERIOD = CYCLES_PER_SECOND / 2;

    explicit EL9300(const std::vector<uint8_t> &rom);

    // Hardware reset. RAM contents are kept.
    void reset();
    // Executes one CPU step and returns the number of cycles consumed.
    int step() { return cpu.step(); }

    // Save / restore the whole machine (CPU, RAM, LCD memory). The real calculator
    // keeps RAM while switched off, so the GUI saves on exit and restores on start.
    bool saveState(const std::string &path) const;
    bool loadState(const std::string &path);

    Keyboard &keyboard() { return memory.keyboard; }
    const Lcd &lcd() const { return memory.lcd; }

    EL9300Mem memory;
    SC62015 cpu;

private:
    // Passes the CPU's I/O registers (F0h-FFh) to the peripherals again, after the CPU
    // changed them without going through the bus (reset, loading a state).
    void syncIO();
};
