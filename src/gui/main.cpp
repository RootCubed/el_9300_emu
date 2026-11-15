#include "gui/app.hpp"
#include "gui/fonts.hpp"
#include "gui/imgui_util.hpp"
#include "gui/rom_dialog.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstdio>
#include <optional>
#include <string>

#ifdef WIN32
#define NOMINMAX
#include <ShellScalingAPI.h>
#endif

int main(int argc, char **argv) {
#ifdef WIN32
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL Init Error: %s\n", SDL_GetError());
        return -1;
    }

    const SDL_DisplayID display = SDL_GetPrimaryDisplay();
    float scale = SDL_GetDisplayContentScale(display);

    SDL_Window *window = SDL_CreateWindow("EL-9300 Emulator", 1600 * scale, 900 * scale,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = "imgui.ini";
    loadFonts();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 6;
    style.FrameRounding = 3;
    style.FontSizeBase = 18.0f;
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;

    // The app only exists once a ROM is loaded (from the command line or the start screen).
    std::optional<App> app;
    std::string romError;
    bool romDialogOpen = false;

    if (argc > 1) {
        try {
            app.emplace(renderer, loadRom(argv[1]));
        } catch (const std::exception &e) {
            romError = e.what();
        }
    }

    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == romDialogEventType()) {
                romDialogOpen = false;
                auto path = getRomDialogPath(event);
                if (path && !app) {
                    try {
                        app.emplace(renderer, loadRom(*path));
                    } catch (const std::exception &e) {
                        romError = e.what();
                    }
                }
            }
            if (
                event.type == SDL_EVENT_QUIT ||
                event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)
            ) {
                quit = true;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (!app) {
            const ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            if (ImGui::Begin("Load ROM", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove)) {
                ImGui::TextUnformatted("No ROM loaded. Select your EL-9300 ROM image (256 KB) to start.");
                ImGui::Spacing();
                ImGui::BeginDisabled(romDialogOpen);
                if (ImGui::Button("Open ROM...")) {
                    romDialogOpen = true;
                    openRomDialog(window);
                }
                ImGui::EndDisabled();
                if (!romError.empty()) {
                    ImGui::Spacing();
                    ImGui::TextColored(ERROR_COLOR, "%s", romError.c_str());
                }
            }
            ImGui::End();
        } else {
            app->frame(io.DeltaTime);
        }

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    if (app) {
        app->saveState(); // The real calculator keeps its RAM while off
        app = std::nullopt; // Destroy before the renderer so that the textures are freed before the renderer is destroyed
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
