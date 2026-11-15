#pragma once

#include "cpu/sc62015.hpp"

#include <cstdint>

// EL-9300 key matrix.
//
// The keys sit on 10 strobe outputs (KO0-KO9) and 6 inputs (KI bits 2-7).
// A held key connects its KO line to its KI bit.
class Keyboard {
public:
    struct KeyInfo {
        const char *name;
        uint8_t ko;
        uint8_t ki;
    };

    static constexpr int NUM_KEYS = 51;
    // All keys, in row order (the order of the on-screen keypad).
    static const KeyInfo KEYS[NUM_KEYS];
    // Index into keys, or -1. Name matching is case-sensitive ("X" vs "x").
    static int findKey(const char *name);

    // Clears the strobe latches; key states are kept.
    void reset();
    void setKey(int index, bool down);
    void setOnKey(bool down);
    void releaseAll();

    // Gets key presses independent of the strobe lines.
    bool isKeyDown(int index) const { return (KOxKI[KEYS[index].ko] >> KEYS[index].ki) & 1; }
    bool isOnKeyDown() const { return onKey; }

    // --- Internal I/O register hooks ---

    uint8_t readKI() const;

    // The ON key connects KO13 to the CPU's ON input.
    bool onKeyLevel() const { return onKey && (getKOLines() & (1 << 13)); }

    // The ON key input is also connected to CI (the actual connection is a bit more complicated,
    // and has not been fully reverse engineered, but it's enough for here.)
    bool readCI() const { return onKeyLevel(); }

    void writeKOL(uint8_t v) { kol = v; checkInterrupts(); }
    void writeKOH(uint8_t v) { koh = v; checkInterrupts(); }
    void writeKSD(bool disable) { disableStrobe = disable; checkInterrupts(); }

private:
    void checkInterrupts();

    uint16_t getKOLines() const { return disableStrobe ? 0 : uint16_t(kol | (koh << 8)); }

public:
    // CPU reference for interrupt signaling.
    SC62015 *cpu = nullptr;

private:
    // Held keys: for each KO line, the KI bits it connects to.
    uint8_t KOxKI[16]{};

    uint8_t kol = 0, koh = 0;
    bool disableStrobe = false;
    bool onKey = false;
    bool prevOnKey = false;
    uint8_t prevKI = 0;
};
