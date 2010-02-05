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

  return (op>=LD && op<=STBADDR) || (op>=RMTFETCH && op<=RMTNXIPK);
}

/* Put all of the mappings into the data structures */
void InstructionMap::initialise() {

  static bool initialised = false;
  if(initialised) return;

  short a;  // Makes consistency easier below

  a=-1;   nto["nop"] = a;           oti[a] = InstructionMap::NOP;

  a=0;    nto["ld"] = a;            oti[a] = InstructionMap::LD;
  a=1;    nto["ldb"] = a;           oti[a] = InstructionMap::LDB;
  a=2;    nto["st"] = a;            oti[a] = InstructionMap::ST;
  a=3;    nto["stb"] = a;           oti[a] = InstructionMap::STB;
  a=4;    nto["staddr"] = a;        oti[a] = InstructionMap::STADDR;
  a=5;    nto["stbaddr"] = a;       oti[a] = InstructionMap::STBADDR;

  a=6;    nto["sll"] = a;           oti[a] = InstructionMap::SLL;
  a=7;    nto["srl"] = a;           oti[a] = InstructionMap::SRL;
  a=8;    nto["sra"] = a;           oti[a] = InstructionMap::SRA;
  a=9;    nto["sllv"] = a;          oti[a] = InstructionMap::SLLV;
  a=10;   nto["srlv"] = a;          oti[a] = InstructionMap::SRLV;
  a=11;   nto["srav"] = a;          oti[a] = InstructionMap::SRAV;

  a=12;   nto["seq"] = a;           oti[a] = InstructionMap::SEQ;
  a=13;   nto["sne"] = a;           oti[a] = InstructionMap::SNE;
  a=14;   nto["slt"] = a;           oti[a] = InstructionMap::SLT;
  a=15;   nto["sltu"] = a;          oti[a] = InstructionMap::SLTU;
  a=16;   nto["seqi"] = a;          oti[a] = InstructionMap::SEQI;
  a=17;   nto["snei"] = a;          oti[a] = InstructionMap::SNEI;
  a=18;   nto["slti"] = a;          oti[a] = InstructionMap::SLTI;
  a=19;   nto["sltiu"] = a;         oti[a] = InstructionMap::SLTIU;

  a=20;   nto["lui"] = a;           oti[a] = InstructionMap::LUI;

  a=21;   nto["psel"] = a;          oti[a] = InstructionMap::PSEL;

  a=22;   nto["clz"] = a;           oti[a] = InstructionMap::CLZ;

  a=23;   nto["nor"] = a;           oti[a] = InstructionMap::NOR;
  a=24;   nto["nori"] = a;          oti[a] = InstructionMap::NORI;
  a=25;   nto["and"] = a;           oti[a] = InstructionMap::AND;
  a=26;   nto["andi"] = a;          oti[a] = InstructionMap::ANDI;
  a=27;   nto["or"] = a;            oti[a] = InstructionMap::OR;
  a=28;   nto["ori"] = a;           oti[a] = InstructionMap::ORI;
  a=29;   nto["xor"] = a;           oti[a] = InstructionMap::XOR;
  a=30;   nto["xori"] = a;          oti[a] = InstructionMap::XORI;

  a=31;   nto["nand"] = a;          oti[a] = InstructionMap::NAND;
  a=32;   nto["clr"] = a;           oti[a] = InstructionMap::CLR;
  a=33;   nto["orc"] = a;           oti[a] = InstructionMap::ORC;
  a=34;   nto["popc"] = a;          oti[a] = InstructionMap::POPC;
  a=35;   nto["rsubi"] = a;         oti[a] = InstructionMap::RSUBI;

  a=36;   nto["addu"] = a;          oti[a] = InstructionMap::ADDU;
  a=37;   nto["addui"] = a;         oti[a] = InstructionMap::ADDUI;
  a=38;   nto["subu"] = a;          oti[a] = InstructionMap::SUBU;
  a=39;   nto["mulhw"] = a;         oti[a] = InstructionMap::MULHW;
  a=40;   nto["mullw"] = a;         oti[a] = InstructionMap::MULLW;
  a=41;   nto["mulhwu"] = a;        oti[a] = InstructionMap::MULHWU;

  a=42;   nto["irdr"] = a;          oti[a] = InstructionMap::IRDR;
  a=43;   nto["iwtr"] = a;          oti[a] = InstructionMap::IWTR;

  a=44;   nto["woche"] = a;         oti[a] = InstructionMap::WOCHE;
  a=45;   nto["tstch"] = a;         oti[a] = InstructionMap::TSTCH;
  a=46;   nto["selch"] = a;         oti[a] = InstructionMap::SELCH;

  a=47;   nto["setfetchch"] = a;    oti[a] = InstructionMap::SETFETCHCH;
  a=48;   nto["fetch"] = a;         oti[a] = InstructionMap::FETCH;
  a=49;   nto["fetchpst"] = a;      oti[a] = InstructionMap::FETCHPST;
  a=50;   nto["rmtnxipk"] = a;      oti[a] = InstructionMap::RMTNXIPK;
  a=51;   nto["rmtfetch"] = a;      oti[a] = InstructionMap::RMTFETCH;
  a=52;   nto["rmtfetchpst"] = a;   oti[a] = InstructionMap::RMTFETCHPST;
  a=53;   nto["rmtexecute"] = a;    oti[a] = InstructionMap::RMTEXECUTE;
  a=54;   nto["ibjmp"] = a;         oti[a] = InstructionMap::IBJMP;
  a=55;   nto["rmtfill"] = a;       oti[a] = InstructionMap::RMTFILL;


  initialised = true;

}
