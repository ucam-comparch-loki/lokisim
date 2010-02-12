/*
 * ALU.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ALU.h"
#include "../../InstructionMap.h"

void ALU::doOp() {

  if(select.read() == InstructionMap::NOP) return;

  Data d1 = in1.read(), d2 = in2.read();

  if(DEBUG) std::cout<<"ALU received Data: "<<d1<<" and "<<d2<<"\n";

  unsigned int val1 = d1.getData();
  unsigned int val2 = d2.getData();
  unsigned int result;

  switch(select.read()) {

    case InstructionMap::SLL:
    case InstructionMap::SLLV: result = val1 << val2; break;
    case InstructionMap::SRL:
    case InstructionMap::SRLV: result = val1 >> val2; break;
    case InstructionMap::SRA:
    case InstructionMap::SRAV: result = (int)val1 >> (int)val2; break;

    case InstructionMap::SEQ:
    case InstructionMap::SEQI: result = (val1 == val2); break;
    case InstructionMap::SNE:
    case InstructionMap::SNEI: result = (val1 != val2); break;
    case InstructionMap::SLT:
    case InstructionMap::SLTI: result = ((int)val1 < (int)val2); break;
    case InstructionMap::SLTU:
    case InstructionMap::SLTIU: result = (val1 < val2); break;

    case InstructionMap::LUI:  result = val1 << 16; break;

//    case InstructionMap::PSEL: result = predicate ? val1 : val2; break;

//    case InstructionMap::CLZ: result = 32 - math.log(2, val1) + 1; break;

    case InstructionMap::NOR:
    case InstructionMap::NORI: result = ~(val1 | val2); break;
    case InstructionMap::AND:
    case InstructionMap::ANDI: result = val1 & val2; break;
    case InstructionMap::OR:
    case InstructionMap::ORI:  result = val1 | val2; break;
    case InstructionMap::XOR:
    case InstructionMap::XORI: result = val1 ^ val2; break;

    case InstructionMap::NAND: result = ~(val1 & val2); break;
    case InstructionMap::CLR:  result = val1 & ~val2; break;
    case InstructionMap::ORC:  result = val1 | ~val2; break;
//    case InstructionMap::POPC: result = ???; break;
    case InstructionMap::RSUBI: result = val2 - val1; break;

    case InstructionMap::ADDU:
    case InstructionMap::ADDUI: result = val1 + val2; break;
    case InstructionMap::SUBU: result = val1 - val2; break;
    case InstructionMap::MULHW: result = ((long)val1 * (long)val2) >> 32; break;
    case InstructionMap::MULLW: result = ((long)val1 * (long)val2) & -1; break;
//    case InstructionMap::MULHWU: result = ???; break;

    default: result = val1;// Is this a good default?
  }

  out.write(Data(result));

}

ALU::ALU(sc_core::sc_module_name name, int ID) : Component(name, ID) {

  SC_METHOD(doOp);
  sensitive << select /*<< in1 << in2*/;  // Would prefer to just have select
  dont_initialize();

}

ALU::~ALU() {

}
