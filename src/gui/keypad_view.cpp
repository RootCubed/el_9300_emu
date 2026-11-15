#include "gui/keypad_view.hpp"

#include "gui/imgui_util.hpp"
#include "machine/keyboard.hpp"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstring>
#include <vector>

enum class KeyStyle { Function, Number, Second, Alpha, Clear };

// One key on the on-screen keypad.
struct KeyCap {
    const char *key;
    const char *label;
    const char *second = nullptr; // 2ndF function
    const char *alpha = nullptr;  // ALPHA function
    KeyStyle style = KeyStyle::Function;
    float width = 1.0f;           // in key units; every row is 6 units wide
};

constexpr float NUM_W = 6.0f / 5.0f; // the top row and the lower block has 5 keys per row

const KeyCap SPACER = { nullptr, nullptr };

const std::vector<KeyCap> ROWS[] = {
    {
        {"CALC", "CALC", "SOLVER", nullptr, KeyStyle::Function, NUM_W},
        {"GRAPH", "GRAPH", nullptr, nullptr, KeyStyle::Function, NUM_W},
        {"PROG", "PROG", nullptr, nullptr, KeyStyle::Function, NUM_W},
        {"DATA", "DATA", "STAT PLOT", nullptr, KeyStyle::Function, NUM_W},
        {"ON", "ON", "OFF", nullptr, KeyStyle::Function, NUM_W},
    },
    {
        {"EQTN", "EQTN", "JUMP"}, {"RANGE", "RANGE", "AUTO"}, {"ZOOM", "ZOOM", "PLOT"},
        SPACER, {"Up", "▲"}, SPACER,
    },
    {
        {"MATH", "MATH", "COMMAND"}, {"MENU", "MENU", "OPTION"}, {"QUIT", "QUIT"},
        {"Left", "◀"}, {"Down", "▼"}, {"Right", "▶"},
    },
    {
        {"2ndF", "2ndF", nullptr, nullptr, KeyStyle::Second},
        {"ALPHA", "ALPHA", "A-LOCK", nullptr, KeyStyle::Alpha},
        {"SETUP", "SET UP"}, {"DEL", "DEL", "INS"}, {"BS", "BS"},
        {"CL", "CL", "CA", nullptr, KeyStyle::Clear},
    },
    {
        {"sin", "sin", "sin⁻¹", "A"},
        {"cos", "cos", "cos⁻¹", "B"},
        {"tan", "tan", "tan⁻¹", "C"},
        {"x^2", "x²", "x⁻¹", "D"},
        {"log", "log", "10ˣ", "E"},
        {"ln", "ln", "eˣ", "F"},
    },
    {
        {"Exp", "Exp", "π", "G"},
        {"a/b", "a/b", "i", "H"},
        {"a^b", "aᵇ", "∠", "I"},
        {"sqrt", "√", "ˣ√", "J"},
        {"STO", "STO", "RCL", "K"},
        {"X", "X/θ/T", "MAT"},
    },
    {
        {"7", "7", nullptr, "L", KeyStyle::Number, NUM_W}, {"8", "8", nullptr, "M", KeyStyle::Number, NUM_W},
        {"9", "9", nullptr, "N", KeyStyle::Number, NUM_W}, {"(", "(", nullptr, "O", KeyStyle::Number, NUM_W},
        {")", ")", nullptr, "P", KeyStyle::Number, NUM_W},
    },
    {
        {"4", "4", nullptr, "Q", KeyStyle::Number, NUM_W}, {"5", "5", nullptr, "R", KeyStyle::Number, NUM_W},
        {"6", "6", nullptr, "S", KeyStyle::Number, NUM_W}, {"x", "×", nullptr, "T", KeyStyle::Number, NUM_W},
        {"/", "÷", nullptr, "U", KeyStyle::Number, NUM_W},
    },
    {
        {"1", "1", nullptr, "V", KeyStyle::Number, NUM_W}, {"2", "2", nullptr, "W", KeyStyle::Number, NUM_W},
        {"3", "3", nullptr, "X", KeyStyle::Number, NUM_W}, {"+", "+", nullptr, "Y", KeyStyle::Number, NUM_W},
        {"-", "−", nullptr, "Z", KeyStyle::Number, NUM_W},
    },
    {
        {"0", "0", nullptr, "θ", KeyStyle::Number, NUM_W},
        {".", ".", nullptr, ",", KeyStyle::Number, NUM_W},
        {"(-)", "(−)", nullptr, "SPACE", KeyStyle::Number, NUM_W},
        {"ENTER", "ENTER", "ANS", "=", KeyStyle::Number, 2 * NUM_W},
    },
};

constexpr ImColor SECOND_COLOR = hex("#FFC740");
constexpr ImColor ALPHA_COLOR = hex("#73BFFF");

struct ButtonColors {
    ImColor base, text;
};

ButtonColors colorsFor(KeyStyle style) {
    switch (style) {
        case KeyStyle::Number: return {hex("#1A1A1C"), hex("#FFFFFF")};
        case KeyStyle::Second: return {SECOND_COLOR, hex("#000000")};
        case KeyStyle::Alpha: return {ALPHA_COLOR, hex("#000000")};
        case KeyStyle::Clear: return {hex("#D94D4D"), hex("#FFFFFF")};
        default: return {hex("#42474F"), hex("#FFFFFF")};
    }
}

// Brightens a color.
ImColor brighten(const ImColor &c, float f) {
    auto b = [&](float v) {
        return std::min(std::max(v * f, v + 0.15f * (f - 1.0f)), 1.0f);
    };
    return ImColor(b(c.Value.x), b(c.Value.y), b(c.Value.z), c.Value.w);
}

// Draws an angle sign (∠), which the UI font (Source Sans 3) lacks. It takes the space a lowercase 'o' would take up.
void drawAngleSymbol(ImDrawList *dl, ImFont *font, float size, ImVec2 pos, ImU32 color) {
    const ImFontGlyph *h = font->GetFontBaked(size)->FindGlyph('o');
    const float base = pos.y + h->Y1, top = base - (h->Y1 - h->Y0);
    const float left = pos.x + h->X0, right = pos.x + h->X1;
    const ImVec2 points[] = {{right, top}, {left, base}, {right, base}};
    dl->AddPolyline(points, 3, color, ImDrawFlags_None, std::max(1.0f, size * 0.05f));
}

// Draws the 2ndF and ALPHA legends above a key.
void drawLegends(const KeyCap &cap, ImVec2 pos, float width) {
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImFont *font = ImGui::GetFont();

    const float size = ImGui::GetFontSize() * 0.95f;

    if (cap.second) {
        float textWidth = font->CalcTextSizeA(size, FLT_MAX, 0.0f, cap.second).x;
        float x = pos.x;
        // Center if there is no ALPHA legend
        if (!cap.alpha) {
            x = pos.x + (width - textWidth) * 0.5f;
        }
        if (std::strcmp(cap.second, "∠") == 0) {
            drawAngleSymbol(dl, font, size, ImVec2(x, pos.y), SECOND_COLOR);
        } else {
            dl->AddText(font, size, ImVec2(x, pos.y), SECOND_COLOR, cap.second);
        }
    }
    if (cap.alpha) {
        float textWidth = font->CalcTextSizeA(size, FLT_MAX, 0.0f, cap.alpha).x;
        float x = pos.x + width - textWidth;
        dl->AddText(font, size, ImVec2(x, pos.y), ALPHA_COLOR, cap.alpha);
    }
}

// Draws one key below its legend strip; returns true while it is held with the mouse.
bool drawKey(const KeyCap &cap, ImVec2 pos, ImVec2 size, float legendHeight, bool down) {
    drawLegends(cap, pos, size.x);

    const ButtonColors c = colorsFor(cap.style);
    ImGui::PushStyleColor(ImGuiCol_Button, down ? brighten(c.base, 1.8f).Value : c.base.Value);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, brighten(c.base, 1.3f).Value);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, brighten(c.base, 1.8f).Value);
    ImGui::PushStyleColor(ImGuiCol_Text, c.text.Value);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + legendHeight));
    ImGui::PushID(cap.key);
    ImGui::Button(cap.label, size);
    const bool held = ImGui::IsItemActive();
    ImGui::PopID();

    ImGui::PopStyleColor(4);

    return held;
}

// KeyStates index for a key name (a Keyboard::KEYS name or "ON"), or -1.
int keyIndex(const char *name) {
    return std::strcmp(name, "ON") == 0 ? ON_KEY : Keyboard::findKey(name);
}

void applyKeyStates(Keyboard &kb, const KeyStates &held) {
    for (int i = 0; i < Keyboard::NUM_KEYS; i++) {
        if (held[i] != kb.isKeyDown(i)) kb.setKey(i, held[i]);
    }
    if (held[ON_KEY] != kb.isOnKeyDown()) kb.setOnKey(held[ON_KEY]);
}

KeyStates drawKeypad(const Keyboard &kb) {
    KeyStates held{};

    beginWindow("Keypad", 500, 10, 470, 700);

    // Keys scale with the window width; rows are 6 units wide.
    const float gap = 6.0f * uiScale();
    const float unit = std::max(40.0f * uiScale(), (ImGui::GetContentRegionAvail().x - 5 * gap) / 6.0f);
    const float legendHeight = ImGui::GetFontSize();
    const float keyHeight = ImGui::GetFrameHeight() * 1.3f;
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    float y = origin.y;
    for (const std::vector<KeyCap> &row : ROWS) {
        float x = origin.x;
        for (const KeyCap &cap : row) {
            // A key spanning several units also covers the gaps between them.
            const float width = cap.width * unit + (cap.width - 1.0f) * gap;
            if (cap.key) {
                const int k = keyIndex(cap.key);
                const bool down = k == ON_KEY ? kb.isOnKeyDown() : kb.isKeyDown(k);
                held[k] = drawKey(cap, ImVec2(x, y), ImVec2(width, keyHeight), legendHeight, down);
            }
            x += width + gap;
        }
        y += legendHeight + keyHeight + gap;
    }

    ImGui::End();
    return held;
}
