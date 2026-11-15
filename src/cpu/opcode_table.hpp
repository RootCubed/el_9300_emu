#pragma once

#include "cpu/instr.hpp"

#include <cstdint>

// Returns a table entry for an opcode byte.
// Prebytes have no handler, the decoder consumes them before looking up the opcode.
const InstrMeta &opcodeInfo(uint8_t opcode);
