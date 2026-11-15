#pragma once

// Operand resolution shared by the instruction handlers.

#include "cpu/instr.hpp"
#include "cpu/sc62015.hpp"

#include <cstdint>

namespace operands {

using AM = Instruction::AddressingMode;
using Operand = Instruction::Operand;

// A resolved operand location.
struct Loc {
    enum Type : uint8_t { Reg, Int, Ext, Imm, B, F } type;
    uint32_t addr; // register code / internal address / external address / immediate value

    RegCode reg() const { return RegCode(addr & 7); }
};

// Mask covering w bytes.
inline uint32_t widthMask(int w) {
    return w >= 4 ? 0xFFFFFFFF : (1u << (8 * w)) - 1;
}

// Effective internal RAM address for (n) in the given addressing mode.
uint8_t internalAddress(SC62015 &cpu, AM mode, uint8_t n);

// Resolves an operand to a location. Applies pointer side effects
// ([r3++] / [--r3]) using size as the step.
Loc resolve(const Operand &o, int size, SC62015 &cpu);

uint32_t load(SC62015 &cpu, const Loc &l, int size);
void store(SC62015 &cpu, const Loc &l, int size, uint32_t v);

// Value mask of an arithmetic destination: 20 bits for X/Y/U/S, otherwise the data width.
uint32_t destMask(const Loc &l, int w);

// Number of iterations for loop instructions (I = 0 means 65536). Clears I.
uint32_t loopCount(SC62015 &cpu);

// A walking pointer for block (loop) instructions.
struct Cursor {
    bool internal;
    uint32_t addr;
    int dir;

    uint8_t read(SC62015 &cpu) const;
    void write(SC62015 &cpu, uint8_t v) const;
    void advance() { addr += dir; }
};

// Builds a cursor for a block-operation operand covering count bytes.
// [r3++] / [--r3] update the register by count; [--r3] walks downwards and
// sets forceDown so the other operand follows.
Cursor blockCursor(const Operand &o, uint32_t count, int defaultDir, SC62015 &cpu, bool &forceDown);

} // namespace operands
