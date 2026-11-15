#pragma once

#include <cstdint>

// Register numbers as used in SC62015 instruction encodings.
enum class RegCode : uint8_t {
    A, IL, BA, I, X, Y, U, S
};

// The SC62015 register file.
// BA = B + A, I = IH + IL.
// X, Y, U, S are 20-bit registers.
// F is just the Z and C flags, but can be stored as a byte.
class Registers {
public:
    uint8_t getA() const { return BA & 0xFF; }
    void setA(uint8_t v) { BA = (BA & 0xFF00) | v; }

    uint8_t getB() const { return BA >> 8; }
    void setB(uint8_t v) { BA = (BA & 0x00FF) | (v << 8); }

    uint16_t getBA() const { return BA; }
    void setBA(uint16_t v) { BA = v; }

    uint8_t getIL() const { return I & 0xFF; }
    // Writing IL always clears IH.
    void setIL(uint8_t v) { I = v; }

    uint16_t getI() const { return I; }
    void setI(uint16_t v) { I = v; }

    uint32_t getX() const { return X; }
    void setX(uint32_t v) { X = v & 0xFFFFF; }

    uint32_t getY() const { return Y; }
    void setY(uint32_t v) { Y = v & 0xFFFFF; }

    uint32_t getU() const { return U; }
    void setU(uint32_t v) { U = v & 0xFFFFF; }

    uint32_t getS() const { return S; }
    void setS(uint32_t v) { S = v & 0xFFFFF; }

    // F as stored on the stack: bit 1 = Z, bit 0 = C.
    uint8_t getF() const { return (Z ? 0x02 : 0) | (C ? 0x01 : 0); }
    void setF(uint8_t v) { setZ(v & 0x02); setC(v & 0x01); }

    bool isZ() const { return Z; }
    void setZ(bool v) { Z = v; }

    bool isC() const { return C; }
    void setC(bool v) { C = v; }

    // Accesses a register by encoded register code.
    uint32_t get(RegCode code) const;
    void set(RegCode code, uint32_t v);

    // Returns the size of a register in bytes when stored to memory (1, 2 or 3).
    static int sizeOf(RegCode code);

    // Returns the value mask of a register (8, 16 or 20 bits).
    static uint32_t maskOf(RegCode code);

    // Returns the name of a register code.
    static const char *nameOf(RegCode code);

private:
    uint16_t BA = 0;
    uint16_t I = 0;
    uint32_t X = 0;
    uint32_t Y = 0;
    uint32_t U = 0;
    uint32_t S = 0;
    bool Z = false;
    bool C = false;
};
