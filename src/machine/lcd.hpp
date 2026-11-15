#pragma once

#include <cstdint>

// EL-9300 96x64 dot-matrix LCD, driven by two controller blocks mapped at
// 0x04000 (lower half) and 0x06000 (upper half).
//
// The layout was derived from the ROM's boot screens:
// The two blocks contain 4 128-byte pages each.
// Each page holds a 8*96 section of the screen, one byte per 8-pixel column.
// The first 0x60 bytes of a page are the visible pixels, 0x60-0x7F hold annunciator bits.
// 0x4300 is the contrast register.
class Lcd {
public:
    static constexpr int WIDTH = 96;
    static constexpr int HEIGHT = 64;

    // The actual memory is much less, but it's easier to just allocate the entire range.
    static constexpr size_t MEMORY_SIZE = 0x4000;

    uint8_t read(uint32_t offset) const { return mem[offset & (MEMORY_SIZE - 1)]; }
    void write(uint32_t offset, uint8_t v) { mem[offset & (MEMORY_SIZE - 1)] = v; }

    bool pixel(int x, int y) const;

    // Raw controller memory (for save states).
    uint8_t *raw() { return mem; }
    const uint8_t *raw() const { return mem; }

    struct Annunciator {
        const char *name;
        uint32_t offset;
        uint8_t bit;
    };
    static constexpr Annunciator ANNUNCIATORS[6] = {
        {"2ndF", 0x21E0, 2},
        {"ALPHA", 0x21E1, 3},
        {"BUSY", 0x21E1, 2},
        {"<", 0x21E0, 1},
        {">", 0x21E0, 0},
        {"BATT", 0x21E1, 7}
    };
    bool annunciator(const Annunciator &a) const { return (mem[a.offset] >> a.bit) & 1; }

private:
    uint8_t mem[MEMORY_SIZE]{};
};

inline bool Lcd::pixel(int x, int y) const {
    const int page = 7 - (y / 8);
    const uint32_t block = page < 4 ? 0x0000 : 0x2000;
    const uint32_t offset = block + (page & 3) * 0x80 + (WIDTH - x - 1);
    return (mem[offset] >> (y % 8)) & 1;
}
