/*
 * InstructionMap.cpp
 *
 *  Created on: 18 Jan 2010
 *      Author: db434
 */

#include "InstructionMap.h"
#include <iostream>

// Need to define static class members
std::map<inst_name_t, opcode_t> InstructionMap::nto; // name to opcode
std::map<opcode_t, operation_t> InstructionMap::oti; // opcode to instruction
std::map<operation_t, inst_name_t> InstructionMap::itn; // instruction to name

/* Return the opcode of the given instruction name */
opcode_t InstructionMap::opcode(const inst_name_t& name) {
  InstructionMap::initialise();
  if(nto.find(name) == nto.end()) {
    std::cerr << "Error: unknown instruction: " << name << std::endl;
    throw std::exception();
  }
  else return nto[name];
}

/* Returns the instruction identifier of the given opcode */
operation_t InstructionMap::operation(opcode_t opcode) {
  InstructionMap::initialise();
  if(oti.find(opcode) == oti.end()) throw std::exception();
  else return oti[opcode];
}

const inst_name_t& InstructionMap::name(operation_t operation) {
  InstructionMap::initialise();
  if(itn.find(operation) == itn.end()) throw std::exception();
  else return itn[operation];
}

int InstructionMap::numInstructions() {
  return SYSCALL + 1;
}

/* Return whether the instruction has a register which it stores its result in. */
bool InstructionMap::hasDestReg(operation_t op) {
  return storesResult(op);
}

/* Return whether the instruction uses the first source register. */
bool InstructionMap::hasSrcReg1(operation_t op) {
  static const operation_t withoutSource1[] = {
      NOP, LUI, SELCH, IBJMP, RMTEXECUTE, NXIPK, SYSCALL
  };

  static const std::vector<bool>& hasSource1 =
      bitVector(true, withoutSource1, sizeof(withoutSource1)/sizeof(operation_t));

  return hasSource1[op];
}

/* Return whether the instruction uses the second source register. */
bool InstructionMap::hasSrcReg2(operation_t op) {
  static const operation_t withSource2[] = {
      STW, STHW, STB,
      SLL, SRL, SRA,
      SETEQ, SETNE, SETLT, SETLTU, SETGTE, SETGTEU,
      PSEL, NOR, AND, OR, XOR,
      ADDU, SUBU, MULHW, MULLW, MULHWU,
      PSELFETCH
  };

  static const std::vector<bool>& hasSource2 =
      bitVector(false, withSource2, sizeof(withSource2)/sizeof(operation_t));

  return hasSource2[op];
}

/* Return whether the instruction contains an immediate value */
bool InstructionMap::hasImmediate(operation_t op) {
  static const operation_t withImmed[] = {
      LDW, LDHWU, LDBU, STW, STHW, STB,
      SLLI, SRLI, SRAI,
      SETEQI, SETNEI, SETLTI, SETLTUI, SETGTEI, SETGTEUI, LUI,
      NORI, ANDI, ORI, XORI,
      ADDUI,
      FETCH, FETCHPST, IBJMP, FILL,
      SETCHMAP, SYSCALL
  };

  static const std::vector<bool>& hasImmed =
      bitVector(false, withImmed, sizeof(withImmed)/sizeof(operation_t));

  return hasImmed[op];
}

/* Return whether the instruction specifies a remote channel */
bool InstructionMap::hasRemoteChannel(operation_t op) {
  // Since so many instructions may send their results onto the network, we
  // only list the ones which don't here.
  // Note that the number of instructions with remote channels may decrease
  // when we finalise an instruction encoding.
  static const operation_t noChannel[] = {
      NOP,
      WOCHE, TSTCH, SELCH,
      IBJMP, FETCH, PSELFETCH, FETCHPST, SETCHMAP,
      SYSCALL
  };

  static const std::vector<bool>& hasChannel =
      bitVector(true, noChannel, sizeof(noChannel)/sizeof(operation_t));

  return hasChannel[op];
}

/* Return whether the operation requires use of the ALU */
bool InstructionMap::isALUOperation(operation_t op) {
  // Since there are far more instructions which use the ALU than which don't,
  // we only list the ones that don't here.
  static const operation_t notALU[] = {
      /*LDW, LDBU, STW, STHW, STB,*/
      WOCHE, TSTCH, SELCH,
      IBJMP, /*FETCH, PSELFETCH, FETCHPST,
      FILL,*/ RMTEXECUTE, NXIPK, SETCHMAP, SYSCALL
  };

  static const std::vector<bool>& useALU =
      bitVector(true, notALU, sizeof(notALU)/sizeof(operation_t));

  return useALU[op];
}

/* Return whether the operation stores its result. */
bool InstructionMap::storesResult(operation_t op) {
  // Since there are far more instructions which store their results than which
  // don't, we only list the ones that don't here.
  static const operation_t doesntStore[] = {
      NOP, LDW, LDHWU, LDBU, STW, STHW, STB,
      WOCHE,
      IBJMP, FETCH, PSELFETCH, FETCHPST,
      FILL, RMTEXECUTE, NXIPK, SETCHMAP, SYSCALL
  };

  static const std::vector<bool>& stores =
      bitVector(true, doesntStore, sizeof(doesntStore)/sizeof(operation_t));

  return stores[op];
}


/* Put all of the mappings into the data structures */
void InstructionMap::initialise() {
  static bool initialised = false;
  if(initialised) return;

  // Keeping track of the opcode gives us more control below. It allows us to
  // decide that some opcodes should be skipped, allowing better alignment.
  short a;

  a=0;    addToMaps("nop", a, NOP);

  a++;    addToMaps("ldw", a, LDW);
  a++;    addToMaps("ldhwu", a, LDHWU);
  a++;    addToMaps("ldbu", a, LDBU);
  a++;    addToMaps("stw", a, STW);
  a++;    addToMaps("sthw", a, STHW);
  a++;    addToMaps("stb", a, STB);
  a++;    addToMaps("stwaddr", a, STWADDR);
  a++;    addToMaps("stbaddr", a, STBADDR);

  a++;    addToMaps("ldvec", a, LDVECTOR);
  a++;    addToMaps("ldstream", a, LDSTREAM);
  a++;    addToMaps("stvec", a, STVECTOR);
  a++;    addToMaps("ststream", a, STSTREAM);

  a++;    addToMaps("sll", a, SLL);
  a++;    addToMaps("srl", a, SRL);
  a++;    addToMaps("sra", a, SRA);
  a++;    addToMaps("slli", a, SLLI);
  a++;    addToMaps("srli", a, SRLI);
  a++;    addToMaps("srai", a, SRAI);

  a++;    addToMaps("seteq", a, SETEQ);
  a++;    addToMaps("setne", a, SETNE);
  a++;    addToMaps("setlt", a, SETLT);
  a++;    addToMaps("setltu", a, SETLTU);
  a++;    addToMaps("setgte", a, SETGTE);
  a++;    addToMaps("setgteu", a, SETGTEU);
  a++;    addToMaps("seteqi", a, SETEQI);
  a++;    addToMaps("setnei", a, SETNEI);
  a++;    addToMaps("setlti", a, SETLTI);
  a++;    addToMaps("setltui", a, SETLTUI);
  a++;    addToMaps("setgtei", a, SETGTEI);
  a++;    addToMaps("setgteui", a, SETGTEUI);

  a++;    addToMaps("lui", a, LUI);

  a++;    addToMaps("psel", a, PSEL);

  a++;    addToMaps("clz", a, CLZ);

  a++;    addToMaps("nor", a, NOR);
  a++;    addToMaps("and", a, AND);
  a++;    addToMaps("or", a, OR);
  a++;    addToMaps("xor", a, XOR);
  a++;    addToMaps("nori", a, NORI);
  a++;    addToMaps("andi", a, ANDI);
  a++;    addToMaps("ori", a, ORI);
  a++;    addToMaps("xori", a, XORI);

  a++;    addToMaps("nand", a, NAND);
  a++;    addToMaps("clr", a, CLR);
  a++;    addToMaps("orc", a, ORC);
  a++;    addToMaps("popc", a, POPC);
  a++;    addToMaps("rsubi", a, RSUBI);

  a++;    addToMaps("addu", a, ADDU);
  a++;    addToMaps("addui", a, ADDUI);
  a++;    addToMaps("subu", a, SUBU);
  a++;    addToMaps("mulhw", a, MULHW);
  a++;    addToMaps("mullw", a, MULLW);
  a++;    addToMaps("mulhwu", a, MULHWU);

  a++;    addToMaps("irdr", a, IRDR);
  a++;    addToMaps("iwtr", a, IWTR);

  a++;    addToMaps("woche", a, WOCHE);
  a++;    addToMaps("tstch", a, TSTCH);
  a++;    addToMaps("selch", a, SELCH);

  a++;    addToMaps("setfetchch", a, SETFETCHCH);
  a++;    addToMaps("ibjmp", a, IBJMP);
  a++;    addToMaps("fetch", a, FETCH);
  a++;    addToMaps("psel.fetch", a, PSELFETCH);
  a++;    addToMaps("fetchpst", a, FETCHPST);
  a++;    addToMaps("rmtfetch", a, RMTFETCH);
  a++;    addToMaps("rmtfetchpst", a, RMTFETCHPST);
  a++;    addToMaps("fill", a, FILL);
  a++;    addToMaps("rmtexecute", a, RMTEXECUTE);
  a++;    addToMaps("nxipk", a, NXIPK);

  a++;    addToMaps("setchmap", a, SETCHMAP);

  a++;    addToMaps("syscall", a, SYSCALL);


  initialised = true;

}

void InstructionMap::addToMaps(const inst_name_t& name,
                               opcode_t opcode,
                               operation_t instruction) {
  nto[name] = opcode;
  oti[opcode] = instruction;
  itn[instruction] = name;
}

const std::vector<bool>& InstructionMap::bitVector(bool defaultVal, const operation_t exceptions[], int numExceptions) {
  // Generate a new vector, with a space for every instruction, and the
  // default value set.
  std::vector<bool>* vec = new std::vector<bool>(numInstructions(), defaultVal);

  // Flip the boolean value for all exceptions.
  for(int i=0; i<numExceptions; i++) vec->at(exceptions[i]) = !defaultVal;

  return *vec;
}
