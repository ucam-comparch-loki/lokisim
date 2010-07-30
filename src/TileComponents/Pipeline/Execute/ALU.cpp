/*
 * ALU.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ALU.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Datatype/Instruction.h"

void ALU::doOp() {

  if(operation.read() == InstructionMap::NOP) return;

  signed int val1 = (int)in1.read().getData();
  signed int val2 = (int)in2.read().getData();
  signed int result;

  if(DEBUG) cout << this->name() << ": executing " <<
    InstructionMap::name(operation.read())<<" on "<<val1<<" and "<<val2<<endl;

  switch(operation.read()) {

    case InstructionMap::SLL:
    case InstructionMap::SLLV:  result = val1 << val2; break;
    case InstructionMap::SRL:
    case InstructionMap::SRLV:  result = val1 >> val2; break;
    case InstructionMap::SRA:
    case InstructionMap::SRAV:  result = (int)val1 >> (int)val2; break;

    case InstructionMap::SEQ:
    case InstructionMap::SEQI:  result = (val1 == val2); break;
    case InstructionMap::SNE:
    case InstructionMap::SNEI:  result = (val1 != val2); break;
    case InstructionMap::SLT:
    case InstructionMap::SLTI:  result = ((int)val1 < (int)val2); break;
    case InstructionMap::SLTU:
    case InstructionMap::SLTIU: result = (val1 < val2); break;

    case InstructionMap::LUI:   result = val2 << 16; break;

    case InstructionMap::PSEL:  result = predicate.read() ? val1 : val2; break;

//    case InstructionMap::CLZ: result = 32 - math.log(2, val1) + 1; break;

    case InstructionMap::NOR:
    case InstructionMap::NORI:  result = ~(val1 | val2); break;
    case InstructionMap::AND:
    case InstructionMap::ANDI:  result = val1 & val2; break;
    case InstructionMap::OR:
    case InstructionMap::ORI:   result = val1 | val2; break;
    case InstructionMap::XOR:
    case InstructionMap::XORI:  result = val1 ^ val2; break;

    case InstructionMap::NAND:  result = ~(val1 & val2); break;
    case InstructionMap::CLR:   result = val1 & ~val2; break;
    case InstructionMap::ORC:   result = val1 | ~val2; break;
//    case InstructionMap::POPC: result = ???; break;
    case InstructionMap::RSUBI: result = val2 - val1; break;

    case InstructionMap::ADDU:
    case InstructionMap::ADDUI: result = val1 + val2; break;
    case InstructionMap::SUBU:  result = val1 - val2; break;
    case InstructionMap::MULHW: result = ((long)val1 * (long)val2) >> 32; break;
    case InstructionMap::MULLW: result = ((long)val1 * (long)val2) & -1; break;
//    case InstructionMap::MULHWU: result = ???; break;

    default: result = val1; // Is this a good default?
  }

  out.write(Data(result));

  if(setPredicate.read()) {

    bool newPredicate;

    switch(operation.read()) {
      case InstructionMap::ADDU:
      case InstructionMap::ADDUI: newPredicate = ((unsigned int)val1+(unsigned int)val2) > UINT_MAX;
      case InstructionMap::SUBU:  newPredicate = ((int)val1-(int)val2) < 0;
      default: newPredicate = result&1; // Store lowest bit in predicate register
    }

    setPred(newPredicate);
  }

}

void ALU::setPred(bool val) {
  predicate.write(val);
  if(DEBUG) cout << this->name() << " set predicate bit to " << val << endl;
}

ALU::ALU(sc_module_name name) : Component(name) {

  SC_METHOD(doOp);
  sensitive << operation;
  dont_initialize();

}

ALU::~ALU() {

}
