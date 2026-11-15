#include "machine/el9300.hpp"

#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

// State file: magic, CPU state, RAM, LCD memory.
constexpr char STATE_MAGIC[4] = {'E', 'L', 'S', 'T'};
const size_t STATE_FILE_SIZE = sizeof(STATE_MAGIC) + SC62015::STATE_SIZE + EL9300Mem::RAM_SIZE + Lcd::MEMORY_SIZE;

EL9300::EL9300(const std::vector<uint8_t> &rom) : memory(rom), cpu(memory, SUB_TIMER_PERIOD) {
    memory.keyboard.cpu = &cpu;
    reset();
}

void EL9300::reset() {
    // Like the RESET pin: only the CPU is reset. The key strobe outputs keep their state
    // (pin list, p. 41), so the keyboard just follows the CPU's registers.
    cpu.reset();
    syncIO();
}

void EL9300::syncIO() {
    for (int i = 0xF0; i <= 0xFF; i++) {
        cpu.writeInt(uint8_t(i), cpu.peekInt(uint8_t(i)));
    }
}

bool EL9300::saveState(const std::string &path) const {
    std::vector<uint8_t> data(STATE_MAGIC, STATE_MAGIC + sizeof(STATE_MAGIC));

    // Save CPU state
    cpu.saveState(data);

    // Save RAM
    data.insert(data.end(), memory.RAM, memory.RAM + sizeof(memory.RAM));

    // Save raw LCD memory
    data.insert(data.end(), memory.lcd.raw(), memory.lcd.raw() + Lcd::MEMORY_SIZE);

    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char *>(data.data()), std::streamsize(data.size()));
    return bool(f);
}

bool EL9300::loadState(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    const std::vector<uint8_t> data(std::istreambuf_iterator<char>(f), {});

    if (data.size() != STATE_FILE_SIZE) {
        return false;
    }
    if (std::memcmp(data.data(), STATE_MAGIC, sizeof(STATE_MAGIC)) != 0) {
        return false;
    }

    const uint8_t *curr = data.data() + sizeof(STATE_MAGIC);

    cpu.loadState(curr);

    std::memcpy(memory.RAM, curr, sizeof(memory.RAM));
    curr += sizeof(memory.RAM);

    std::memcpy(memory.lcd.raw(), curr, Lcd::MEMORY_SIZE);

    // Release all keys and re-sync the peripherals with the restored registers.
    memory.keyboard.reset();
    memory.keyboard.releaseAll();
    syncIO();

    return true;
}
