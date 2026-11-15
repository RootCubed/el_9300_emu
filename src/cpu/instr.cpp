// Instruction decoding.
//
// Encoding: [prebyte] opcode [register/mode byte] [operand bytes...] [displacement]
//  - A prebyte (20h-27h, 30h-37h) selects the internal RAM addressing mode of the
//    first and second (n) operands; without one both use (BP+n).
//  - Register-pair / register-select / [r3] / [(n)] mode bytes directly follow the opcode.
//  - Operand bytes follow in operand order; the [r3±n] / [(n)±m] displacement is last.

#include "cpu/instr.hpp"

#include "cpu/handlers.hpp"
#include "cpu/opcode_table.hpp"
#include "cpu/registers.hpp"

using AM = Instruction::AddressingMode;

namespace {

bool isPrefix(uint8_t b) {
    return (b & 0xF8) == 0x20 || (b & 0xF8) == 0x30;
}

// Prebyte bits 4 and 2 select the first internal operand's mode, bits 1-0 the
// second's. 20h is equivalent to no prebyte.
AM prefixMode1(uint8_t p) {
    switch (p & 0x14) {
        case 0x00: return AM::Internal_BP_n;
        case 0x04: return AM::Internal_BP_Px;
        case 0x10: return AM::Internal_Abs;
        default: return AM::Internal_Px_n;
    }
}

AM prefixMode2(uint8_t p) {
    switch (p & 0x03) {
        case 0: return AM::Internal_BP_n;
        case 1: return AM::Internal_BP_Py;
        case 2: return AM::Internal_Abs;
        default: return AM::Internal_Py_n;
    }
}

// [r3] mode byte: bits 2-0 register; 0x80 = +n (0xC0 = -n), 0x20 = r3++ (0x30 = --r3).
void decodeRegisterMode(Instruction::Operand &o, uint8_t mb) {
    o.reg = RegCode(mb & 7);
    if (mb & 0x80) {
        o.mode = AM::External_Reg_n;
        o.dispNegative = (mb & 0x40) != 0;
    } else if (mb & 0x20) {
        o.mode = (mb & 0x10) ? AM::External_RegDec : AM::External_RegInc;
    } else {
        o.mode = AM::External_Reg;
    }
}

// [(n)] mode byte: 0x00 = [(n)], 0x80 = [(n)+m], 0xC0 = [(n)-m].
void decodeIndirectMode(Instruction::Operand &o, uint8_t mb) {
    if (mb & 0x80) {
        o.mode = AM::External_Ind_m_n;
        o.dispNegative = (mb & 0x40) != 0;
    } else {
        o.mode = AM::External_Ind_n;
    }
}

} // namespace

Instruction::Instruction(uint32_t addr, Bus &bus) : address(addr & 0xFFFFF) {
    auto fetch = [&]() {
        uint8_t v = bus.read((address + bytes.size()) & 0xFFFFF);
        bytes.push_back(v);
        return v;
    };

    uint8_t b = fetch();
    while (isPrefix(b) && bytes.size() < 4) {
        hasPrefix = true;
        modeOp1 = prefixMode1(b);
        modeOp2 = prefixMode2(b);
        b = fetch();
    }
    opcode = b;
    meta = opcodeInfo(opcode);
    if (!meta.handler) {
        // Only reachable through a run of prefix bytes; treat as NOP.
        meta = InstrMeta{"???", Opnd::None, Opnd::None, 0, {1}, handlers::nop};
        return;
    }

    // Pass 1: fixed registers and the register/mode byte that follows the opcode.
    const Opnd kinds[2] = {meta.op1, meta.op2};
    for (int i = 0; i < 2; i++) {
        Operand &o = ops[i];
        o.kind = kinds[i];
        if (uint8_t(kinds[i]) <= uint8_t(Opnd::S)) {
            o.kind = Opnd::Reg;
            o.reg = operandToRegCode(kinds[i]);
            continue;
        }
        switch (kinds[i]) {
            case Opnd::RegHi: {
                // Register pair byte: high nibble = first operand, low nibble = second.
                uint8_t rb = fetch();
                ops[0].kind = ops[1].kind = Opnd::Reg;
                ops[0].reg = RegCode((rb >> 4) & 7);
                ops[1].reg = RegCode(rb & 7);
                i = 2;
                break;
            }
            case Opnd::RegSel:
                o.kind = Opnd::Reg;
                o.reg = RegCode(fetch() & 7);
                break;
            case Opnd::ExtReg:
                decodeRegisterMode(o, fetch());
                break;
            case Opnd::ExtInd:
                decodeIndirectMode(o, fetch());
                break;
            default:
                break;
        }
    }

    // Pass 2: operand bytes in operand order.
    int internalIndex = 0;
    const int w = dataWidth();
    auto fetchLE = [&](int n) {
        uint32_t v = 0;
        for (int k = 0; k < n; k++) v |= fetch() << (8 * k);
        return v;
    };
    for (Operand &o : ops) {
        switch (o.kind) {
            case Opnd::Int:
            case Opnd::ExtInd:
                o.value = fetch();
                o.intMode = (internalIndex++ == 0) ? modeOp1 : modeOp2;
                if (o.kind == Opnd::Int) o.mode = o.intMode;
                break;
            case Opnd::Imm: o.value = fetchLE(w); break;
            case Opnd::Abs:
                o.value = fetchLE(3);
                o.mode = AM::External_Abs;
                break;
            case Opnd::Addr20: o.value = fetchLE(3); break;
            case Opnd::Addr16: o.value = fetchLE(2); break;
            case Opnd::RelP:
            case Opnd::RelM: o.value = fetch(); break;
            default: break;
        }
    }

    // Pass 3: displacement byte (always last).
    for (Operand &o : ops) {
        if (o.mode == AM::External_Reg_n || o.mode == AM::External_Ind_m_n) {
            o.disp = fetch();
        }
    }
}

int Instruction::baseCycles() const {
    // The column order of meta.cycles follows the manual's tables (see InstrMeta).
    for (const Operand &o : ops) {
        switch (o.mode) {
            case AddressingMode::External_RegInc: return meta.cycles[1];
            case AddressingMode::External_RegDec: return meta.cycles[2];
            case AddressingMode::External_Reg_n: return meta.cycles[3];
            case AddressingMode::External_Ind_m_n: return meta.cycles[1];
            default: break; // [r], [(n)] and all other modes use the first value
        }
    }
    return meta.cycles[0];
}

int Instruction::dataWidth() const {
    if (meta.width) return meta.width;
    for (const Operand &o : ops) {
        if (o.kind == Opnd::Reg) return Registers::sizeOf(o.reg);
        if (o.kind == Opnd::B || o.kind == Opnd::F || o.kind == Opnd::IMR) return 1;
    }
    return 0;
}

void Instruction::execute(SC62015 &cpu) {
    meta.handler(*this, cpu);
}
