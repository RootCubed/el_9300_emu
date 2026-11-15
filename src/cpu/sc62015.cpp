#include "cpu/sc62015.hpp"

#include "cpu/instr.hpp"

SC62015::SC62015(Bus &bus, uint32_t subTimerPeriod) : bus(bus), subTimerPeriod(subTimerPeriod) {
    reset();
}

void SC62015::reset() {
    // See p. 34
    internalRAM[IREG_AMC] = 0;
    internalRAM[IREG_UCR] = 0;
    internalRAM[IREG_USR] &= ~0b00100111;
    internalRAM[IREG_USR] |=  0b00011000;
    internalRAM[IREG_IMR] = 0;
    internalRAM[IREG_SCR] = 0;
    internalRAM[IREG_SSR] &= ~0b00000100;

    runState = RunState::Running;
    extraCycles = 0;
    mainTimerCount = 0;
    subTimerCount = 0;

    pc = bus.read24(VEC_RESET) & 0xFFFFF;
}

// ---------------------------------------------------------------------------
// Memory access
// ---------------------------------------------------------------------------

uint8_t SC62015::readInt(uint8_t addr) {
    if (addr >= 0xF0) {
        return bus.readIO(addr, internalRAM[addr]);
    }
    return internalRAM[addr];
}

void SC62015::writeInt(uint8_t addr, uint8_t v) {
    internalRAM[addr] = v;
    if (addr >= 0xF0) {
        bus.writeIO(addr, v);
    }
}

uint32_t SC62015::readIntN(uint8_t addr, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
        uint8_t ea = addr + i;
        v |= readInt(ea) << (8 * i);
    }
    return v;
}

void SC62015::writeIntN(uint8_t addr, int n, uint32_t v) {
    for (int i = 0; i < n; i++) {
        uint8_t ea = addr + i;
        writeInt(ea, (v >> (8 * i)) & 0xFF);
    }
}

uint32_t SC62015::readExtN(uint32_t addr, int n) {
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
        v |= readExt(addr + i) << (8 * i);
    }
    return v;
}

void SC62015::writeExtN(uint32_t addr, int n, uint32_t v) {
    for (int i = 0; i < n; i++) {
        writeExt(addr + i, (v >> (8 * i)) & 0xFF);
    }
}

void SC62015::pushS(uint32_t v, int n) {
    regs.setS(regs.getS() - n);
    writeExtN(regs.getS(), n, v);
}

uint32_t SC62015::popS(int n) {
    uint32_t v = readExtN(regs.getS(), n);
    regs.setS(regs.getS() + n);
    return v;
}

void SC62015::pushU(uint32_t v, int n) {
    regs.setU(regs.getU() - n);
    writeExtN(regs.getU(), n, v);
}

uint32_t SC62015::popU(int n) {
    uint32_t v = readExtN(regs.getU(), n);
    regs.setU(regs.getU() + n);
    return v;
}

// ---------------------------------------------------------------------------
// Interrupts and timers
// ---------------------------------------------------------------------------

void SC62015::enterInterrupt(uint32_t returnAddr) {
    // Stack layout:
    // - S+0 IMR
    // - S+1 F
    // - S+2..S+4 PC (PCL, PCH, PS).
    pushS(returnAddr & 0xFFFFF, 3);
    pushS(regs.getF(), 1);
    uint8_t imr = internalRAM[IREG_IMR];
    pushS(imr, 1);
    internalRAM[IREG_IMR] = imr & ~INT_IRM;

    pc = bus.read24(VEC_INTERRUPT) & 0xFFFFF;
}

void SC62015::requestInterrupt(uint8_t bits) {
    internalRAM[IREG_ISR] |= bits & INT_ALL;
}

bool SC62015::interruptPending() const {
    uint8_t imr = internalRAM[IREG_IMR];
    return (imr & INT_IRM) != 0 && (internalRAM[IREG_ISR] & imr & INT_ALL);
}

void SC62015::tickTimers(int n) {
    cycles += n;
    // HALT stops the main clock, OFF stops the main clock and the sub-clock (pp. 31-32).
    if (mainTimerPeriod != 0 && runState == RunState::Running) {
        mainTimerCount += n;
        while (mainTimerCount >= mainTimerPeriod) {
            requestInterrupt(INT_MTI);
            mainTimerCount -= mainTimerPeriod;
        }
    }
    if (subTimerPeriod != 0 && runState != RunState::Off) {
        subTimerCount += n;
        while (subTimerCount >= subTimerPeriod) {
            requestInterrupt(INT_STI);
            subTimerCount -= subTimerPeriod;
        }
    }
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

int SC62015::step() {
    if (runState != RunState::Running) {
        // HALT and OFF end on the ON key, a key input or the sub timer (which is stopped in OFF)
        // If SCR.ISE is set, the external interrupt can also wake the CPU even if it is masked.
        // (See pp. 31-32)
        uint8_t wake = INT_ONKI | INT_KEYI | INT_STI;
        if (internalRAM[IREG_SCR] & SCR_ISE) {
            wake |= INT_EXI;
        }
        if (internalRAM[IREG_ISR] & wake) {
            runState = RunState::Running;
        } else {
            // Idle for a few cycles
            tickTimers(10);
            return 10;
        }
    }

    if (interruptPending()) {
        enterInterrupt(pc);
        // Interrupt handling takes 12 cycles (p. 87)
        tickTimers(12);
        return 12;
    }

    // The instruction metadata contains the base cycle count, but there are some additional factors that can add or subtract cycles.
    // We clear these counters to ensure only the extra cycles for this instruction are counted.
    bus.takeWaitStates();
    extraCycles = 0;

    Instruction instr(pc, bus);
    pc = (pc + instr.bytes.size()) & 0xFFFFF;

    instr.execute(*this);

    int cycles = instr.baseCycles() + (instr.hasPrefix ? 1 : 0) + extraCycles + bus.takeWaitStates();
    tickTimers(cycles);
    return cycles;
}
