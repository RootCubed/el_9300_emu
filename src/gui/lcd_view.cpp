#include "gui/lcd_view.hpp"

#include "gui/imgui_util.hpp"
#include "machine/el9300.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <cmath>
#include <cstring>

namespace {

constexpr ImColor PIXEL_ON = hex("#222113");
constexpr ImColor PIXEL_OFF = hex("#B1B997");

constexpr int MARGIN = 2;
constexpr int TEXTURE_W = Lcd::WIDTH + 2 * MARGIN;
constexpr int TEXTURE_H = Lcd::HEIGHT + 2 * MARGIN;

} // namespace

LcdView::LcdView(SDL_Renderer *renderer) {
    texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
        TEXTURE_W, TEXTURE_H
    );
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
}

LcdView::~LcdView() {
    SDL_DestroyTexture(texture);
}

template <typename Dot>
void rasterize(uint8_t *dst, int pitch, int w, int h, int margin, Dot dot) {
    const ImU32 on = PIXEL_ON, off = PIXEL_OFF;
    for (int y = 0; y < h; y++) {
        uint8_t *row = dst + y * pitch;
        for (int x = 0; x < w; x++) {
            const int bx = x - margin, by = y - margin;
            const bool inside = bx >= 0 && bx < w - 2 * margin && by >= 0 && by < h - 2 * margin;
            const ImU32 c = inside && dot(bx, by) ? on : off;
            std::memcpy(row + x * 4, &c, 4);
        }
    }
}

void LcdView::draw(const EL9300 &emu) {
    void *pixels;
    int pitch;

    const Lcd &lcd = emu.lcd();
    const bool off = emu.cpu.getRunState() == SC62015::RunState::Off;

    if (SDL_LockTexture(texture, nullptr, &pixels, &pitch)) {
        rasterize(static_cast<uint8_t *>(pixels), pitch,
            TEXTURE_W, TEXTURE_H, MARGIN,
            [&](int x, int y) { return !off && lcd.pixel(x, y); });
        SDL_UnlockTexture(texture);
    }

    beginWindow("LCD", 10, 10, 500, 400);

    // Annunciators above the dot matrix
    for (const Lcd::Annunciator &a : Lcd::ANNUNCIATORS) {
        const bool on = !off && lcd.annunciator(a);
        ImGui::TextColored(on ? hex("#F2F2F2") : hex("#4D4D4D"), "%s", a.name);
        ImGui::SameLine(0, 18 * uiScale());
    }
    ImGui::NewLine();

    const float scale = std::max(1.0f, std::floor(ImGui::GetContentRegionAvail().x / TEXTURE_W));
    ImTextureRef texRef(ImTextureID(reinterpret_cast<intptr_t>(texture)));

    ImGuiPlatformIO &pio = ImGui::GetPlatformIO();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    if (pio.DrawCallback_SetSamplerNearest) {
        dl->AddCallback(pio.DrawCallback_SetSamplerNearest, nullptr);
    }
    ImGui::Image(texRef, ImVec2(TEXTURE_W * scale, TEXTURE_H * scale));
    if (pio.DrawCallback_SetSamplerLinear) {
        dl->AddCallback(pio.DrawCallback_SetSamplerLinear, nullptr);
    }

    ImGui::End();
}
