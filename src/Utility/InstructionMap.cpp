/*
 * InstructionMap.cpp
 *
 *  Created on: 18 Jan 2010
 *      Author: db434
 */

#include "InstructionMap.h"

// Need to define static class members
std::map<short, int> InstructionMap::oti; // opcode to instruction
std::map<std::string, short> InstructionMap::nto; // name to opcode

/* Returns the instruction identifier of the given opcode */
short InstructionMap::operation(short opcode) {

  InstructionMap::initialise();
  return (short)(InstructionMap::oti[opcode]);

}

/* Return the opcode of the given instruction name */
short InstructionMap::opcode(std::string& name) {

  InstructionMap::initialise();
  return InstructionMap::nto[name];

}

/* Return whether the instruction contains an immediate value */
bool InstructionMap::hasImmediate(short op) {
  /*
   * Instructions with immediates:
   *  LD, LDB, ST, STB, STADDR, STBADDR,
   *  SLL, SRL, SRA,
   *  SEQI, SNEI, SLTI, SLTIU, LUI
   *  NORI, ANDI, ORI, XORI
   *  ADDUI, RSUBI
   *  FETCH, FETCHPST, RMTFETCH, RMTFETCHPST, IBJMP, RMTFILL
   */

  return (op>=LD && op<=SRA) || (op>=SEQI && op<=LUI) || (op>=NORI && op<=XORI)
      || (op == RSUBI) || (op == ADDUI) || (op>=IBJMP && op<=RMTFILL);

}

/* Return whether the instruction specifies a remote channel */
bool InstructionMap::hasRemoteChannel(short op) {
  /*
   * Instructions with remote channels:
   *  LD, LDB, ST, STB, STADDR, STBADDR,
   *  RMTFETCH, RMTFETCHPST, RMTFILL, RMTEXECUTE, RMTNXIPK
   */

  // For Version 1.0, allow all instructions to specify remote channels
  return isALUOperation(op) || (op == TSTCH) || (op == SELCH);//(op>=LD && op<=STBADDR) || (op>=RMTFETCH && op<=RMTNXIPK);
}

/* Return whether the operation requires use of the ALU */
bool InstructionMap::isALUOperation(short op) {
  /*
   * Instructions which DON'T use the ALU:
   * LD, LDB, ST, STB, STADDR, ATBADDR,   // Not sure about these
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

  short a;  // Makes consistency easier below

  a=0;    nto["nop"] = a;           oti[a] = NOP;

  a++;    nto["ld"] = a;            oti[a] = LD;
  a++;    nto["ldb"] = a;           oti[a] = LDB;
  a++;    nto["st"] = a;            oti[a] = ST;
  a++;    nto["stb"] = a;           oti[a] = STB;
  a++;    nto["staddr"] = a;        oti[a] = STADDR;
  a++;    nto["stbaddr"] = a;       oti[a] = STBADDR;

  a++;    nto["sll"] = a;           oti[a] = SLL;
  a++;    nto["srl"] = a;           oti[a] = SRL;
  a++;    nto["sra"] = a;           oti[a] = SRA;
  a++;    nto["sllv"] = a;          oti[a] = SLLV;
  a++;    nto["srlv"] = a;          oti[a] = SRLV;
  a++;    nto["srav"] = a;          oti[a] = SRAV;

  a++;    nto["seq"] = a;           oti[a] = SEQ;
  a++;    nto["sne"] = a;           oti[a] = SNE;
  a++;    nto["slt"] = a;           oti[a] = SLT;
  a++;    nto["sltu"] = a;          oti[a] = SLTU;
  a++;    nto["seqi"] = a;          oti[a] = SEQI;
  a++;    nto["snei"] = a;          oti[a] = SNEI;
  a++;    nto["slti"] = a;          oti[a] = SLTI;
  a++;    nto["sltiu"] = a;         oti[a] = SLTIU;

  a++;    nto["lui"] = a;           oti[a] = LUI;

  a++;    nto["psel"] = a;          oti[a] = PSEL;

  a++;    nto["clz"] = a;           oti[a] = CLZ;

  a++;    nto["nor"] = a;           oti[a] = NOR;
  a++;    nto["nori"] = a;          oti[a] = NORI;
  a++;    nto["and"] = a;           oti[a] = AND;
  a++;    nto["andi"] = a;          oti[a] = ANDI;
  a++;    nto["or"] = a;            oti[a] = OR;
  a++;    nto["ori"] = a;           oti[a] = ORI;
  a++;    nto["xor"] = a;           oti[a] = XOR;
  a++;    nto["xori"] = a;          oti[a] = XORI;

  a++;    nto["nand"] = a;          oti[a] = NAND;
  a++;    nto["clr"] = a;           oti[a] = CLR;
  a++;    nto["orc"] = a;           oti[a] = ORC;
  a++;    nto["popc"] = a;          oti[a] = POPC;
  a++;    nto["rsubi"] = a;         oti[a] = RSUBI;

  a++;    nto["addu"] = a;          oti[a] = ADDU;
  a++;    nto["addui"] = a;         oti[a] = ADDUI;
  a++;    nto["subu"] = a;          oti[a] = SUBU;
  a++;    nto["mulhw"] = a;         oti[a] = MULHW;
  a++;    nto["mullw"] = a;         oti[a] = MULLW;
  a++;    nto["mulhwu"] = a;        oti[a] = MULHWU;

  a++;    nto["irdr"] = a;          oti[a] = IRDR;
  a++;    nto["iwtr"] = a;          oti[a] = IWTR;

  a++;    nto["woche"] = a;         oti[a] = WOCHE;
  a++;    nto["tstch"] = a;         oti[a] = TSTCH;
  a++;    nto["selch"] = a;         oti[a] = SELCH;

  a++;    nto["setfetchch"] = a;    oti[a] = SETFETCHCH;
  a++;    nto["ibjmp"] = a;         oti[a] = IBJMP;
  a++;    nto["fetch"] = a;         oti[a] = FETCH;
  a++;    nto["psel.fetch"] = a;    oti[a] = PSELFETCH;
  a++;    nto["fetchpst"] = a;      oti[a] = FETCHPST;
  a++;    nto["rmtfetch"] = a;      oti[a] = RMTFETCH;
  a++;    nto["rmtfetchpst"] = a;   oti[a] = RMTFETCHPST;
  a++;    nto["rmtfill"] = a;       oti[a] = RMTFILL;
  a++;    nto["rmtexecute"] = a;    oti[a] = RMTEXECUTE;
  a++;    nto["rmtnxipk"] = a;      oti[a] = RMTNXIPK;


  initialised = true;

}
