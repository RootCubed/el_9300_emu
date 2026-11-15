add_executable(el-9300
    cpu/disassembler.cpp
    cpu/handlers_alu.cpp
    cpu/handlers_control.cpp
    cpu/handlers_data.cpp
    cpu/instr.cpp
    cpu/opcode_table.cpp
    cpu/operands.cpp
    cpu/registers.cpp
    cpu/sc62015.cpp
    cpu/sc62015_state.cpp
    machine/el9300.cpp
    machine/keyboard.cpp
    machine/memory.cpp
    gui/app.cpp
    gui/debugger_view.cpp
    gui/fonts.cpp
    gui/keypad_view.cpp
    gui/lcd_view.cpp
    gui/main.cpp
    gui/rom_dialog.cpp
    gui/run_control.cpp
)
target_include_directories(el-9300 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(el-9300 PRIVATE IMGUI SDL3-static EMBEDDED_FONTS)
if(WIN32)
    target_link_libraries(el-9300 PRIVATE shcore)
endif()
