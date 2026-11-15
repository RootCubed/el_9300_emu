#include "gui/app.hpp"

#include "gui/debugger_view.hpp"
#include "gui/keypad_view.hpp"

// The file to save the machine state to.
const char *const STATE_FILE = "state.bin";

App::App(SDL_Renderer *renderer, const std::vector<uint8_t> &rom) : emu(rom), lcdView(renderer) {
    if (emu.loadState(STATE_FILE)) {
        status = std::string("Restored ") + STATE_FILE;
    }
}

void App::frame(float dt) {
    // Input from the previous frame's on-screen keypad.
    applyKeyStates(emu.keyboard(), keypadKeys);

    run.runFrame(emu, dt);

    debuggerView.draw(*this);
    lcdView.draw(emu);
    keypadKeys = drawKeypad(emu.keyboard());
}

void App::reset() {
    run.lastError.clear();
    emu.reset();
}

void App::saveState() {
    status = emu.saveState(STATE_FILE) ? std::string("Saved ") + STATE_FILE : "Save failed";
}

void App::loadState() {
    status = emu.loadState(STATE_FILE) ? std::string("Loaded ") + STATE_FILE : "No valid state file";
}
