#include "fonts.hpp"

// Compressed into the program at build time (see libs/CMakeLists.txt).
#include "IBMPlexMono.inc"
#include "SourceSans3.inc"

ImFont *mono = nullptr;

void loadFonts() {
    ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(SourceSans3_compressed_data, SourceSans3_compressed_size);
    mono = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(IBMPlexMono_compressed_data, IBMPlexMono_compressed_size);
}

ImFont *monospaceFont() {
    return mono;
}
