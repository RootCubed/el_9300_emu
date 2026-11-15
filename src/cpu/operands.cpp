#include "cpu/operands.hpp"

namespace operands {

uint8_t internalAddress(SC62015 &cpu, AM mode, uint8_t n) {
    const uint8_t bp = cpu.peekInt(SC62015::IREG_BP);
    const uint8_t px = cpu.peekInt(SC62015::IREG_PX);
    const uint8_t py = cpu.peekInt(SC62015::IREG_PY);
    switch (mode) {
        case AM::Internal_Abs: return n;
        case AM::Internal_BP_n: return bp + n;
        case AM::Internal_Px_n: return px + n;
        case AM::Internal_Py_n: return py + n;
        case AM::Internal_BP_Px: return bp + px;
        case AM::Internal_BP_Py: return bp + py;
        default: return n;
    }
}

namespace {

// Pointer stored in internal RAM for [(n)] / [(n)±m].
uint32_t indirectPointer(SC62015 &cpu, const Operand &o) {
    uint32_t p = cpu.readIntN(internalAddress(cpu, o.intMode, uint8_t(o.value)), 3);
    if (o.mode == AM::External_Ind_m_n) {
        p = o.dispNegative ? p - o.disp : p + o.disp;
    }
    return p & 0xFFFFF;
}

} // namespace

Loc resolve(const Operand &o, int size, SC62015 &cpu) {
    Registers &r = cpu.getRegisters();
    switch (o.kind) {
        case Opnd::Reg: return {Loc::Reg, static_cast<uint32_t>(o.reg)};
        case Opnd::B: return {Loc::B, 0};
        case Opnd::F: return {Loc::F, 0};
        case Opnd::IMR: return {Loc::Int, SC62015::IREG_IMR};
        case Opnd::Imm: return {Loc::Imm, o.value};
        case Opnd::Int: return {Loc::Int, internalAddress(cpu, o.intMode, uint8_t(o.value))};
        case Opnd::Abs: return {Loc::Ext, o.value & 0xFFFFF};
        case Opnd::ExtInd: return {Loc::Ext, indirectPointer(cpu, o)};
        case Opnd::ExtReg: {
            uint32_t p = r.get(o.reg);
            switch (o.mode) {
                case AM::External_RegInc:
                    r.set(o.reg, p + size);
                    break;
                case AM::External_RegDec:
                    p -= size;
                    r.set(o.reg, p);
                    break;
                case AM::External_Reg_n:
                    p = o.dispNegative ? p - o.disp : p + o.disp;
                    break;
                default:
                    break;
            }
            return {Loc::Ext, p & 0xFFFFF};
        }
        default:
            return {Loc::Imm, 0};
    }
}

uint32_t load(SC62015 &cpu, const Loc &l, int size) {
    switch (l.type) {
        case Loc::Reg: return cpu.getRegisters().get(l.reg());
        case Loc::B: return cpu.getRegisters().getB();
        case Loc::F: return cpu.getRegisters().getF();
        case Loc::Int: return cpu.readIntN(uint8_t(l.addr), size);
        case Loc::Ext: return cpu.readExtN(l.addr, size);
        default: return l.addr & widthMask(size);
    }
}

void store(SC62015 &cpu, const Loc &l, int size, uint32_t v) {
    switch (l.type) {
        case Loc::Reg: cpu.getRegisters().set(l.reg(), v); break;
        case Loc::B: cpu.getRegisters().setB(v & 0xFF); break;
        case Loc::F: cpu.getRegisters().setF(v & 0xFF); break;
        case Loc::Int: cpu.writeIntN(uint8_t(l.addr), size, v); break;
        case Loc::Ext: cpu.writeExtN(l.addr, size, v); break;
        default: break;
    }
}

uint32_t destMask(const Loc &l, int w) {
    if (l.type == Loc::Reg) return Registers::maskOf(l.reg()) & widthMask(w);
    return widthMask(w);
}

uint32_t loopCount(SC62015 &cpu) {
    uint32_t n = cpu.getRegisters().getI();
    cpu.getRegisters().setI(0);
    return n == 0 ? 0x10000 : n;
}

uint8_t Cursor::read(SC62015 &cpu) const {
    return internal ? cpu.readInt(addr & 0xFF) : cpu.readExt(addr & 0xFFFFF);
}

void Cursor::write(SC62015 &cpu, uint8_t v) const {
    if (internal) cpu.writeInt(addr & 0xFF, v);
    else cpu.writeExt(addr & 0xFFFFF, v);
}

Cursor blockCursor(const Operand &o, uint32_t count, int defaultDir, SC62015 &cpu, bool &forceDown) {
    if (o.kind == Opnd::ExtReg) {
        Registers &r = cpu.getRegisters();
        uint32_t p = r.get(o.reg);
        switch (o.mode) {
            case AM::External_RegInc:
                r.set(o.reg, p + count);
                return {false, p, +1};
            case AM::External_RegDec:
                r.set(o.reg, p - count);
                forceDown = true;
                return {false, p - 1, -1};
            case AM::External_Reg_n:
                return {false, o.dispNegative ? p - o.disp : p + o.disp, defaultDir};
            default:
                return {false, p, defaultDir};
        }
    }
    Loc l = resolve(o, 1, cpu);
    return {l.type == Loc::Int, l.addr, defaultDir};
}

} // namespace operands
