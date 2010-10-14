/*
 * InstructionMap.cpp
 *
 *  Created on: 18 Jan 2010
 *      Author: db434
 */

#include "InstructionMap.h"

// Need to define static class members
std::map<std::string, short> InstructionMap::nto; // name to opcode
std::map<short, int> InstructionMap::oti; // opcode to instruction
std::map<int, std::string> InstructionMap::itn; // instruction to name

/* Return the opcode of the given instruction name */
short InstructionMap::opcode(std::string& name) {
  InstructionMap::initialise();
  return nto[name];
}

/* Returns the instruction identifier of the given opcode */
short InstructionMap::operation(short opcode) {
  InstructionMap::initialise();
  return (short)(oti[opcode]);
}

std::string& InstructionMap::name(int operation) {
  InstructionMap::initialise();
  return itn[operation];
}

/* Return whether the instruction contains an immediate value */
bool InstructionMap::hasImmediate(short op) {
  /*
   * Instructions with immediates:
   *  LDW, LDB, STW, STB, STWADDR, STBADDR,
   *  SLLI, SRLI, SRAI,
   *  SETEQI, SETNEI, SETLTI, SETLTUI, SETGTEUI, LUI
   *  NORI, ANDI, ORI, XORI
   *  ADDUI, RSUBI
   *  FETCH, FETCHPST, RMTFETCH, RMTFETCHPST, IBJMP, RMTFILL
   */

  return (op>=LDW && op<=SRAI) || (op>=SETEQI && op<=LUI) || (op>=NORI && op<=XORI)
      || (op == RSUBI) || (op == ADDUI) || (op>=IBJMP && op<=RMTFILL);

}

/* Return whether the instruction specifies a remote channel */
bool InstructionMap::hasRemoteChannel(short op) {
  /*
   * Instructions with remote channels:
   *  LDW, LDB, STW, STB, STWADDR, STBADDR,
   *  RMTFETCH, RMTFETCHPST, RMTFILL, RMTEXECUTE, RMTNXIPK
   */

  // For Version 1.0, allow all instructions to specify remote channels
  return isALUOperation(op) || (op == TSTCH) || (op == SELCH);//(op>=LD && op<=STBADDR) || (op>=RMTFETCH && op<=RMTNXIPK);
}

/* Return whether the operation requires use of the ALU */
bool InstructionMap::isALUOperation(short op) {
  /*
   * Instructions which DON'T use the ALU:
   * LDW, LDB, STW, STB, STWADDR, STBADDR,   // Not sure about these
   * WOCHE, TSTCH, SELCH
   * SETFETCHCH, IBJMP, FETCH, FETCHPST, RMTFETCH, RMTFETCHPST, RMTFILL,
   * RMTEXECUTE, RMTNXIPK
   */

  return !(op>=WOCHE && op<=RMTNXIPK);
}


/* Put all of the mappings into the data structures */
void InstructionMap::initialise() {

  static bool initialised = false;
  if(initialised) return;

  // Could make a a static variable in addToMaps
  short a;  // Makes consistency easier below

  a=0;    addToMaps("nop", a, NOP);

  a++;    addToMaps("ldw", a, LDW);
  a++;    addToMaps("ldb", a, LDB);
  a++;    addToMaps("stw", a, STW);
  a++;    addToMaps("stb", a, STB);
  a++;    addToMaps("stwaddr", a, STWADDR);
  a++;    addToMaps("stbaddr", a, STBADDR);

  a++;    addToMaps("slli", a, SLLI);
  a++;    addToMaps("srli", a, SRLI);
  a++;    addToMaps("srai", a, SRAI);
  a++;    addToMaps("sll", a, SLL);
  a++;    addToMaps("srl", a, SRL);
  a++;    addToMaps("sra", a, SRA);

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
  a++;    addToMaps("setgteui", a, SETGTEUI);

  a++;    addToMaps("lui", a, LUI);

  a++;    addToMaps("psel", a, PSEL);

  a++;    addToMaps("clz", a, CLZ);

  a++;    addToMaps("nor", a, NOR);
  a++;    addToMaps("nori", a, NORI);
  a++;    addToMaps("and", a, AND);
  a++;    addToMaps("andi", a, ANDI);
  a++;    addToMaps("or", a, OR);
  a++;    addToMaps("ori", a, ORI);
  a++;    addToMaps("xor", a, XOR);
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

  a++;    addToMaps("setchmap", a, SETCHMAP);


  initialised = true;

}

void InstructionMap::addToMaps(std::string name, short opcode, int instruction) {
  nto[name] = opcode;
  oti[opcode] = instruction;
  itn[instruction] = name;
}
