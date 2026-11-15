// Data movement: MV/MVW/MVP, EX*, block moves, stacks.

#include "cpu/operands.hpp"

using namespace operands;

namespace handlers {

void mv(Instruction &in, SC62015 &cpu) {
    int w = in.dataWidth();
    Loc src = resolve(in.ops[1], w, cpu);
    Loc dst = resolve(in.ops[0], w, cpu);
    store(cpu, dst, w, load(cpu, src, w));
}

void ex(Instruction &in, SC62015 &cpu) {
    int w = in.dataWidth();
    Loc a = resolve(in.ops[0], w, cpu);
    Loc b = resolve(in.ops[1], w, cpu);
    uint32_t va = load(cpu, a, w);
    uint32_t vb = load(cpu, b, w);
    store(cpu, a, w, vb);
    store(cpu, b, w, va);
}

namespace {

// Copies I bytes between two operands (internal or external). [--r3] makes both
// sides walk downwards; MVLD walks downwards by default.
void blockMove(Instruction &in, SC62015 &cpu, int defaultDir) {
    uint32_t count = loopCount(cpu);
    bool down = defaultDir < 0;
    Cursor src = blockCursor(in.ops[1], count, defaultDir, cpu, down);
    Cursor dst = blockCursor(in.ops[0], count, defaultDir, cpu, down);
    if (down) {
        auto isPreDec = [](const Operand &o) { return o.kind == Opnd::ExtReg && o.mode == AM::External_RegDec; };
        if (!isPreDec(in.ops[1])) src.dir = -1;
        if (!isPreDec(in.ops[0])) dst.dir = -1;
    }
    for (uint32_t i = 0; i < count; i++) {
        dst.write(cpu, src.read(cpu));
        src.advance();
        dst.advance();
    }
    cpu.addCycles(int(2 * count));
}

} // namespace

void mvl(Instruction &in, SC62015 &cpu) { blockMove(in, cpu, +1); }
void mvld(Instruction &in, SC62015 &cpu) { blockMove(in, cpu, -1); }

void exl(Instruction &in, SC62015 &cpu) {
    uint32_t count = loopCount(cpu);
    uint8_t a = uint8_t(resolve(in.ops[0], 1, cpu).addr);
    uint8_t b = uint8_t(resolve(in.ops[1], 1, cpu).addr);
    for (uint32_t i = 0; i < count; i++, a++, b++) {
        uint8_t va = cpu.readInt(a), vb = cpu.readInt(b);
        cpu.writeInt(a, vb);
        cpu.writeInt(b, va);
    }
    cpu.addCycles(int(3 * count));
}

void pushu(Instruction &in, SC62015 &cpu) {
    int w = in.dataWidth();
    Loc l = resolve(in.ops[0], w, cpu);
    cpu.pushU(load(cpu, l, w), w);
    if (in.ops[0].kind == Opnd::IMR) {
        // Saving IMR also disables interrupts (clears IRM).
        cpu.writeInt(SC62015::IREG_IMR, cpu.peekInt(SC62015::IREG_IMR) & ~SC62015::INT_IRM);
    }
}

void popu(Instruction &in, SC62015 &cpu) {
    int w = in.dataWidth();
    Loc l = resolve(in.ops[0], w, cpu);
    store(cpu, l, w, cpu.popU(w));
}

void pushs(Instruction &, SC62015 &cpu) {
    cpu.pushS(cpu.getRegisters().getF(), 1);
}

void pops(Instruction &, SC62015 &cpu) {
    cpu.getRegisters().setF(uint8_t(cpu.popS(1)));
}

} // namespace handlers
