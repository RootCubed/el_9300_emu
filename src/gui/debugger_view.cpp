#include "gui/debugger_view.hpp"

#include "cpu/instr.hpp"
#include "gui/app.hpp"
#include "gui/fonts.hpp"
#include "gui/imgui_util.hpp"

#include <imgui.h>

#include <format>
#include <string>

std::string DebuggerView::buildStateText(EL9300 &emu, bool running) {
    const SC62015 &cpu = emu.cpu;
    const Registers &r = cpu.getRegisters();

    auto io = [&](uint8_t addr) { return cpu.peekInt(addr); };

    static const char *runStates[] = {"Running", "HALT", "OFF"};

    std::string out = std::format("Status: {}   CPU: {}   Cycles: {}\n",
        running ? "Running" : "Paused",
        runStates[int(cpu.getRunState())],
        cpu.getCycles()
    );
    out += std::format("PC  {:05X}\n", cpu.getPC());
    out += std::format("BA  {:04X}    I  {:04X}\n", r.getBA(), r.getI());
    out += std::format("X   {:05X}   Y  {:05X}\n", r.getX(), r.getY());
    out += std::format("U   {:05X}   S  {:05X}\n", r.getU(), r.getS());
    out += std::format("F   {:02X}      Z  {:d}    C  {:d}\n", r.getF(), int(r.isZ()), int(r.isC()));
    out += std::format("BP  {:02X} PX  {:02X} PY {:02X} IMR {:02X} ISR {:02X} SCR {:02X}\n",
        io(SC62015::IREG_BP),
        io(SC62015::IREG_PX),
        io(SC62015::IREG_PY),
        io(SC62015::IREG_IMR),
        io(SC62015::IREG_ISR),
        io(SC62015::IREG_SCR)
    );
    out += std::format("KOL {:02X} KOH {:02X} KI {:02X} LCC {:02X} SSR {:02X}\n",
        io(SC62015::IREG_KOL),
        io(SC62015::IREG_KOH),
        io(SC62015::IREG_KI),
        io(SC62015::IREG_LCC),
        io(SC62015::IREG_SSR)
    );

    out += "\nDisassembly\n";
    uint32_t addr = cpu.getPC();
    for (int i = 0; i < 16; i++) {
        Instruction instr(addr, emu.memory);
        out += std::format("{} {:05X}  {:<18} {}\n", i == 0 ? '>' : ' ', addr, instr.bytesHex(), instr.to_string());
        addr = uint32_t(addr + instr.bytes.size()) & 0xFFFFF;
    }

    return out;
}

void DebuggerView::draw(App &app) {
    beginWindow("Debugger", 980, 10, 610, 560);

    EL9300 &emu = app.machine();
    RunControl &run = app.runControl();

    // Action buttons
    if (ImGui::Button(run.running ? "Pause" : "Run")) {
        run.running = !run.running;
        run.lastError.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) run.stepOnce(app.machine());
    ImGui::SameLine();
    if (ImGui::Button("Reset")) app.reset();

    if (ImGui::Button("Save state")) app.saveState();
    ImGui::SameLine();
    if (ImGui::Button("Load state")) app.loadState();
    ImGui::SameLine();
    ImGui::TextDisabled("(saved automatically on exit)");

    // Run settings
    if (ImGui::RadioButton("Max speed", run.maxSpeed)) run.maxSpeed = true;
    ImGui::SameLine();
    if (ImGui::RadioButton("Real time (2.304 MHz)", !run.maxSpeed)) run.maxSpeed = false;

    if (!run.lastError.empty()) {
        ImGui::TextColored(ERROR_COLOR, "Error: %s", run.lastError.c_str());
    }

    const std::string text = buildStateText(emu, run.running);
    ImGui::PushFont(monospaceFont(), ImGui::GetStyle().FontSizeBase * 0.9f);
    ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
    ImGui::PopFont();

    ImGui::End();
}
