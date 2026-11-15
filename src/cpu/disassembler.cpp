// Instruction disassembly (text form of a decoded Instruction).
#include "cpu/instr.hpp"
#include "cpu/registers.hpp"

#include <algorithm>
#include <format>
#include <string>

namespace {

// Name for an absolute internal RAM address, including memory-mapped registers.
std::string internalName(uint8_t n) {
    static const char *names[0x100 - 0xEC] = {
        "BP", "PX", "PY", "AMC",
        "KOL", "KOH", "KI", "EOL", "EOH", "EIL", "EIH", "UCR",
        "USR", "RXD", "TXD", "IMR", "ISR", "SCR", "LCC", "SSR",
    };
    if (n >= 0xEC) {
        return names[n - 0xEC];
    }
    return std::format("{:02X}", n);
}

std::string formatInternal(Instruction::AddressingMode mode, uint8_t n) {
    switch (mode) {
        case Instruction::AddressingMode::Internal_Abs: return "(" + internalName(n) + ")";
        case Instruction::AddressingMode::Internal_BP_n: return std::format("(BP+{:02X})", n);
        case Instruction::AddressingMode::Internal_Px_n: return std::format("(PX+{:02X})", n);
        case Instruction::AddressingMode::Internal_Py_n: return std::format("(PY+{:02X})", n);
        case Instruction::AddressingMode::Internal_BP_Px: return "(BP+PX)";
        case Instruction::AddressingMode::Internal_BP_Py: return "(BP+PY)";
        default: return "(?)";
    }
}

std::string formatDisp(const Instruction::Operand &o) {
    return std::format("{}{:02X}", o.dispNegative ? '-' : '+', o.disp);
}

std::string formatOperand(const Instruction &in, const Instruction::Operand &o) {
    const uint32_t next = (in.address + in.bytes.size()) & 0xFFFFF;
    const uint32_t page = next & 0xF0000;
    const int w = in.dataWidth();

    switch (o.kind) {
        case Opnd::Reg: return Registers::nameOf(o.reg);
        case Opnd::B: return "B";
        case Opnd::F: return "F";
        case Opnd::IMR: return "IMR";
        case Opnd::Imm: {
            if (w == 3) {
                return std::format("{:05X}", o.value);
            } else {
                return std::format("{:0{}X}", o.value, w * 2);
            }
        }
        case Opnd::Int: return formatInternal(o.intMode, uint8_t(o.value));
        case Opnd::Abs: return std::format("[{:05X}]", o.value & 0xFFFFF);
        case Opnd::ExtReg: {
            const char *r = Registers::nameOf(o.reg);
            switch (o.mode) {
                case Instruction::AddressingMode::External_RegInc: return std::format("[{}++]", r);
                case Instruction::AddressingMode::External_RegDec: return std::format("[--{}]", r);
                case Instruction::AddressingMode::External_Reg_n: return std::format("[{}{}]", r, formatDisp(o));
                default: return std::format("[{}]", r);
            }
        }
        case Opnd::ExtInd: {
            std::string s = "[" + formatInternal(o.intMode, uint8_t(o.value));
            if (o.mode == Instruction::AddressingMode::External_Ind_m_n) {
                s += formatDisp(o);
            }
            return s + "]";
        }
        case Opnd::Addr16: return std::format("{:05X}", page | (o.value & 0xFFFF));
        case Opnd::Addr20: return std::format("{:05X}", o.value & 0xFFFFF);
        case Opnd::RelP: return std::format("{:05X}", page | ((next + o.value) & 0xFFFF));
        case Opnd::RelM: return std::format("{:05X}", page | ((next - o.value) & 0xFFFF));
        default: return "?";
    }
}

} // namespace

std::string Instruction::to_string() const {
    if (bytes.empty() || !meta.mnemonic) {
        return "Invalid instruction";
    }

    std::string args;
    for (const Operand &o : ops) {
        if (o.kind == Opnd::None) continue;
        if (!args.empty()) args += ", ";
        args += formatOperand(*this, o);
    }

    std::string result = meta.mnemonic;
    if (!args.empty()) {
        result.resize(std::max<size_t>(result.size() + 1, 6), ' ');
        result += args;
    }
    return result;
}

std::string Instruction::bytesHex() const {
    std::string s;
    for (uint8_t b : bytes) {
        if (!s.empty()) {
            s += ' ';
        }
        s += std::format("{:02X}", b);
    }
    return s;
}
