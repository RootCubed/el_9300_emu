#include "gui/rom_dialog.hpp"

#include "machine/memory.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

std::vector<uint8_t> loadRom(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open ROM file: " + path);
    }

    std::vector<uint8_t> rom(std::istreambuf_iterator<char>(f), {});
    if (rom.size() != EL9300Mem::ROM_SIZE) {
        throw std::runtime_error(
            "Wrong ROM size: " + path + " has " + std::to_string(rom.size()) +
            " bytes, expected " + std::to_string(EL9300Mem::ROM_SIZE));
    }

    return rom;
}

uint32_t romDialogEventType() {
    static const uint32_t type = SDL_RegisterEvents(1);
    return type;
}

void SDLCALL dialogCallback(void *, const char *const *files, int) {
    SDL_Event event{};
    event.type = romDialogEventType();
    if (files && files[0]) {
        event.user.data1 = SDL_strdup(files[0]);
    } else {
        event.user.data1 = nullptr;
    }
    SDL_PushEvent(&event);
}

void openRomDialog(SDL_Window *window) {
    romDialogEventType(); // register the event type on this thread, before the callback can run
    static const SDL_DialogFileFilter filters[] = {{"ROM image (*.bin)", "bin"}, {"All files", "*"}};
    SDL_ShowOpenFileDialog(dialogCallback, nullptr, window, filters, SDL_arraysize(filters), nullptr, false);
}

std::optional<std::string> getRomDialogPath(const SDL_Event &event) {
    char *path = static_cast<char *>(event.user.data1);
    if (!path) {
        return std::nullopt;
    }
    std::string result(path);
    SDL_free(path);
    return result;
}
