#include "gui/run_control.hpp"

#include "machine/el9300.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <exception>

void RunControl::runFrame(EL9300 &emu, float dt) {
    if (!running) {
        return;
    }

    // If the GUI freezes for a long time, don't try to catch up entirely
    dt = std::min(dt, 0.5f);

    try {
        if (maxSpeed) {
            // Get as many cycles as possible within the time frame.
            const uint64_t deadline = SDL_GetTicksNS() + 1'000'000'000 / 60;
            while (SDL_GetTicksNS() < deadline) {
                for (int i = 0; i < 1000; i++) {
                    emu.step();
                }
            }
        } else {
            const uint64_t deadline = emu.cpu.getCycles() + uint64_t(dt * EL9300::CYCLES_PER_SECOND);
            while (emu.cpu.getCycles() < deadline) {
                emu.step();
            }
        }
    } catch (const std::exception &e) {
        running = false;
        lastError = e.what();
    }
}

void RunControl::stepOnce(EL9300 &emu) {
    running = false;
    try {
        emu.step();
    } catch (const std::exception &e) {
        lastError = e.what();
    }
}
