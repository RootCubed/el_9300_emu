#include "machine/keyboard.hpp"

#include "cpu/sc62015.hpp"

#include <cstring>

// {name, KO line, KI bit}. Listed in keypad order, top left to bottom right.
const Keyboard::KeyInfo Keyboard::KEYS[NUM_KEYS] = {
    {"CALC", 9, 6}, {"GRAPH", 9, 5}, {"PROG", 9, 4}, {"DATA", 9, 3},
    {"EQTN", 8, 7}, {"RANGE", 8, 6}, {"ZOOM", 8, 5}, {"Up", 8, 3},
    {"MATH", 7, 7}, {"MENU", 7, 6}, {"QUIT", 7, 5}, {"Left", 7, 4}, {"Down", 7, 3}, {"Right", 7, 2},
    {"2ndF", 6, 7}, {"ALPHA", 6, 6}, {"SETUP", 6, 5}, {"DEL", 6, 4}, {"BS", 6, 3}, {"CL", 6, 2},
    {"sin", 5, 7}, {"cos", 5, 6}, {"tan", 5, 5}, {"x^2", 5, 4}, {"log", 5, 3}, {"ln", 5, 2},
    {"Exp", 4, 7}, {"a/b", 4, 6}, {"a^b", 4, 5}, {"sqrt", 4, 4}, {"STO", 4, 3}, {"X", 4, 2},
    {"7", 3, 6}, {"8", 3, 5}, {"9", 3, 4}, {"(", 3, 3}, {")", 3, 2},
    {"4", 2, 6}, {"5", 2, 5}, {"6", 2, 4}, {"x", 2, 3}, {"/", 2, 2},
    {"1", 1, 6}, {"2", 1, 5}, {"3", 1, 4}, {"+", 1, 3}, {"-", 1, 2},
    {"0", 0, 6}, {".", 0, 5}, {"(-)", 0, 4}, {"ENTER", 0, 3},
};

int Keyboard::findKey(const char *name) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (std::strcmp(KEYS[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void Keyboard::reset() {
    kol = koh = 0;
    disableStrobe = false;
    prevKI = readKI();
    prevOnKey = onKeyLevel();
}

void Keyboard::setKey(int index, bool down) {
    if (index < 0 || index >= NUM_KEYS) {
        return;
    }

    const KeyInfo &k = KEYS[index];
    if (down) {
        KOxKI[k.ko] |= 1 << k.ki;
    } else {
        KOxKI[k.ko] &= ~(1 << k.ki);
    }
    checkInterrupts();
}

void Keyboard::setOnKey(bool down) {
    onKey = down;
    checkInterrupts();
}

void Keyboard::releaseAll() {
    for (uint8_t &ki : KOxKI) {
        ki = 0;
    }
    onKey = false;
    prevKI = readKI();
    prevOnKey = onKeyLevel();
}

uint8_t Keyboard::readKI() const {
    uint8_t ki = 0;
    const uint16_t ko = getKOLines();

    for (uint8_t koLine = 0; koLine < 16; koLine++) {
        if ((ko >> koLine) & 1) {
            // If KO is active, KI becomes active if there is a key held on that KO line.
            ki |= KOxKI[koLine];
        }
    }

    return ki;
}

void Keyboard::checkInterrupts() {
    // A key interrupt is raised when a newly pressed key becomes visible on KI.
    const uint8_t ki = readKI();
    if ((ki & ~prevKI) && cpu) {
        cpu->requestInterrupt(SC62015::INT_KEYI);
    }
    prevKI = ki;

    // The ON-key interrupt is raised on a rising edge of the ON-key input.
    const bool on = onKeyLevel();
    if (on && !prevOnKey && cpu) {
        cpu->requestInterrupt(SC62015::INT_ONKI);
    }
    prevOnKey = on;
}
