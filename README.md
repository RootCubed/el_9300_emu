# EL-9300 Emulator

An emulator for the Sharp EL-9300 graphing calculator, built around its Sharp SC62015 ("ESR-L") CPU.

## What is emulated

- SC62015 CPU, including cycle timing taken from the ESR-L manual and checked against a real calculator
- Memory map: 256 KB ROM, 32 KB RAM, LCD
- 96×64 dot-matrix LCD
- Keyboard matrix and ON key

The link port (printer / PC connection) is currently not emulated.

## ROM

The ROM is not included, you need to provide your own 256 KB dump of the ROM chip of an EL-9300.

## Building

Requirements: CMake 4.1 or newer and a C++20 compiler. Developed and tested on Windows with
MSVC; the code only uses SDL3 and standard C++ (plus a Windows-only DPI setting), but other
platforms have not been tried yet.

```sh
git clone --recursive <repository URL>
cmake -S . -B build
cmake --build build
```

## Usage

```sh
build/bin/el-9300 [path/to/rom.bin]
```

Without a path, a start screen lets you pick the ROM file. Input is through the on-screen
keypad. The machine state (RAM, LCD, CPU) is saved to `state.bin` in the working directory
on exit and restored on the next start, like the real calculator keeping its RAM while
switched off.

The debugger window can pause, single-step and reset the CPU, save and load the state
manually, and switch between real-time and maximum speed.

## AI disclaimer

This project was created with heavy assistance from AI tools. After I was able to dump the ROM, I created the boilerplate for the GUI and a rough outline of the emulator core, after which Claude Code generated the rest of the emulator.

I manually went through the whole codebase and rewrote a large amount of it, but the CPU core was left mostly unchanged. That part of the code is still a bit messy because I haven't gotten around to cleaning it up yet.

## License

The emulator's source code is released under the [MIT License](LICENSE). Bundled and downloaded third-party components keep their own licenses: SDL3 (zlib), Dear ImGui (MIT), Source Sans 3 and IBM Plex Mono (SIL Open Font License 1.1). Distributed binaries must include the licenses of all third-party components.

This project is not affiliated with or endorsed by Sharp Corporation. "Sharp" and "EL-9300" are used only to identify the emulated hardware. No ROM or other Sharp software is included.
