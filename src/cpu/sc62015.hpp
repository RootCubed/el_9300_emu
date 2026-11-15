#pragma once

#include "cpu/bus.hpp"
#include "cpu/registers.hpp"

#include <cstdint>
#include <vector>

// Sharp SC62015 (ESR-L) CPU core.
// See https://sharppocketcomputers.com/4HK7JnFJDuVm/Tech/ESR-L_CPU_tech_manual.pdf
class SC62015 {
public:
    // Internal RAM addresses with special meaning.
    enum IntRamRegisters : uint8_t {
        IREG_BP = 0xEC, // Internal RAM base pointer
        IREG_PX = 0xED, IREG_PY = 0xEE, // Internal RAM index registers
        IREG_AMC = 0xEF, // Address Modify Control (unused in EL-9300)
        IREG_KOL = 0xF0, IREG_KOH = 0xF1, // Key Output Buffer
        IREG_KI = 0xF2, // Key Input Buffer
        IREG_EOL = 0xF3, IREG_EOH = 0xF4, // E Port Output Buffer
        IREG_EIL = 0xF5, IREG_EIH = 0xF6, // E Port Input Buffer
        IREG_UCR = 0xF7, // UART Control Register (unused in EL-9300)
        IREG_USR = 0xF8, // UART Status Register (unused in EL-9300)
        IREG_RXD = 0xF9, IREG_TXD = 0xFA, // UART Receive / Transmit Buffer (unused in EL-9300)
        IREG_IMR = 0xFB, // Interrupt Mask Register
        IREG_ISR = 0xFC, // Interrupt Status Register
        IREG_SCR = 0xFD, // System Control Register
        IREG_LCC = 0xFE, // LCD Contrast Control (+ key strobe disable and timer reset)
        IREG_SSR = 0xFF, // System Status Register
    };

    // Interrupt bits (shared by IMR and ISR).
    enum InterruptBits : uint8_t {
        INT_MTI = 0x01,  // MSEC Timer Interrupt
        INT_STI = 0x02,  // SEC Timer Interrupt
        INT_KEYI = 0x04, // Key Interrupt
        INT_ONKI = 0x08, // ON Key Interrupt
        INT_TXRI = 0x10, // Transmitter Ready Interrupt
        INT_RXRI = 0x20, // Receiver Ready Interrupt
        INT_EXI = 0x40,  // External Interrupt
        INT_IRM = 0x80,  // Interrupt Mask (0 disables all interrupts)
        INT_ALL = 0x7F,  // (shorthand for all ISR bits)
    };

    // System Control Register (SCR) bits.
    enum SCRBits : uint8_t {
        SCR_DISC = 0x01, // LCD Driver Control (DIS pin, 1 = display on)
        SCR_MTS = 0x02,  // MSEC Timer Select (0 = short period, 1 = long period)
        SCR_STS = 0x04,  // SEC Timer Select (0 = long period, 1 = short period)
        SCR_VDDC = 0x08, // VDD Control (1 = high/GND; also stops the sub-clock)
        SCR_BZ0 = 0x10,  // CO/CI Control Factors
        SCR_BZ1 = 0x20,
        SCR_BZ2 = 0x40,
        SCR_BZ = 0x70,
        SCR_ISE = 0x80,  // IRQ Start Enable (IRQ may release HALT/OFF)
    };

    // System Status Register (SSR) bits (read only).
    enum SSRBits : uint8_t {
        SSR_TEST = 0x01, // TEST pin level
        SSR_CI = 0x02,   // CI (CMT input) pin level, only while SCR_BZ2 is set
        SSR_RSF = 0x04,  // Reset Start Flag (0 after RESET, 1 after HALT/OFF)
        SSR_ONK = 0x08,  // ON pin level
    };

    // LCC (LCD contrast control) bits. Bits 7-3 are the LCD contrast,
    // though on the EL-9300 LCD contrast is handled by the LCD controller.
    enum LCCBits : uint8_t {
        LCC_MTCL = 0x01, // TCL resets the main timer
        LCC_STCL = 0x02, // TCL resets the sub timer
        LCC_KSD = 0x04,  // key strobe disable: all KO pins low
    };

    enum class RunState : uint8_t { Running, Halted, Off };

    static constexpr uint32_t VEC_INTERRUPT = 0xFFFFA;
    static constexpr uint32_t VEC_RESET = 0xFFFFD;

    explicit SC62015(Bus &bus, uint32_t subTimerPeriod);

    void reset();

    // Executes one instruction (or services an interrupt / idles while halted).
    // Returns the number of CPU cycles consumed.
    int step();

    uint32_t getPC() const { return pc; }
    void setPC(uint32_t v) { pc = v & 0xFFFFF; }
    const Registers &getRegisters() const { return regs; }
    Registers &getRegisters() { return regs; }
    Bus &getBus() { return bus; }
    RunState getRunState() const { return runState; }
    uint64_t getCycles() const { return cycles; }

    // Raise interrupt request bits in ISR (from peripherals).
    void requestInterrupt(uint8_t bits);

    // One CPU cycle (called a "state" in the ESR-L manual) takes 3 periods of the main clock.
    // All references to "cycle counts" in this emulator are actually CPU state counts.
    static constexpr uint32_t CLOCK_PERIODS_PER_CYCLE = 3;

    // Timer periods in CPU cycles (0 disables the timer).
    // Main timer: divides the main clock by 2^15.
    // Sub timer: driven by a separate clock, defined by the board.
    uint32_t mainTimerPeriod = (1 << 15) / CLOCK_PERIODS_PER_CYCLE;
    uint32_t subTimerPeriod;

    // --- Memory access (used by instruction handlers) ---

    // Internal RAM (F0h-FFh are I/O registers and go through the bus' I/O hooks)
    uint8_t readInt(uint8_t addr);
    void writeInt(uint8_t addr, uint8_t v);

    // Multi-byte read/write
    uint32_t readIntN(uint8_t addr, int n);
    void writeIntN(uint8_t addr, int n, uint32_t v);

    // Raw internal RAM access without I/O side effects
    uint8_t peekInt(uint8_t addr) const { return internalRAM[addr]; }

    // External memory access
    uint8_t readExt(uint32_t addr) { return bus.read(addr & 0xFFFFF); }
    void writeExt(uint32_t addr, uint8_t v) { bus.write(addr & 0xFFFFF, v); }
    uint32_t readExtN(uint32_t addr, int n);
    void writeExtN(uint32_t addr, int n, uint32_t v);

    // System (S) and user (U) stacks; values are stored little-endian.
    void pushS(uint32_t v, int n);
    uint32_t popS(int n);
    void pushU(uint32_t v, int n);
    uint32_t popU(int n);

    // Push PC/F/IMR, disable interrupts and jump through the interrupt vector.
    void enterInterrupt(uint32_t returnAddr);

    void halt() { runState = RunState::Halted; }
    void powerOff() { runState = RunState::Off; }

    // TCL: resets the timers selected by LCC bits MTCL / STCL.
    void clearTimers() {
        const uint8_t lcc = readInt(IREG_LCC);
        if (lcc & LCC_MTCL) mainTimerCount = 0;
        if (lcc & LCC_STCL) subTimerCount = 0;
    }

    // Extra cycles taken by the instruction being executed (may be negative).
    void addCycles(int n) { extraCycles += n; }

    // Save states (see sc62015_state.cpp)
    static const size_t STATE_SIZE;
    void saveState(std::vector<uint8_t> &out) const;
    // Reads STATE_SIZE bytes from p and advances it.
    void loadState(const uint8_t *&p);

private:
    // Advances the timers by n cycles.
    void tickTimers(int n);
    bool interruptPending() const;

    Bus &bus;
    Registers regs;
    uint32_t pc = 0;
    uint8_t internalRAM[0x100]{};

    RunState runState = RunState::Running;
    uint64_t cycles = 0;
    int extraCycles = 0;
    uint32_t mainTimerCount = 0;
    uint32_t subTimerCount = 0;
};
