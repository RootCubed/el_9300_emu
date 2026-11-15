#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Reads the ROM image. Throws std::runtime_error if it can't be opened or has the wrong size.
std::vector<uint8_t> loadRom(const std::string &path);

// Shows the open-file dialog for the ROM and returns immediately. The result arrives
// later as an SDL event of type romDialogEventType().
void openRomDialog(SDL_Window *window);

// Returns the SDL event type for the ROM dialog result.
uint32_t romDialogEventType();

// The path picked in the dialog, or nullopt if it was cancelled. Only call once per event.
std::optional<std::string> getRomDialogPath(const SDL_Event &event);
