#pragma once

#include <imgui.h>

#include <cstdint>

struct SDL_Texture;

// "#RRGGBB" -> opaque color. Written as a string so editor extensions can preview it.
constexpr ImColor hex(const char (&s)[8]) {
    uint32_t value = 0;
    for (int i = 1; i < 7; i++) {
        const char c = s[i] | 0x20; // lower case
        value = value * 16 + (c <= '9' ? c - '0' : c - 'a' + 10);
    }
    return ImColor(int(value >> 16), int((value >> 8) & 0xFF), int(value & 0xFF));
}

// Text color for error messages.
constexpr ImColor ERROR_COLOR = hex("#FF6666");

// Display scale (e.g. 1.5 at 150% Windows scaling); set at startup in main.cpp.
inline float uiScale() {
    return ImGui::GetStyle().FontScaleDpi;
}

// ImGui::Begin() with a default initial position and size.
inline void beginWindow(const char *name, float x, float y, float w, float h) {
    const float s = uiScale();
    ImGui::SetNextWindowPos(ImVec2(x * s, y * s), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w * s, h * s), ImGuiCond_FirstUseEver);
    ImGui::Begin(name);
}
