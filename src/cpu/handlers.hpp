#pragma once

// Instruction handlers referenced by the opcode table (internal to the CPU core).
//   handlers_data.cpp    - moves, exchanges, block moves, stacks
//   handlers_alu.cpp     - arithmetic, logic, BCD, shifts/rotates
//   handlers_control.cpp - jumps, calls, returns, interrupts, misc

class Instruction;
class SC62015;

namespace handlers {

// Data movement
void mv(Instruction &, SC62015 &);
void ex(Instruction &, SC62015 &);
void mvl(Instruction &, SC62015 &);
void mvld(Instruction &, SC62015 &);
void exl(Instruction &, SC62015 &);
void pushu(Instruction &, SC62015 &);
void popu(Instruction &, SC62015 &);
void pushs(Instruction &, SC62015 &);
void pops(Instruction &, SC62015 &);

// Arithmetic / logic

// Template wrappers for a more concise opcode table definition.
enum class Alu { Add, Sub, Adc, Sbc, Cmp, And, Or, Xor, Test, Pmdf };
enum class Rot { Ror, Rol, Shr, Shl };
enum class Loop { Adcl, Sbcl, Dadl, Dsbl }; // multi-byte arithmetic over I bytes
template <Alu op> void alu(Instruction &in, SC62015 &cpu) { aluOp(op, in, cpu); }
template <Rot op> void rotate(Instruction &in, SC62015 &cpu) { rotateOp(op, in, cpu); }
template <Loop op> void loop(Instruction &in, SC62015 &cpu) { loopOp(op, in, cpu); }

void aluOp(Alu op, Instruction &, SC62015 &);
void rotateOp(Rot op, Instruction &, SC62015 &);
void loopOp(Loop op, Instruction &, SC62015 &);

void inc(Instruction &, SC62015 &);
void dec(Instruction &, SC62015 &);
void dsll(Instruction &, SC62015 &);
void dsrl(Instruction &, SC62015 &);
void swap(Instruction &, SC62015 &);
void sc(Instruction &, SC62015 &);
void rc(Instruction &, SC62015 &);

// Control flow and system
void nop(Instruction &, SC62015 &);
void jp(Instruction &, SC62015 &);
void jpcc(Instruction &, SC62015 &);
void jr(Instruction &, SC62015 &);
void jrcc(Instruction &, SC62015 &);
void call(Instruction &, SC62015 &);
void ret(Instruction &, SC62015 &);
void retf(Instruction &, SC62015 &);
void reti(Instruction &, SC62015 &);
void halt(Instruction &, SC62015 &);
void off(Instruction &, SC62015 &);
void tcl(Instruction &, SC62015 &);
void wait(Instruction &, SC62015 &);
void ir(Instruction &, SC62015 &);
void reset(Instruction &, SC62015 &);

} // namespace handlers
