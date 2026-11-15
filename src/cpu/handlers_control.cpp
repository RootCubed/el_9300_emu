// Jumps, calls, returns, interrupts and system instructions.

#include "cpu/operands.hpp"

using namespace operands;

namespace handlers {

namespace {

// Condition codes in bits 1-0 (JPcc) or 2-1 (JRcc): Z, NZ, C, NC.
bool condition(uint8_t cc, const Registers &r) {
    switch (cc & 3) {
        case 0: return r.isZ();
        case 1: return !r.isZ();
        case 2: return r.isC();
        default: return !r.isC();
    }
}

uint32_t jumpTarget(const Instruction &in, SC62015 &cpu) {
    const Operand &o = in.ops[0];
    switch (o.kind) {
        case Opnd::Addr16: return (cpu.getPC() & 0xF0000) | (o.value & 0xFFFF);
        case Opnd::Addr20: return o.value & 0xFFFFF;
        case Opnd::Int: return cpu.readIntN(internalAddress(cpu, o.intMode, uint8_t(o.value)), 3) & 0xFFFFF;
        case Opnd::Reg: return cpu.getRegisters().get(o.reg) & 0xFFFFF;
        default: return cpu.getPC();
    }
}

void relativeJump(const Instruction &in, SC62015 &cpu) {
    uint32_t pc = cpu.getPC();
    int off = in.ops[0].kind == Opnd::RelM ? -int(in.ops[0].value) : int(in.ops[0].value);
    // PC is 16 bits; relative jumps stay within the current page.
    cpu.setPC((pc & 0xF0000) | ((pc + off) & 0xFFFF));
}

} // namespace

void nop(Instruction &, SC62015 &) {}

void jp(Instruction &in, SC62015 &cpu) {
    cpu.setPC(jumpTarget(in, cpu));
}

void jpcc(Instruction &in, SC62015 &cpu) {
    if (condition(in.opcode & 3, cpu.getRegisters())) {
        cpu.setPC(jumpTarget(in, cpu));
    } else {
        cpu.addCycles(-1); // not taken
    }
}

void jr(Instruction &in, SC62015 &cpu) {
    relativeJump(in, cpu);
}

void jrcc(Instruction &in, SC62015 &cpu) {
    if (condition(in.opcode >> 1, cpu.getRegisters())) {
        relativeJump(in, cpu);
    } else {
        cpu.addCycles(-1); // not taken
    }
}

void call(Instruction &in, SC62015 &cpu) {
    uint32_t ret = cpu.getPC();
    if (in.ops[0].kind == Opnd::Addr16) {
        cpu.pushS(ret & 0xFFFF, 2); // CALL: PC only, page unchanged
    } else {
        cpu.pushS(ret, 3); // CALLF: PS:PC
    }
    cpu.setPC(jumpTarget(in, cpu));
}

void ret(Instruction &, SC62015 &cpu) {
    uint32_t page = cpu.getPC() & 0xF0000;
    cpu.setPC(page | cpu.popS(2));
}

void retf(Instruction &, SC62015 &cpu) {
    cpu.setPC(cpu.popS(3));
}

void reti(Instruction &, SC62015 &cpu) {
    uint8_t imr = uint8_t(cpu.popS(1));
    uint8_t f = uint8_t(cpu.popS(1));
    cpu.writeInt(SC62015::IREG_IMR, imr);
    cpu.getRegisters().setF(f);
    cpu.setPC(cpu.popS(3));
}

void halt(Instruction &, SC62015 &cpu) { cpu.halt(); }
void off(Instruction &, SC62015 &cpu) { cpu.powerOff(); }
void tcl(Instruction &, SC62015 &cpu) { cpu.clearTimers(); }

void wait(Instruction &, SC62015 &cpu) {
    Registers &r = cpu.getRegisters();
    cpu.addCycles(r.getI());
    r.setI(0);
}

void ir(Instruction &in, SC62015 &cpu) {
    // The pushed return address points at the IR opcode itself; the ROM's
    // interrupt handler checks for FEh there and skips over it.
    cpu.enterInterrupt(in.address);
}

void reset(Instruction &, SC62015 &cpu) {
    cpu.setPC(cpu.getBus().read24(SC62015::VEC_RESET));
}

} // namespace handlers
