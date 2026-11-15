#pragma once

#include "cpu/bus.hpp"
#include "cpu/registers.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class Instruction;
class SC62015;

using InstrHandler = void (*)(Instruction &, SC62015 &);

// Operand kinds as they appear in the opcode table.
enum class Opnd : uint8_t {
    // Fixed registers (values equal the register codes)
    A, IL, BA, I, X, Y, U, S,
    None,
    B,      // high byte of BA
    F,      // flags
    IMR,    // internal RAM FBh (not affected by prebytes)
    Imm,    // immediate, width = instruction data width
    Int,    // (n)    internal RAM, mode selected by prebyte
    Abs,    // [lmn]  external absolute
    ExtReg, // [r3], [r3++], [--r3], [r3+n], [r3-n]   (mode byte)
    ExtInd, // [(n)], [(n)+m], [(n)-m]                (mode byte)
    RegHi,  // register pair byte, high nibble
    RegLo,  // register pair byte, low nibble
    RegSel, // register select byte (0r)
    Addr16, // 16-bit jump target within the current page
    Addr20, // 20-bit jump target
    RelP,   // +n relative
    RelM,   // -n relative

    Reg,    // (decoded only) register with code in Operand::reg
};

inline RegCode operandToRegCode(Opnd opnd) {
    if (uint8_t(opnd) > 7) {
        throw std::invalid_argument("Opnd is not a register");
    }
    return RegCode(uint8_t(opnd));
}

// An opcode table entry.
struct InstrMeta {
    const char *mnemonic = nullptr;
    Opnd op1 = Opnd::None;
    Opnd op2 = Opnd::None;
    // Data width in bytes (1/2/3).
    // 0 = derived from a register operand or not applicable.
    uint8_t width = 0;
    // Execution cycles for (bp+n) internal addressing.
    // Depends on the external addressing mode, if the instruction has such an operand:
    //   ExtReg: [r], [r++], [--r], [r±n]
    //   ExtInd: [(n)], [(n)±m]
    //   others: a single value
    std::array<uint8_t, 4> cycles{};
    InstrHandler handler = nullptr;
};

// A decoded instruction. Decoding has no side effects, so it is also used by
// the disassembler/debugger.
class Instruction {
public:
    enum class AddressingMode : uint8_t {
        // Internal RAM
        Internal_Abs,    // (n)
        Internal_BP_n,   // (BP+n)
        Internal_Px_n,   // (PX+n)
        Internal_Py_n,   // (PY+n)
        Internal_BP_Px,  // (BP+PX)
        Internal_BP_Py,  // (BP+PY)
        // External memory
        External_Abs,    // [lmn]
        External_Reg,    // [r3]
        External_Reg_n,  // [r3+n] / [r3-n]
        External_RegInc, // [r3++]
        External_RegDec, // [--r3]
        External_Ind_n,  // [(n)]
        External_Ind_m_n // [(n)+m] / [(n)-m]
    };

    struct Operand {
        Opnd kind = Opnd::None;
        RegCode reg = RegCode::A; // register code (Reg / ExtReg pointer register)
        AddressingMode mode = AddressingMode::Internal_BP_n;    // overall addressing mode
        AddressingMode intMode = AddressingMode::Internal_BP_n; // internal mode for (n) / pointer of [(n)]
        uint32_t value = 0; // immediate / internal n / absolute address / jump target / rel offset
        uint8_t disp = 0; // displacement for [r3±n] / [(n)±m]
        bool dispNegative = false;
    };

    Instruction(uint32_t addr, Bus &bus);

    void execute(SC62015 &cpu);

    // Returns a disassembly string for this instruction.
    std::string to_string() const;
    // Returns the raw bytes of this instruction in hex.
    std::string bytesHex() const;

    // Effective data width for this instruction (1/2/3 bytes), 0 for loop/none.
    int dataWidth() const;

    // Execution cycles from the opcode table for this instruction's addressing mode
    // (without prebyte, loop counts or wait states).
    int baseCycles() const;

    InstrMeta meta;

    // Internal RAM modes selected by the prebyte for the first/second (n) operand.
    AddressingMode modeOp1 = AddressingMode::Internal_BP_n;
    AddressingMode modeOp2 = AddressingMode::Internal_BP_n;

    uint32_t address;
    uint8_t opcode = 0;
    bool hasPrefix = false;
    Operand ops[2];

    std::vector<uint8_t> bytes;
};
