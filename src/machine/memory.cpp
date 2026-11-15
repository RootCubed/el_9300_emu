#include "machine/memory.hpp"

#include "cpu/sc62015.hpp"

#include <format>
#include <stdexcept>

EL9300Mem::EL9300Mem(const std::vector<uint8_t> &rom) : ROM(rom) {
    if (ROM.size() != ROM_SIZE) {
        throw std::invalid_argument(std::format("ROM image has {} bytes, expected {}", ROM.size(), ROM_SIZE));
    }
}

uint8_t EL9300Mem::read(uint32_t addr) {
    addr &= 0xFFFFF;

    if (addr >= ROM_BASE) {
        return ROM[addr - ROM_BASE];
    } else if (addr >= RAM_BASE && addr <= RAM_END) {
        return ramEnabled ? RAM[addr - RAM_BASE] : 0xFF;
    } else if (addr >= LCD_BASE && addr <= LCD_END) {
        // The LCD area requires an extra cycle to access.
        waitStates++;
        return lcd.read(addr - LCD_BASE);
    } else {
        return 0xFF; // Unmapped
    }
}

void EL9300Mem::write(uint32_t addr, uint8_t data) {
    addr &= 0xFFFFF;

    if (addr >= ROM_BASE) {
        return;
    } else if (addr >= RAM_BASE && addr <= RAM_END) {
        if (ramEnabled) {
            RAM[addr - RAM_BASE] = data;
        }
    } else if (addr >= LCD_BASE && addr <= LCD_END) {
        // The LCD area requires an extra cycle to access.
        waitStates++;
        lcd.write(addr - LCD_BASE, data);
    }
}

uint8_t EL9300Mem::readIO(uint8_t addr, uint8_t latched) {
    switch (addr) {
        case SC62015::IREG_KI: return keyboard.readKI();
        case SC62015::IREG_EIL: return 0x10; // Not implemented apart from bit 4 (0 = backup battery depleted)
        case SC62015::IREG_EIH: return 0; // Not implemented
        case SC62015::IREG_SSR: {
            uint8_t v = 0;
            if (keyboard.readCI()) {
                v |= SC62015::SSR_CI;
            }
            if (keyboard.onKeyLevel()) {
                v |= SC62015::SSR_ONK;
            }
            return v;
        }
        default: return latched;
    }
}

void EL9300Mem::writeIO(uint8_t addr, uint8_t value) {
    switch (addr) {
        case SC62015::IREG_KOL: keyboard.writeKOL(value); break;
        case SC62015::IREG_KOH: keyboard.writeKOH(value); break;
        case SC62015::IREG_EOH: break; // Not implemented
        case SC62015::IREG_EOL: break; // Not implemented
        case SC62015::IREG_LCC: keyboard.writeKSD(value & SC62015::LCC_KSD); break;
        case SC62015::IREG_SCR: {
            uint8_t bz = (value & SC62015::SCR_BZ) >> 4;
            switch (bz) {
                case 0:
                case 4:
                    ramEnabled = false; break;
                case 1:
                case 5:
                    ramEnabled = true; break;
                default: throw std::logic_error(std::format("SCR.BZ value {:02X} is unsupported", bz));
            }
            break;
        }
        default: break;
    }
}
