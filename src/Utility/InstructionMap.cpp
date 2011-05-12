/*
 * InstructionMap.cpp
 *
 *  Created on: 18 Jan 2010
 *      Author: db434
 */

#include "InstructionMap.h"
#include <set>

// Need to define static class members
std::map<std::string, short> InstructionMap::nto; // name to opcode
std::map<short, int> InstructionMap::oti; // opcode to instruction
std::map<int, std::string> InstructionMap::itn; // instruction to name

/* Return the opcode of the given instruction name */
short InstructionMap::opcode(const std::string& name) {
  InstructionMap::initialise();
  if(nto.find(name) == nto.end()) throw std::exception();
  else return nto[name];
}

/* Returns the instruction identifier of the given opcode */
short InstructionMap::operation(short opcode) {
  InstructionMap::initialise();
  if(oti.find(opcode) == oti.end()) throw std::exception();
  else return (short)(oti[opcode]);
}

std::string& InstructionMap::name(int operation) {
  InstructionMap::initialise();
  if(itn.find(operation) == itn.end()) throw std::exception();
  else return itn[operation];
}

/* Return whether the instruction has a register which it stores its result in. */
bool InstructionMap::hasDestReg(short op) {
  return storesResult(op);
}

/* Return whether the instruction uses the first source register. */
bool InstructionMap::hasSrcReg1(short op) {
  static const short withoutSource1[] = {
      NOP, LUI, SELCH, IBJMP, RMTEXECUTE, RMTNXIPK, SYSCALL
  };

  static const std::set<short> ops(withoutSource1, withoutSource1+7);

  // The operation has a source register if it isn't in the set.
  return ops.find(op) == ops.end();
}

/* Return whether the instruction uses the second source register. */
bool InstructionMap::hasSrcReg2(short op) {
  static const short withSource2[] = {
      STW, STHW, STB,
      SLL, SRL, SRA,
      SETEQ, SETNE, SETLT, SETLTU, SETGTE, SETGTEU,
      PSEL, NOR, AND, OR, XOR, NAND, CLR, ORC,
      ADDU, SUBU, MULHW, MULLW, MULHWU
  };

  static const std::set<short> ops(withSource2, withSource2+25);

  // The operation has a source register if it is in the set.
  return ops.find(op) != ops.end();
}

/* Return whether the instruction contains an immediate value */
bool InstructionMap::hasImmediate(short op) {
  static const short withImmed[] = {
      LDW, LDHWU, LDBU, STW, STHW, STB, STWADDR, STBADDR,
      CFGMEM,
      SLLI, SRLI, SRAI,
      SETEQI, SETNEI, SETLTI, SETLTUI, SETGTEUI, LUI,
      NORI, ANDI, ORI, XORI,
      ADDUI, RSUBI,
      FETCH, FETCHPST, RMTFETCH, RMTFETCHPST, IBJMP, RMTFILL,
      SETCHMAP, SYSCALL
  };

  static const std::set<short> ops(withImmed, withImmed+31);

  // The operation has an immediate if it is in the set.
  return ops.find(op) != ops.end();
}

/* Return whether the instruction specifies a remote channel */
bool InstructionMap::hasRemoteChannel(short op) {
  // Since so many instructions may send their results onto the network, we
  // only list the ones which don't here.
  // Note that the number of instructions with remote channels may decrease
  // when we finalise an instruction encoding.
  static const short noChannel[] = {
      NOP,
      CFGMEM,
      WOCHE, TSTCH, SELCH,
      SETFETCHCH, IBJMP, FETCH, PSELFETCH, FETCHPST, SETCHMAP,
      SYSCALL
  };

  static const std::set<short> ops(noChannel, noChannel+11);

  // The operation has a remote channel if it isn't in the set.
  return ops.find(op) == ops.end();
}

/* Return whether the operation requires use of the ALU */
bool InstructionMap::isALUOperation(short op) {
  // Since there are far more instructions which use the ALU than which don't,
  // we only list the ones that don't here.
  static const short notALU[] = {
      /*LDW, LDBU,*/ STW, STHW, STB, STWADDR, STBADDR,
      CFGMEM,
      WOCHE, TSTCH, SELCH,
      SETFETCHCH, IBJMP, FETCH, PSELFETCH, FETCHPST, RMTFETCH, RMTFETCHPST,
      RMTFILL, RMTEXECUTE, RMTNXIPK, SETCHMAP, SYSCALL
  };

  static const std::set<short> ops(notALU, notALU+17);

  // The operation is an ALU operation if it isn't in the set.
  return ops.find(op) == ops.end();
}

/* Return whether the operation stores its result. */
bool InstructionMap::storesResult(short op) {
  // Since there are far more instructions which store their results than which
  // don't, we only list the ones that don't here.
  static const short doesntStore[] = {
      NOP, LDW, LDHWU, LDBU, STW, STHW, STB, STWADDR, STBADDR,
      CFGMEM,
      WOCHE,
      SETFETCHCH, IBJMP, FETCH, PSELFETCH, FETCHPST, RMTFETCH, RMTFETCHPST,
      RMTFILL, RMTEXECUTE, RMTNXIPK, SETCHMAP, SYSCALL
  };

  static const std::set<short> ops(doesntStore, doesntStore+22);

  // The operation stores its result if it isn't in the set.
  return ops.find(op) == ops.end();
}


/* Put all of the mappings into the data structures */
void InstructionMap::initialise() {

  static bool initialised = false;
  if(initialised) return;

  // Could make a a static variable in addToMaps
  short a;  // Makes consistency easier below

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
  a++;    addToMaps("ldstr", a, LDSTREAM);
  a++;    addToMaps("stvec", a, STVECTOR);
  a++;    addToMaps("ststr", a, STSTREAM);

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
  a++;    addToMaps("rmtfill", a, RMTFILL);
  a++;    addToMaps("rmtexecute", a, RMTEXECUTE);
  a++;    addToMaps("rmtnxipk", a, RMTNXIPK);

  a++;    addToMaps("cfgmem", a, CFGMEM);
  a++;    addToMaps("setchmap", a, SETCHMAP);

  a++;    addToMaps("syscall", a, SYSCALL);


  initialised = true;

}

void InstructionMap::addToMaps(const std::string& name, short opcode, int instruction) {
  nto[name] = opcode;
  oti[opcode] = instruction;
  itn[instruction] = name;
}
