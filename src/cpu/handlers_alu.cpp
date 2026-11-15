// Arithmetic, logic, BCD and shift/rotate instructions.

#include "cpu/handlers.hpp"
#include "cpu/operands.hpp"

using namespace operands;

namespace handlers {

namespace {

// INC / DEC: Z only, C unchanged.
void incdec(int delta, Instruction &in, SC62015 &cpu) {
    int w = in.dataWidth();
    Loc l = resolve(in.ops[0], w, cpu);
    uint32_t v = (load(cpu, l, w) + delta) & destMask(l, w);
    cpu.getRegisters().setZ(v == 0);
    store(cpu, l, w, v);
}

// One BCD byte: a + b + c, or a - b - c when subtracting; `c` is the carry / borrow.
uint8_t bcdByte(uint8_t a, uint8_t b, bool &c, bool subtract) {
    const int s = subtract ? -1 : 1;
    auto overflows = [&](int digit) { return subtract ? digit < 0 : digit > 9; };
    int lo = (a & 0x0F) + s * ((b & 0x0F) + (c ? 1 : 0));
    int hi = (a >> 4) + s * (b >> 4);
    if (overflows(lo)) {
        lo -= s * 10;
        hi += s;
    }
    c = overflows(hi);
    if (c) hi -= s * 10;
    return uint8_t(((hi & 0x0F) << 4) | (lo & 0x0F));
}

// DSLL / DSRL: shift a big-endian BCD number by one digit. DSLL (left) starts at
// its last (lowest) byte and walks down; DSRL (right) starts at the first byte.
void digitShift(bool left, Instruction &in, SC62015 &cpu) {
    uint32_t count = loopCount(cpu);
    uint8_t a = uint8_t(resolve(in.ops[0], 1, cpu).addr);
    uint8_t carryNibble = 0;
    bool zero = true;
    for (uint32_t i = 0; i < count; i++, a += left ? -1 : 1) {
        uint8_t v = cpu.readInt(a);
        uint8_t nv = left ? uint8_t((v << 4) | carryNibble) : uint8_t((carryNibble << 4) | (v >> 4));
        carryNibble = left ? v >> 4 : v & 0x0F;
        cpu.writeInt(a, nv);
        if (nv) zero = false;
    }
    cpu.getRegisters().setZ(zero);
    cpu.addCycles(int(count));
}

} // namespace

// Two-operand ALU operation on 1/2/3-byte data.
//   ADD/ADC/SUB/SBC/CMP set Z and C; AND/OR/XOR/TEST set Z only; PMDF sets nothing.
//   CMP and TEST don't store the result.
void aluOp(Alu op, Instruction &in, SC62015 &cpu) {
    Registers &r = cpu.getRegisters();
    int w = in.dataWidth();
    Loc dst = resolve(in.ops[0], w, cpu);
    Loc src = resolve(in.ops[1], w, cpu);
    uint32_t mask = destMask(dst, w);
    uint32_t a = load(cpu, dst, w) & mask;
    uint32_t b = load(cpu, src, w) & mask;
    uint32_t carryIn = r.isC() ? 1 : 0;
    uint64_t res = 0;
    bool carry = false;

    switch (op) {
        case Alu::Add:
        case Alu::Pmdf:
            res = uint64_t(a) + b;
            carry = res > mask;
            break;
        case Alu::Adc:
            res = uint64_t(a) + b + carryIn;
            carry = res > mask;
            break;
        case Alu::Sub:
        case Alu::Cmp:
            res = uint64_t(a) - b;
            carry = a < b;
            break;
        case Alu::Sbc:
            res = uint64_t(a) - b - carryIn;
            carry = uint64_t(a) < uint64_t(b) + carryIn;
            break;
        case Alu::And:
        case Alu::Test: res = a & b; break;
        case Alu::Or: res = a | b; break;
        case Alu::Xor: res = a ^ b; break;
    }
    uint32_t v = uint32_t(res) & mask;

    const bool setsCarry = op == Alu::Add || op == Alu::Adc || op == Alu::Sub || op == Alu::Sbc || op == Alu::Cmp;
    if (op != Alu::Pmdf) r.setZ(v == 0);
    if (setsCarry) r.setC(carry);
    if (op != Alu::Cmp && op != Alu::Test) store(cpu, dst, w, v);
}

// ADCL / SBCL (ascending, binary) and DADL / DSBL (descending, BCD) over I bytes.
// None of them take the incoming C flag: the carry starts at 0 and is only
// propagated between bytes. (The ROM relies on this, e.g. "MV A,01 / ADCL (SI),A"
// to step a 3-byte font pointer right after code that leaves C=1.)
// For the "(n),A" forms, A is added to the first byte only.
void loopOp(Loop op, Instruction &in, SC62015 &cpu) {
    const bool subtract = op == Loop::Sbcl || op == Loop::Dsbl;
    const bool bcd = op == Loop::Dadl || op == Loop::Dsbl;
    Registers &r = cpu.getRegisters();
    uint32_t count = loopCount(cpu);
    int dir = bcd ? -1 : +1;
    bool srcIsA = in.ops[1].kind == Opnd::Reg;
    uint8_t d = uint8_t(resolve(in.ops[0], 1, cpu).addr);
    uint8_t s = srcIsA ? 0 : uint8_t(resolve(in.ops[1], 1, cpu).addr);

    bool carry = false;
    bool zero = true;
    for (uint32_t i = 0; i < count; i++, d += dir, s += dir) {
        uint8_t a = cpu.readInt(d);
        uint8_t b = srcIsA ? (i == 0 ? r.getA() : 0) : cpu.readInt(s);
        uint8_t v;
        if (bcd) {
            v = bcdByte(a, b, carry, subtract);
        } else {
            int t = subtract ? int(a) - b - (carry ? 1 : 0) : int(a) + b + (carry ? 1 : 0);
            carry = subtract ? t < 0 : t > 0xFF;
            v = uint8_t(t);
        }
        cpu.writeInt(d, v);
        if (v) zero = false;
    }
    r.setZ(zero);
    r.setC(carry);
    cpu.addCycles(int(srcIsA ? count : 2 * count));
}

// ROR/ROL rotate within the byte; SHR/SHL shift through the carry. All set C and Z.
void rotateOp(Rot op, Instruction &in, SC62015 &cpu) {
    Registers &r = cpu.getRegisters();
    Loc l = resolve(in.ops[0], 1, cpu);
    uint8_t v = uint8_t(load(cpu, l, 1));
    uint8_t res = 0;
    bool c = false;
    switch (op) {
        case Rot::Ror: c = v & 1; res = uint8_t((v >> 1) | (v << 7)); break;
        case Rot::Rol: c = v & 0x80; res = uint8_t((v << 1) | (v >> 7)); break;
        case Rot::Shr: c = v & 1; res = uint8_t((v >> 1) | (r.isC() ? 0x80 : 0)); break;
        case Rot::Shl: c = v & 0x80; res = uint8_t((v << 1) | (r.isC() ? 1 : 0)); break;
    }
    r.setC(c);
    r.setZ(res == 0);
    store(cpu, l, 1, res);
}

void inc(Instruction &in, SC62015 &cpu) { incdec(+1, in, cpu); }
void dec(Instruction &in, SC62015 &cpu) { incdec(-1, in, cpu); }
void dsll(Instruction &in, SC62015 &cpu) { digitShift(true, in, cpu); }
void dsrl(Instruction &in, SC62015 &cpu) { digitShift(false, in, cpu); }

void swap(Instruction &, SC62015 &cpu) {
    Registers &r = cpu.getRegisters();
    uint8_t a = r.getA();
    a = uint8_t((a << 4) | (a >> 4));
    r.setA(a);
    r.setZ(a == 0);
}

void sc(Instruction &, SC62015 &cpu) { cpu.getRegisters().setC(true); }
void rc(Instruction &, SC62015 &cpu) { cpu.getRegisters().setC(false); }

} // namespace handlers
