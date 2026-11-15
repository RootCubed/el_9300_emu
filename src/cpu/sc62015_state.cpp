// SC62015 save-state serialization.
//
// Layout (little-endian): PC (4), BA I X Y U S (20), F (1),
// internal RAM (256), run state (1), cycle counter (8), main/sub timer counters (4 each).

#include "cpu/sc62015.hpp"

#include <algorithm>

namespace {

void put(std::vector<uint8_t> &out, uint64_t v, int n) {
    for (int i = 0; i < n; i++) out.push_back(uint8_t(v >> (8 * i)));
}

uint64_t get(const uint8_t *&p, int n) {
    uint64_t v = 0;
    for (int i = 0; i < n; i++) v |= uint64_t(*p++) << (8 * i);
    return v;
}

} // namespace

const size_t SC62015::STATE_SIZE = 4 + 20 + 1 + sizeof(internalRAM) + 1 + 8 + 4 + 4;

void SC62015::saveState(std::vector<uint8_t> &out) const {
    put(out, pc, 4);
    put(out, regs.getBA(), 2);
    put(out, regs.getI(), 2);
    put(out, regs.getX(), 4);
    put(out, regs.getY(), 4);
    put(out, regs.getU(), 4);
    put(out, regs.getS(), 4);
    put(out, regs.getF(), 1);

    out.insert(out.end(), internalRAM, internalRAM + sizeof(internalRAM));

    put(out, uint8_t(runState), 1);
    put(out, cycles, 8);
    put(out, mainTimerCount, 4);
    put(out, subTimerCount, 4);
}

void SC62015::loadState(const uint8_t *&p) {
    pc = uint32_t(get(p, 4)) & 0xFFFFF;
    regs.setBA(uint16_t(get(p, 2)));
    regs.setI(uint16_t(get(p, 2)));
    regs.setX(uint32_t(get(p, 4)) & 0xFFFFF);
    regs.setY(uint32_t(get(p, 4)) & 0xFFFFF);
    regs.setU(uint32_t(get(p, 4)) & 0xFFFFF);
    regs.setS(uint32_t(get(p, 4)) & 0xFFFFF);
    regs.setF(uint8_t(get(p, 1)));

    std::copy(p, p + sizeof(internalRAM), internalRAM);
    p += sizeof(internalRAM);

    runState = RunState(get(p, 1));
    if (runState != RunState::Running && runState != RunState::Halted && runState != RunState::Off) {
        runState = RunState::Running;
    }

    cycles = get(p, 8);
    mainTimerCount = uint32_t(get(p, 4));
    subTimerCount = uint32_t(get(p, 4));

    extraCycles = 0;
}
