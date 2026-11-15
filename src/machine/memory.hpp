#pragma once

#include "cpu/bus.hpp"
#include "machine/keyboard.hpp"
#include "machine/lcd.hpp"

#include <cstdint>
#include <vector>

// EL-9300 external memory map:
//   04000-07FFF  LCD (two controller blocks at 04000 and 06000)
//   18000-1FFFF  RAM (32 KB)
//   C0000-FFFFF  ROM (256 KB)
// Everything else is unmapped is implemented to read as 0xFF.
class EL9300Mem : public Bus {
public:
    static constexpr uint32_t LCD_BASE = 0x04000, LCD_END = 0x07FFF;
    static constexpr uint32_t RAM_BASE = 0x18000, RAM_SIZE = 0x8000, RAM_END = RAM_BASE + RAM_SIZE - 1;
    static constexpr uint32_t ROM_BASE = 0xC0000, ROM_SIZE = 0x40000, ROM_END = ROM_BASE + ROM_SIZE - 1;

    explicit EL9300Mem(const std::vector<uint8_t> &rom);

    uint8_t read(uint32_t addr) override;
    void write(uint32_t addr, uint8_t data) override;

    uint8_t readIO(uint8_t addr, uint8_t latched) override;
    void writeIO(uint8_t addr, uint8_t value) override;

    const std::vector<uint8_t> ROM;
    uint8_t RAM[RAM_SIZE]{};
    bool ramEnabled = false;

    Lcd lcd;
    Keyboard keyboard;
};
