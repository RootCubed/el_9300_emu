# SDL3
set(SDL3_DIR "SDL")
set(SDL_STATIC ON)
add_subdirectory(${SDL3_DIR})

# imgui
set(IMGUI_DIR "imgui")
add_library(IMGUI STATIC)

target_sources(IMGUI
    PRIVATE
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui.cpp

    PRIVATE
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

target_include_directories(IMGUI
    PUBLIC ${IMGUI_DIR}
    PUBLIC ${IMGUI_DIR}/backends
    PUBLIC ${SDL3_DIR}/include
)

target_link_libraries(IMGUI PRIVATE SDL3-static)

# Fonts, downloaded at configure time and compiled into the program:
#   Source Sans 3 (SIL Open Font License) - UI font
#   IBM Plex Mono (SIL Open Font License) - debugger
include(FetchContent)
FetchContent_Declare(source_sans
    URL https://github.com/adobe-fonts/source-sans/releases/download/3.052R/TTF-source-sans-3.052R.zip
    URL_HASH SHA256=1b0dd1ec44b39f1dd98bbd153a1a3815f083639874ddee02c842bd601bad3d21
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(plex_mono
    URL https://github.com/IBM/plex/releases/download/%40ibm/plex-mono%402.5.0/ibm-plex-mono.zip
    URL_HASH SHA256=6d23f01257663d8cc49a0d64c22ced630b79e0e2a0ac08a0da86e9a38bbc481c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(source_sans plex_mono)

# Build tool only: keep it out of bin/, next to the emulator.
add_executable(binary_to_compressed_c ${IMGUI_DIR}/misc/fonts/binary_to_compressed_c.cpp)
set_target_properties(binary_to_compressed_c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools)

# <symbol>=<ttf>: each font becomes <symbol>.inc defining <symbol>_compressed_data/_size.
set(FONTS
    SourceSans3=${source_sans_SOURCE_DIR}/TTF/SourceSans3-Regular.ttf
    IBMPlexMono=${plex_mono_SOURCE_DIR}/fonts/complete/ttf/IBMPlexMono-Regular.ttf
)

set(FONT_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/fonts)
set(FONT_INCS)
foreach(entry ${FONTS})
    string(REPLACE "=" ";" parts ${entry})
    list(GET parts 0 font)
    list(GET parts 1 ttf)
    set(inc ${FONT_GEN_DIR}/${font}.inc)
    add_custom_command(
        OUTPUT ${inc}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${FONT_GEN_DIR}
        COMMAND ${CMAKE_COMMAND}
            -DTOOL=$<TARGET_FILE:binary_to_compressed_c> -DINPUT=${ttf} -DSYMBOL=${font} -DOUTPUT=${inc}
            -P ${PROJECT_SOURCE_DIR}/cmake/embed_font.cmake
        DEPENDS binary_to_compressed_c ${ttf} ${PROJECT_SOURCE_DIR}/cmake/embed_font.cmake
        COMMENT "Embedding ${font}"
        VERBATIM
    )
    list(APPEND FONT_INCS ${inc})
endforeach()
add_custom_target(embedded_fonts_gen DEPENDS ${FONT_INCS})

# Provides <symbol>.inc (e.g. IBMPlexSans.inc) on the include path.
add_library(EMBEDDED_FONTS INTERFACE)
target_include_directories(EMBEDDED_FONTS INTERFACE ${FONT_GEN_DIR})
add_dependencies(EMBEDDED_FONTS embedded_fonts_gen)
