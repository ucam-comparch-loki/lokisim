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
  return isALUOperation(op);//(op>=LD && op<=STBADDR) || (op>=RMTFETCH && op<=RMTNXIPK);
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

  a=0;    nto["nop"] = a;           oti[a] = InstructionMap::NOP;

  a=1;    nto["ld"] = a;            oti[a] = InstructionMap::LD;
  a=2;    nto["ldb"] = a;           oti[a] = InstructionMap::LDB;
  a=3;    nto["st"] = a;            oti[a] = InstructionMap::ST;
  a=4;    nto["stb"] = a;           oti[a] = InstructionMap::STB;
  a=5;    nto["staddr"] = a;        oti[a] = InstructionMap::STADDR;
  a=6;    nto["stbaddr"] = a;       oti[a] = InstructionMap::STBADDR;

  a=7;    nto["sll"] = a;           oti[a] = InstructionMap::SLL;
  a=8;    nto["srl"] = a;           oti[a] = InstructionMap::SRL;
  a=9;    nto["sra"] = a;           oti[a] = InstructionMap::SRA;
  a=10;   nto["sllv"] = a;          oti[a] = InstructionMap::SLLV;
  a=11;   nto["srlv"] = a;          oti[a] = InstructionMap::SRLV;
  a=12;   nto["srav"] = a;          oti[a] = InstructionMap::SRAV;

  a=13;   nto["seq"] = a;           oti[a] = InstructionMap::SEQ;
  a=14;   nto["sne"] = a;           oti[a] = InstructionMap::SNE;
  a=15;   nto["slt"] = a;           oti[a] = InstructionMap::SLT;
  a=16;   nto["sltu"] = a;          oti[a] = InstructionMap::SLTU;
  a=17;   nto["seqi"] = a;          oti[a] = InstructionMap::SEQI;
  a=18;   nto["snei"] = a;          oti[a] = InstructionMap::SNEI;
  a=19;   nto["slti"] = a;          oti[a] = InstructionMap::SLTI;
  a=20;   nto["sltiu"] = a;         oti[a] = InstructionMap::SLTIU;

  a=21;   nto["lui"] = a;           oti[a] = InstructionMap::LUI;

  a=22;   nto["psel"] = a;          oti[a] = InstructionMap::PSEL;

  a=23;   nto["clz"] = a;           oti[a] = InstructionMap::CLZ;

  a=24;   nto["nor"] = a;           oti[a] = InstructionMap::NOR;
  a=25;   nto["nori"] = a;          oti[a] = InstructionMap::NORI;
  a=26;   nto["and"] = a;           oti[a] = InstructionMap::AND;
  a=27;   nto["andi"] = a;          oti[a] = InstructionMap::ANDI;
  a=28;   nto["or"] = a;            oti[a] = InstructionMap::OR;
  a=29;   nto["ori"] = a;           oti[a] = InstructionMap::ORI;
  a=30;   nto["xor"] = a;           oti[a] = InstructionMap::XOR;
  a=31;   nto["xori"] = a;          oti[a] = InstructionMap::XORI;

  a=32;   nto["nand"] = a;          oti[a] = InstructionMap::NAND;
  a=33;   nto["clr"] = a;           oti[a] = InstructionMap::CLR;
  a=34;   nto["orc"] = a;           oti[a] = InstructionMap::ORC;
  a=35;   nto["popc"] = a;          oti[a] = InstructionMap::POPC;
  a=36;   nto["rsubi"] = a;         oti[a] = InstructionMap::RSUBI;

  a=37;   nto["addu"] = a;          oti[a] = InstructionMap::ADDU;
  a=38;   nto["addui"] = a;         oti[a] = InstructionMap::ADDUI;
  a=39;   nto["subu"] = a;          oti[a] = InstructionMap::SUBU;
  a=40;   nto["mulhw"] = a;         oti[a] = InstructionMap::MULHW;
  a=41;   nto["mullw"] = a;         oti[a] = InstructionMap::MULLW;
  a=42;   nto["mulhwu"] = a;        oti[a] = InstructionMap::MULHWU;

  a=43;   nto["irdr"] = a;          oti[a] = InstructionMap::IRDR;
  a=44;   nto["iwtr"] = a;          oti[a] = InstructionMap::IWTR;

  a=45;   nto["woche"] = a;         oti[a] = InstructionMap::WOCHE;
  a=46;   nto["tstch"] = a;         oti[a] = InstructionMap::TSTCH;
  a=47;   nto["selch"] = a;         oti[a] = InstructionMap::SELCH;

  a=48;   nto["setfetchch"] = a;    oti[a] = InstructionMap::SETFETCHCH;
  a=49;   nto["fetch"] = a;         oti[a] = InstructionMap::FETCH;
  a=50;   nto["fetchpst"] = a;      oti[a] = InstructionMap::FETCHPST;
  a=51;   nto["rmtnxipk"] = a;      oti[a] = InstructionMap::RMTNXIPK;
  a=52;   nto["rmtfetch"] = a;      oti[a] = InstructionMap::RMTFETCH;
  a=53;   nto["rmtfetchpst"] = a;   oti[a] = InstructionMap::RMTFETCHPST;
  a=54;   nto["rmtexecute"] = a;    oti[a] = InstructionMap::RMTEXECUTE;
  a=55;   nto["ibjmp"] = a;         oti[a] = InstructionMap::IBJMP;
  a=56;   nto["rmtfill"] = a;       oti[a] = InstructionMap::RMTFILL;


  initialised = true;

}
