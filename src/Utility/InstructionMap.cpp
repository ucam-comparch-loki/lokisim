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

  a=1;    nto["ld"] = a;            oti[a] = LD;
  a=2;    nto["ldb"] = a;           oti[a] = LDB;
  a=3;    nto["st"] = a;            oti[a] = ST;
  a=4;    nto["stb"] = a;           oti[a] = STB;
  a=5;    nto["staddr"] = a;        oti[a] = STADDR;
  a=6;    nto["stbaddr"] = a;       oti[a] = STBADDR;

  a=7;    nto["sll"] = a;           oti[a] = SLL;
  a=8;    nto["srl"] = a;           oti[a] = SRL;
  a=9;    nto["sra"] = a;           oti[a] = SRA;
  a=10;   nto["sllv"] = a;          oti[a] = SLLV;
  a=11;   nto["srlv"] = a;          oti[a] = SRLV;
  a=12;   nto["srav"] = a;          oti[a] = SRAV;

  a=13;   nto["seq"] = a;           oti[a] = SEQ;
  a=14;   nto["sne"] = a;           oti[a] = SNE;
  a=15;   nto["slt"] = a;           oti[a] = SLT;
  a=16;   nto["sltu"] = a;          oti[a] = SLTU;
  a=17;   nto["seqi"] = a;          oti[a] = SEQI;
  a=18;   nto["snei"] = a;          oti[a] = SNEI;
  a=19;   nto["slti"] = a;          oti[a] = SLTI;
  a=20;   nto["sltiu"] = a;         oti[a] = SLTIU;

  a=21;   nto["lui"] = a;           oti[a] = LUI;

  a=22;   nto["psel"] = a;          oti[a] = PSEL;

  a=23;   nto["clz"] = a;           oti[a] = CLZ;

  a=24;   nto["nor"] = a;           oti[a] = NOR;
  a=25;   nto["nori"] = a;          oti[a] = NORI;
  a=26;   nto["and"] = a;           oti[a] = AND;
  a=27;   nto["andi"] = a;          oti[a] = ANDI;
  a=28;   nto["or"] = a;            oti[a] = OR;
  a=29;   nto["ori"] = a;           oti[a] = ORI;
  a=30;   nto["xor"] = a;           oti[a] = XOR;
  a=31;   nto["xori"] = a;          oti[a] = XORI;

  a=32;   nto["nand"] = a;          oti[a] = NAND;
  a=33;   nto["clr"] = a;           oti[a] = CLR;
  a=34;   nto["orc"] = a;           oti[a] = ORC;
  a=35;   nto["popc"] = a;          oti[a] = POPC;
  a=36;   nto["rsubi"] = a;         oti[a] = RSUBI;

  a=37;   nto["addu"] = a;          oti[a] = ADDU;
  a=38;   nto["addui"] = a;         oti[a] = ADDUI;
  a=39;   nto["subu"] = a;          oti[a] = SUBU;
  a=40;   nto["mulhw"] = a;         oti[a] = MULHW;
  a=41;   nto["mullw"] = a;         oti[a] = MULLW;
  a=42;   nto["mulhwu"] = a;        oti[a] = MULHWU;

  a=43;   nto["irdr"] = a;          oti[a] = IRDR;
  a=44;   nto["iwtr"] = a;          oti[a] = IWTR;

  a=45;   nto["woche"] = a;         oti[a] = WOCHE;
  a=46;   nto["tstch"] = a;         oti[a] = TSTCH;
  a=47;   nto["selch"] = a;         oti[a] = SELCH;

  a=48;   nto["setfetchch"] = a;    oti[a] = SETFETCHCH;
  a=49;   nto["ibjmp"] = a;         oti[a] = IBJMP;
  a=50;   nto["fetch"] = a;         oti[a] = FETCH;
  a=51;   nto["fetchpst"] = a;      oti[a] = FETCHPST;
  a=52;   nto["rmtfetch"] = a;      oti[a] = RMTFETCH;
  a=53;   nto["rmtfetchpst"] = a;   oti[a] = RMTFETCHPST;
  a=54;   nto["rmtfill"] = a;       oti[a] = RMTFILL;
  a=55;   nto["rmtexecute"] = a;    oti[a] = RMTEXECUTE;
  a=56;   nto["rmtnxipk"] = a;      oti[a] = RMTNXIPK;


  initialised = true;

}
