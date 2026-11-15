#pragma once

#include <string>

class EL9300;

// Decides how much to emulate per GUI frame and catches emulation errors.
class RunControl {
public:
    bool running = true;
    bool maxSpeed = false;
    std::string lastError;

    // Emulates the device for dt seconds.
    void runFrame(EL9300 &emu, float dt);
    // Executes a single instruction and pauses.
    void stepOnce(EL9300 &emu);
};
