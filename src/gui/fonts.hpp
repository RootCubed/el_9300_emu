#pragma once

#include <imgui.h>

// Sets up the GUI fonts, both are compiled into the program.
void loadFonts();

// Monospace font for register / disassembly text.
ImFont *monospaceFont();
