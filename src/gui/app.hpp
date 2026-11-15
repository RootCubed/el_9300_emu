#pragma once

#include "gui/keypad_view.hpp"
#include "gui/debugger_view.hpp"
#include "gui/lcd_view.hpp"
#include "gui/run_control.hpp"
#include "machine/el9300.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

// The emulator GUI, owns the machine and GUI views.
class App {
public:
    App(SDL_Renderer *renderer, const std::vector<uint8_t> &rom);

    // Performs one GUI frame.
    void frame(float dt);

    void reset(); // Like the physical RESET switch
    void saveState(); // Saves the machine state to disk
    void loadState(); // Loads the machine state from disk

    EL9300 &machine() { return emu; }
    RunControl &runControl() { return run; }
    const std::string &getStatus() const { return status; }

private:
    EL9300 emu;
    RunControl run;

    DebuggerView debuggerView;
    LcdView lcdView;

    KeyStates keypadKeys{};
    std::string status;
};
