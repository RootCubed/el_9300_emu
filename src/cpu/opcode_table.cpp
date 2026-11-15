// SC62015 opcode table: mnemonic, operand kinds, data width, execution cycles, handler.
//
// Cycles are from the ESR-L manual's command table, a prebyte adds one additional cycle.
// Loop instructions add their per-byte cost (the "+ i" part) in the handler.

#include "cpu/opcode_table.hpp"
#include "cpu/handlers.hpp"

#include <array>

namespace {

// Some aliases to make the table more concise.
using O = Opnd;
constexpr Opnd N = O::None;
using namespace handlers;

std::array<InstrMeta, 256> buildTable() {
    std::array<InstrMeta, 256> t{};

    auto op = [&](
        int code, const char *mnemonic,
        O a, O b, uint8_t width, std::array<uint8_t, 4> cycles,
        InstrHandler h
    ) {
        t[code] = InstrMeta{mnemonic, a, b, width, cycles, h};
    };

    op(0x00, "NOP", N, N, 0, {1}, nop);
    op(0x01, "RETI", N, N, 0, {7}, reti);
    op(0x02, "JP", O::Addr16, N, 0, {4}, jp);
    op(0x03, "JPF", O::Addr20, N, 0, {5}, jp);
    op(0x04, "CALL", O::Addr16, N, 0, {6}, call);
    op(0x05, "CALLF", O::Addr20, N, 0, {8}, call);
    op(0x06, "RET", N, N, 0, {4}, ret);
    op(0x07, "RETF", N, N, 0, {5}, retf);

    op(0x08, "MV", O::A, O::Imm, 0, {2}, mv);
    op(0x09, "MV", O::IL, O::Imm, 0, {3}, mv);
    op(0x0A, "MV", O::BA, O::Imm, 0, {3}, mv);
    op(0x0B, "MV", O::I, O::Imm, 0, {3}, mv);
    op(0x0C, "MV", O::X, O::Imm, 0, {4}, mv);
    op(0x0D, "MV", O::Y, O::Imm, 0, {4}, mv);
    op(0x0E, "MV", O::U, O::Imm, 0, {4}, mv);
    op(0x0F, "MV", O::S, O::Imm, 0, {4}, mv);

    op(0x10, "JP", O::Int, N, 0, {6}, jp);
    op(0x11, "JP", O::RegSel, N, 0, {4}, jp);
    op(0x12, "JR", O::RelP, N, 0, {3}, jr);
    op(0x13, "JR", O::RelM, N, 0, {3}, jr);
    op(0x14, "JPZ", O::Addr16, N, 0, {4}, jpcc);
    op(0x15, "JPNZ", O::Addr16, N, 0, {4}, jpcc);
    op(0x16, "JPC", O::Addr16, N, 0, {4}, jpcc);
    op(0x17, "JPNC", O::Addr16, N, 0, {4}, jpcc);
    op(0x18, "JRZ", O::RelP, N, 0, {3}, jrcc);
    op(0x19, "JRZ", O::RelM, N, 0, {3}, jrcc);
    op(0x1A, "JRNZ", O::RelP, N, 0, {3}, jrcc);
    op(0x1B, "JRNZ", O::RelM, N, 0, {3}, jrcc);
    op(0x1C, "JRC", O::RelP, N, 0, {3}, jrcc);
    op(0x1D, "JRC", O::RelM, N, 0, {3}, jrcc);
    op(0x1E, "JRNC", O::RelP, N, 0, {3}, jrcc);
    op(0x1F, "JRNC", O::RelM, N, 0, {3}, jrcc);

    // 20-27: prebytes (decoded in Instruction, not in this table)

    op(0x28, "PUSHU", O::A, N, 0, {3}, pushu);
    op(0x29, "PUSHU", O::IL, N, 0, {3}, pushu);
    op(0x2A, "PUSHU", O::BA, N, 0, {4}, pushu);
    op(0x2B, "PUSHU", O::I, N, 0, {4}, pushu);
    op(0x2C, "PUSHU", O::X, N, 0, {5}, pushu);
    op(0x2D, "PUSHU", O::Y, N, 0, {5}, pushu);
    op(0x2E, "PUSHU", O::F, N, 0, {3}, pushu);
    op(0x2F, "PUSHU", O::IMR, N, 0, {3}, pushu);

    // 30-37: prebytes (decoded in Instruction, not in this table)

    op(0x38, "POPU", O::A, N, 0, {2}, popu);
    op(0x39, "POPU", O::IL, N, 0, {3}, popu);
    op(0x3A, "POPU", O::BA, N, 0, {3}, popu);
    op(0x3B, "POPU", O::I, N, 0, {3}, popu);
    op(0x3C, "POPU", O::X, N, 0, {4}, popu);
    op(0x3D, "POPU", O::Y, N, 0, {4}, popu);
    op(0x3E, "POPU", O::F, N, 0, {2}, popu);
    op(0x3F, "POPU", O::IMR, N, 0, {2}, popu);

    op(0x40, "ADD", O::A, O::Imm, 1, {3}, alu<Alu::Add>);
    op(0x41, "ADD", O::Int, O::Imm, 1, {4}, alu<Alu::Add>);
    op(0x42, "ADD", O::A, O::Int, 1, {4}, alu<Alu::Add>);
    op(0x43, "ADD", O::Int, O::A, 1, {4}, alu<Alu::Add>);
    op(0x44, "ADD", O::RegHi, O::RegLo, 2, {5}, alu<Alu::Add>);
    op(0x45, "ADD", O::RegHi, O::RegLo, 3, {7}, alu<Alu::Add>);
    op(0x46, "ADD", O::RegHi, O::RegLo, 1, {3}, alu<Alu::Add>);
    op(0x47, "PMDF", O::Int, O::Imm, 1, {4}, alu<Alu::Pmdf>);

    op(0x48, "SUB", O::A, O::Imm, 1, {3}, alu<Alu::Sub>);
    op(0x49, "SUB", O::Int, O::Imm, 1, {4}, alu<Alu::Sub>);
    op(0x4A, "SUB", O::A, O::Int, 1, {4}, alu<Alu::Sub>);
    op(0x4B, "SUB", O::Int, O::A, 1, {4}, alu<Alu::Sub>);
    op(0x4C, "SUB", O::RegHi, O::RegLo, 2, {5}, alu<Alu::Sub>);
    op(0x4D, "SUB", O::RegHi, O::RegLo, 3, {7}, alu<Alu::Sub>);
    op(0x4E, "SUB", O::RegHi, O::RegLo, 1, {3}, alu<Alu::Sub>);
    op(0x4F, "PUSHS", O::F, N, 0, {3}, pushs);

    op(0x50, "ADC", O::A, O::Imm, 1, {3}, alu<Alu::Adc>);
    op(0x51, "ADC", O::Int, O::Imm, 1, {4}, alu<Alu::Adc>);
    op(0x52, "ADC", O::A, O::Int, 1, {4}, alu<Alu::Adc>);
    op(0x53, "ADC", O::Int, O::A, 1, {4}, alu<Alu::Adc>);
    op(0x54, "ADCL", O::Int, O::Int, 0, {5}, loop<Loop::Adcl>);
    op(0x55, "ADCL", O::Int, O::A, 0, {4}, loop<Loop::Adcl>);
    op(0x56, "MVL", O::Int, O::ExtReg, 0, {5, 5, 7, 5}, mvl);
    op(0x57, "PMDF", O::Int, O::A, 1, {4}, alu<Alu::Pmdf>);

    op(0x58, "SBC", O::A, O::Imm, 1, {3}, alu<Alu::Sbc>);
    op(0x59, "SBC", O::Int, O::Imm, 1, {4}, alu<Alu::Sbc>);
    op(0x5A, "SBC", O::A, O::Int, 1, {4}, alu<Alu::Sbc>);
    op(0x5B, "SBC", O::Int, O::A, 1, {4}, alu<Alu::Sbc>);
    op(0x5C, "SBCL", O::Int, O::Int, 0, {5}, loop<Loop::Sbcl>);
    op(0x5D, "SBCL", O::Int, O::A, 0, {4}, loop<Loop::Sbcl>);
    op(0x5E, "MVL", O::ExtReg, O::Int, 0, {5, 5, 7, 5}, mvl);
    op(0x5F, "POPS", O::F, N, 0, {2}, pops);

    op(0x60, "CMP", O::A, O::Imm, 1, {3}, alu<Alu::Cmp>);
    op(0x61, "CMP", O::Int, O::Imm, 1, {4}, alu<Alu::Cmp>);
    op(0x62, "CMP", O::Abs, O::Imm, 1, {6}, alu<Alu::Cmp>);
    op(0x63, "CMP", O::Int, O::A, 1, {4}, alu<Alu::Cmp>);
    op(0x64, "TEST", O::A, O::Imm, 1, {3}, alu<Alu::Test>);
    op(0x65, "TEST", O::Int, O::Imm, 1, {4}, alu<Alu::Test>);
    op(0x66, "TEST", O::Abs, O::Imm, 1, {6}, alu<Alu::Test>);
    op(0x67, "TEST", O::Int, O::A, 1, {4}, alu<Alu::Test>);
    op(0x68, "XOR", O::A, O::Imm, 1, {3}, alu<Alu::Xor>);
    op(0x69, "XOR", O::Int, O::Imm, 1, {4}, alu<Alu::Xor>);
    op(0x6A, "XOR", O::Abs, O::Imm, 1, {7}, alu<Alu::Xor>);
    op(0x6B, "XOR", O::Int, O::A, 1, {4}, alu<Alu::Xor>);
    op(0x6C, "INC", O::RegSel, N, 0, {3}, inc);
    op(0x6D, "INC", O::Int, N, 1, {3}, inc);
    op(0x6E, "XOR", O::Int, O::Int, 1, {6}, alu<Alu::Xor>);
    op(0x6F, "XOR", O::A, O::Int, 1, {4}, alu<Alu::Xor>);

    op(0x70, "AND", O::A, O::Imm, 1, {3}, alu<Alu::And>);
    op(0x71, "AND", O::Int, O::Imm, 1, {4}, alu<Alu::And>);
    op(0x72, "AND", O::Abs, O::Imm, 1, {7}, alu<Alu::And>);
    op(0x73, "AND", O::Int, O::A, 1, {4}, alu<Alu::And>);
    op(0x74, "MV", O::A, O::B, 1, {1}, mv);
    op(0x75, "MV", O::B, O::A, 1, {1}, mv);
    op(0x76, "AND", O::Int, O::Int, 1, {6}, alu<Alu::And>);
    op(0x77, "AND", O::A, O::Int, 1, {4}, alu<Alu::And>);
    op(0x78, "OR", O::A, O::Imm, 1, {3}, alu<Alu::Or>);
    op(0x79, "OR", O::Int, O::Imm, 1, {4}, alu<Alu::Or>);
    op(0x7A, "OR", O::Abs, O::Imm, 1, {7}, alu<Alu::Or>);
    op(0x7B, "OR", O::Int, O::A, 1, {4}, alu<Alu::Or>);
    op(0x7C, "DEC", O::RegSel, N, 0, {3}, dec);
    op(0x7D, "DEC", O::Int, N, 1, {3}, dec);
    op(0x7E, "OR", O::Int, O::Int, 1, {6}, alu<Alu::Or>);
    op(0x7F, "OR", O::A, O::Int, 1, {4}, alu<Alu::Or>);

    op(0x80, "MV", O::A, O::Int, 0, {3}, mv);
    op(0x81, "MV", O::IL, O::Int, 0, {4}, mv);
    op(0x82, "MV", O::BA, O::Int, 0, {4}, mv);
    op(0x83, "MV", O::I, O::Int, 0, {4}, mv);
    op(0x84, "MV", O::X, O::Int, 0, {5}, mv);
    op(0x85, "MV", O::Y, O::Int, 0, {5}, mv);
    op(0x86, "MV", O::U, O::Int, 0, {5}, mv);
    op(0x87, "MV", O::S, O::Int, 0, {5}, mv);
    op(0x88, "MV", O::A, O::Abs, 0, {6}, mv);
    op(0x89, "MV", O::IL, O::Abs, 0, {6}, mv);
    op(0x8A, "MV", O::BA, O::Abs, 0, {7}, mv);
    op(0x8B, "MV", O::I, O::Abs, 0, {7}, mv);
    op(0x8C, "MV", O::X, O::Abs, 0, {8}, mv);
    op(0x8D, "MV", O::Y, O::Abs, 0, {8}, mv);
    op(0x8E, "MV", O::U, O::Abs, 0, {8}, mv);
    op(0x8F, "MV", O::S, O::Abs, 0, {8}, mv);

    op(0x90, "MV", O::A, O::ExtReg, 0, {4, 4, 5, 6}, mv);
    op(0x91, "MV", O::IL, O::ExtReg, 0, {5, 5, 6, 7}, mv);
    op(0x92, "MV", O::BA, O::ExtReg, 0, {5, 5, 6, 7}, mv);
    op(0x93, "MV", O::I, O::ExtReg, 0, {5, 5, 6, 7}, mv);
    op(0x94, "MV", O::X, O::ExtReg, 0, {6, 7, 8, 8}, mv);
    op(0x95, "MV", O::Y, O::ExtReg, 0, {6, 7, 8, 8}, mv);
    op(0x96, "MV", O::U, O::ExtReg, 0, {6, 7, 8, 8}, mv);
    op(0x97, "SC", N, N, 0, {1}, sc);
    op(0x98, "MV", O::A, O::ExtInd, 0, {9, 11}, mv);
    op(0x99, "MV", O::IL, O::ExtInd, 0, {10, 12}, mv);
    op(0x9A, "MV", O::BA, O::ExtInd, 0, {10, 12}, mv);
    op(0x9B, "MV", O::I, O::ExtInd, 0, {10, 12}, mv);
    op(0x9C, "MV", O::X, O::ExtInd, 0, {11, 13}, mv);
    op(0x9D, "MV", O::Y, O::ExtInd, 0, {11, 13}, mv);
    op(0x9E, "MV", O::U, O::ExtInd, 0, {11, 13}, mv);
    op(0x9F, "RC", N, N, 0, {1}, rc);

    op(0xA0, "MV", O::Int, O::A, 0, {3}, mv);
    op(0xA1, "MV", O::Int, O::IL, 0, {3}, mv);
    op(0xA2, "MV", O::Int, O::BA, 0, {4}, mv);
    op(0xA3, "MV", O::Int, O::I, 0, {4}, mv);
    op(0xA4, "MV", O::Int, O::X, 0, {5}, mv);
    op(0xA5, "MV", O::Int, O::Y, 0, {5}, mv);
    op(0xA6, "MV", O::Int, O::U, 0, {5}, mv);
    op(0xA7, "MV", O::Int, O::S, 0, {5}, mv);
    op(0xA8, "MV", O::Abs, O::A, 0, {5}, mv);
    op(0xA9, "MV", O::Abs, O::IL, 0, {5}, mv);
    op(0xAA, "MV", O::Abs, O::BA, 0, {6}, mv);
    op(0xAB, "MV", O::Abs, O::I, 0, {6}, mv);
    op(0xAC, "MV", O::Abs, O::X, 0, {7}, mv);
    op(0xAD, "MV", O::Abs, O::Y, 0, {7}, mv);
    op(0xAE, "MV", O::Abs, O::U, 0, {7}, mv);
    op(0xAF, "MV", O::Abs, O::S, 0, {7}, mv);

    op(0xB0, "MV", O::ExtReg, O::A, 0, {4, 4, 5, 6}, mv);
    op(0xB1, "MV", O::ExtReg, O::IL, 0, {4, 4, 5, 6}, mv);
    op(0xB2, "MV", O::ExtReg, O::BA, 0, {5, 5, 6, 7}, mv);
    op(0xB3, "MV", O::ExtReg, O::I, 0, {5, 5, 6, 7}, mv);
    op(0xB4, "MV", O::ExtReg, O::X, 0, {6, 7, 8, 8}, mv);
    op(0xB5, "MV", O::ExtReg, O::Y, 0, {6, 7, 8, 8}, mv);
    op(0xB6, "MV", O::ExtReg, O::U, 0, {6, 7, 8, 8}, mv);
    op(0xB7, "CMP", O::Int, O::Int, 1, {6}, alu<Alu::Cmp>);
    op(0xB8, "MV", O::ExtInd, O::A, 0, {9, 11}, mv);
    op(0xB9, "MV", O::ExtInd, O::IL, 0, {9, 11}, mv);
    op(0xBA, "MV", O::ExtInd, O::BA, 0, {10, 12}, mv);
    op(0xBB, "MV", O::ExtInd, O::I, 0, {10, 12}, mv);
    op(0xBC, "MV", O::ExtInd, O::X, 0, {11, 13}, mv);
    op(0xBD, "MV", O::ExtInd, O::Y, 0, {11, 13}, mv);
    op(0xBE, "MV", O::ExtInd, O::U, 0, {11, 13}, mv);
    op(0xBF, "MV", O::ExtInd, O::S, 0, {11, 13}, mv);

    op(0xC0, "EX", O::Int, O::Int, 1, {7}, ex);
    op(0xC1, "EXW", O::Int, O::Int, 2, {10}, ex);
    op(0xC2, "EXP", O::Int, O::Int, 3, {13}, ex);
    op(0xC3, "EXL", O::Int, O::Int, 0, {5}, exl);
    op(0xC4, "DADL", O::Int, O::Int, 0, {5}, loop<Loop::Dadl>);
    op(0xC5, "DADL", O::Int, O::A, 0, {4}, loop<Loop::Dadl>);
    op(0xC6, "CMPW", O::Int, O::Int, 2, {8}, alu<Alu::Cmp>);
    op(0xC7, "CMPP", O::Int, O::Int, 3, {10}, alu<Alu::Cmp>);
    op(0xC8, "MV", O::Int, O::Int, 1, {6}, mv);
    op(0xC9, "MVW", O::Int, O::Int, 2, {8}, mv);
    op(0xCA, "MVP", O::Int, O::Int, 3, {10}, mv);
    op(0xCB, "MVL", O::Int, O::Int, 0, {5}, mvl);
    op(0xCC, "MV", O::Int, O::Imm, 1, {3}, mv);
    op(0xCD, "MVW", O::Int, O::Imm, 2, {4}, mv);
    op(0xCE, "TCL", N, N, 0, {1}, tcl);
    op(0xCF, "MVLD", O::Int, O::Int, 0, {5}, mvld);

    op(0xD0, "MV", O::Int, O::Abs, 1, {7}, mv);
    op(0xD1, "MVW", O::Int, O::Abs, 2, {8}, mv);
    op(0xD2, "MVP", O::Int, O::Abs, 3, {9}, mv);
    op(0xD3, "MVL", O::Int, O::Abs, 0, {6}, mvl);
    op(0xD4, "DSBL", O::Int, O::Int, 0, {5}, loop<Loop::Dsbl>);
    op(0xD5, "DSBL", O::Int, O::A, 0, {4}, loop<Loop::Dsbl>);
    op(0xD6, "CMPW", O::Int, O::RegSel, 2, {7}, alu<Alu::Cmp>);
    op(0xD7, "CMPP", O::Int, O::RegSel, 3, {9}, alu<Alu::Cmp>);
    op(0xD8, "MV", O::Abs, O::Int, 1, {6}, mv);
    op(0xD9, "MVW", O::Abs, O::Int, 2, {7}, mv);
    op(0xDA, "MVP", O::Abs, O::Int, 3, {8}, mv);
    op(0xDB, "MVL", O::Abs, O::Int, 0, {6}, mvl);
    op(0xDC, "MVP", O::Int, O::Imm, 3, {5}, mv);
    op(0xDD, "EX", O::A, O::B, 1, {3}, ex);
    op(0xDE, "HALT", N, N, 0, {2}, halt);
    op(0xDF, "OFF", N, N, 0, {2}, off);

    op(0xE0, "MV", O::Int, O::ExtReg, 1, {6, 6, 7, 8}, mv);
    op(0xE1, "MVW", O::Int, O::ExtReg, 2, {7, 7, 10, 9}, mv);
    op(0xE2, "MVP", O::Int, O::ExtReg, 3, {8, 9, 10, 10}, mv);
    op(0xE3, "MVL", O::Int, O::ExtReg, 0, {5, 5, 7, 5}, mvl);
    op(0xE4, "ROR", O::A, N, 1, {2}, rotate<Rot::Ror>);
    op(0xE5, "ROR", O::Int, N, 1, {3}, rotate<Rot::Ror>);
    op(0xE6, "ROL", O::A, N, 1, {2}, rotate<Rot::Rol>);
    op(0xE7, "ROL", O::Int, N, 1, {3}, rotate<Rot::Rol>);
    op(0xE8, "MV", O::ExtReg, O::Int, 1, {6, 6, 7, 8}, mv);
    op(0xE9, "MVW", O::ExtReg, O::Int, 2, {7, 7, 10, 9}, mv);
    op(0xEA, "MVP", O::ExtReg, O::Int, 3, {8, 9, 10, 10}, mv);
    op(0xEB, "MVL", O::ExtReg, O::Int, 0, {5, 5, 7, 5}, mvl);
    op(0xEC, "DSLL", O::Int, N, 0, {4}, dsll);
    op(0xED, "EX", O::RegHi, O::RegLo, 0, {4}, ex);
    op(0xEE, "SWAP", O::A, N, 1, {3}, swap);
    op(0xEF, "WAIT", N, N, 0, {1}, wait);

    op(0xF0, "MV", O::Int, O::ExtInd, 1, {11, 13}, mv);
    op(0xF1, "MVW", O::Int, O::ExtInd, 2, {12, 14}, mv);
    op(0xF2, "MVP", O::Int, O::ExtInd, 3, {13, 15}, mv);
    op(0xF3, "MVL", O::Int, O::ExtInd, 0, {10, 12}, mvl);
    op(0xF4, "SHR", O::A, N, 1, {2}, rotate<Rot::Shr>);
    op(0xF5, "SHR", O::Int, N, 1, {3}, rotate<Rot::Shr>);
    op(0xF6, "SHL", O::A, N, 1, {2}, rotate<Rot::Shl>);
    op(0xF7, "SHL", O::Int, N, 1, {3}, rotate<Rot::Shl>);
    op(0xF8, "MV", O::ExtInd, O::Int, 1, {11, 13}, mv);
    op(0xF9, "MVW", O::ExtInd, O::Int, 2, {12, 14}, mv);
    op(0xFA, "MVP", O::ExtInd, O::Int, 3, {13, 15}, mv);
    op(0xFB, "MVL", O::ExtInd, O::Int, 0, {10, 12}, mvl);
    op(0xFC, "DSRL", O::Int, N, 0, {4}, dsrl);
    op(0xFD, "MV", O::RegHi, O::RegLo, 0, {2}, mv);
    op(0xFE, "IR", N, N, 0, {12}, ir);
    op(0xFF, "RESET", N, N, 0, {6}, reset);

    return t;
}

} // namespace

const InstrMeta &opcodeInfo(uint8_t opcode) {
    static const std::array<InstrMeta, 256> table = buildTable();
    return table[opcode];
}
