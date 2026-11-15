#include "cpu/registers.hpp"

#include <stdexcept>

// If the switch doesn't match any case, an invalid value was passed, throw an exception.
[[noreturn]] void invalidCode() {
    throw std::logic_error("invalid register code");
}

uint32_t Registers::get(RegCode code) const {
    switch (code) {
        case RegCode::A: return getA();
        case RegCode::IL: return getIL();
        case RegCode::BA: return getBA();
        case RegCode::I: return getI();
        case RegCode::X: return getX();
        case RegCode::Y: return getY();
        case RegCode::U: return getU();
        case RegCode::S: return getS();
    }
    invalidCode();
}

void Registers::set(RegCode code, uint32_t v) {
    switch (code) {
        case RegCode::A: setA(v & 0xFF); break;
        case RegCode::IL: setIL(v & 0xFF); break;
        case RegCode::BA: setBA(v & 0xFFFF); break;
        case RegCode::I: setI(v & 0xFFFF); break;
        case RegCode::X: setX(v); break;
        case RegCode::Y: setY(v); break;
        case RegCode::U: setU(v); break;
        case RegCode::S: setS(v); break;
    }
}

int Registers::sizeOf(RegCode code) {
    switch (code) {
        case RegCode::A:
        case RegCode::IL:
            return 1;
        case RegCode::BA:
        case RegCode::I:
            return 2;
        case RegCode::X:
        case RegCode::Y:
        case RegCode::U:
        case RegCode::S:
            return 3;
    }
    invalidCode();
}

uint32_t Registers::maskOf(RegCode code) {
    switch (code) {
        case RegCode::A:
        case RegCode::IL:
            return 0xFF;
        case RegCode::BA:
        case RegCode::I:
            return 0xFFFF;
        case RegCode::X:
        case RegCode::Y:
        case RegCode::U:
        case RegCode::S:
            return 0xFFFFF;
    }
    invalidCode();
}

const char *Registers::nameOf(RegCode code) {
    static const char *names[8] = {"A", "IL", "BA", "I", "X", "Y", "U", "S"};
    return names[static_cast<uint8_t>(code)];
}
