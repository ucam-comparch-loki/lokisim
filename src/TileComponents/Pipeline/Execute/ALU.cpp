/*
 * ALU.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ALU.h"
#include "ExecuteStage.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Datatype/Instruction.h"

bool ALU::execute(DecodedInst& dec) {

  if(dec.getOperation() == InstructionMap::NOP) return false;
  if(dec.hasResult()) return true;

  bool execute = shouldExecute(dec.getPredicate());
//  Instrumentation::operation(operation.read(), execute);
  if(!execute) return false;

  bool pred = parent()->getPredicate();

  // Cast to 32 bits because our architecture is supposed to use 32-bit
  // arithmetic. Also perform any necessary data forwarding.
  int32_t val1 = (dec.getSource1() == lastDestination) ? lastResult
                                                       : dec.getOperand1();
  int32_t val2 = (dec.getSource2() == lastDestination) ? lastResult
                                                       : dec.getOperand2();
  int32_t result;

  if(DEBUG) cout << this->name() << ": executing " <<
    InstructionMap::name(dec.getOperation())<<" on "<<val1<<" and "<<val2<<endl;

  switch(dec.getOperation()) {

    case InstructionMap::SLL:
    case InstructionMap::SLLV:   result = val1 << val2; break;
    case InstructionMap::SRL:
    case InstructionMap::SRLV:   result = val1 >> val2; break;
    case InstructionMap::SRA:
    case InstructionMap::SRAV:   result = val1 >> val2; break;

    case InstructionMap::SEQ:
    case InstructionMap::SEQI:   result = (val1 == val2); break;
    case InstructionMap::SNE:
    case InstructionMap::SNEI:   result = (val1 != val2); break;
    case InstructionMap::SLT:
    case InstructionMap::SLTI:   result = (val1 < val2); break;
    case InstructionMap::SLTU:
    case InstructionMap::SLTIU:  result = (val1 < val2); break;

    case InstructionMap::LUI:    result = val2 << 16; break;

    case InstructionMap::PSEL:   result = pred ? val1 : val2; break;

//    case InstructionMap::CLZ:    result = 32 - math.log(2, val1) + 1; break;

    case InstructionMap::NOR:
    case InstructionMap::NORI:   result = ~(val1 | val2); break;
    case InstructionMap::AND:
    case InstructionMap::ANDI:   result = val1 & val2; break;
    case InstructionMap::OR:
    case InstructionMap::ORI:    result = val1 | val2; break;
    case InstructionMap::XOR:
    case InstructionMap::XORI:   result = val1 ^ val2; break;

    case InstructionMap::NAND:   result = ~(val1 & val2); break;
    case InstructionMap::CLR:    result = val1 & ~val2; break;
    case InstructionMap::ORC:    result = val1 | ~val2; break;
    case InstructionMap::POPC:   result = __builtin_popcount(val1); break;
    case InstructionMap::RSUBI:  result = val2 - val1; break;

    case InstructionMap::ADDU:
    case InstructionMap::ADDUI:  result = val1 + val2; break;
    case InstructionMap::SUBU:   result = val1 - val2; break;
    case InstructionMap::MULHW:
    case InstructionMap::MULHWU: result = ((int64_t)val1 * (int64_t)val2) >> 32; break;
    case InstructionMap::MULLW:  result = ((int64_t)val1 * (int64_t)val2) & -1; break;

    default: result = val1; // Is this a good default?
  }

  dec.setResult(result);
  lastResult = result;
  // We don't want to forward any data sent to register 0.
  lastDestination = (dec.getDestination() == 0) ? -1 : dec.getDestination();

  if(dec.getSetPredicate()) {

    bool newPredicate;

    switch(dec.getOperation()) {
      // For additions, the predicate bit acts as the carry bit.
      case InstructionMap::ADDU:
      case InstructionMap::ADDUI: newPredicate = ((int64_t)val1+(int64_t)val2) > UINT_MAX;

      // For subtractions, the predicate bit acts as the borrow bit.
      case InstructionMap::SUBU:  newPredicate = ((int64_t)val1-(int64_t)val2) < 0;

      // Otherwise, it holds the least significant bit of the result.
      default: newPredicate = result&1; // Store lowest bit in predicate register
    }

    setPred(newPredicate);
  }

  return true;

}

void ALU::setPred(bool val) {
  parent()->setPredicate(val);
  if(DEBUG) cout << this->name() << " set predicate bit to " << val << endl;
}

/* Determine whether this instruction should be executed. */
bool ALU::shouldExecute(short predBits) {
  bool pred = parent()->getPredicate();

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  pred) ||
                (predBits == Instruction::NOT_P && !pred);

  return result;

}

ExecuteStage* ALU::parent() const {
  return (ExecuteStage*)(this->get_parent());
}

ALU::ALU(sc_module_name name) : Component(name) {

}

ALU::~ALU() {

}
