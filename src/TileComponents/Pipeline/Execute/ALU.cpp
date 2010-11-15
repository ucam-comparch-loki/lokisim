/*
 * ALU.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "ALU.h"
#include "ExecuteStage.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Datatype/DecodedInst.h"
#include "../../../Datatype/Instruction.h"

bool ALU::execute(DecodedInst& dec) {

  if(dec.operation() == InstructionMap::NOP) return false;
  if(dec.hasResult()) return true;

  bool execute = shouldExecute(dec.predicate());

  if(InstructionMap::isALUOperation(dec.operation()))
    Instrumentation::operation(dec, execute, parent()->id);

  if(!execute) return false;

  bool pred = parent()->getPredicate();

  // Cast to 32 bits because our architecture is supposed to use 32-bit
  // arithmetic. Also perform any necessary data forwarding.
  int32_t val1 = dec.operand1();
  int32_t val2 = dec.operand2();
  int32_t result;

  if(DEBUG) cout << this->name() << ": executing " <<
    InstructionMap::name(dec.operation())<<" on "<<val1<<" and "<<val2<<endl;

  switch(dec.operation()) {

    case InstructionMap::SLLI:
    case InstructionMap::SLL:   result = val1 << val2; break;
    case InstructionMap::SRLI:
    case InstructionMap::SRL:   result = val1 >> val2; break;
    case InstructionMap::SRAI:
    case InstructionMap::SRA:   result = val1 >> val2; break;

    case InstructionMap::SETEQ:
    case InstructionMap::SETEQI:   result = (val1 == val2); break;
    case InstructionMap::SETNE:
    case InstructionMap::SETNEI:   result = (val1 != val2); break;
    case InstructionMap::SETLT:
    case InstructionMap::SETLTI:   result = (val1 < val2); break;
    case InstructionMap::SETLTU:
    case InstructionMap::SETLTUI:  result = ((uint32_t)val1 < (uint32_t)val2); break;
    case InstructionMap::SETGTE:   result = (val1 >= val2); break;
    case InstructionMap::SETGTEU:
    case InstructionMap::SETGTEUI: result = ((uint32_t)val1 >= (uint32_t)val2); break;

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

    case InstructionMap::LDW:
    case InstructionMap::LDBU:
    case InstructionMap::STWADDR:
    case InstructionMap::STBADDR:
    case InstructionMap::ADDU:
    case InstructionMap::ADDUI:  result = val1 + val2; break;
    case InstructionMap::SUBU:   result = val1 - val2; break;
    case InstructionMap::MULHW:
    case InstructionMap::MULHWU: result = ((int64_t)val1 * (int64_t)val2) >> 32; break;
    case InstructionMap::MULLW:  result = ((int64_t)val1 * (int64_t)val2) & -1; break;

    default: result = val1; // Is this a good default?
  }

  dec.result(result);

  if(dec.setsPredicate()) {

    bool newPredicate;

    switch(dec.operation()) {
      // For additions, the predicate bit acts as the carry bit.
      case InstructionMap::ADDU:
      case InstructionMap::ADDUI: {
        int64_t result64 = (int64_t)val1 + (int64_t)val2;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      // For subtractions, the predicate bit acts as the borrow bit.
      case InstructionMap::SUBU: {
        newPredicate = ((int64_t)val1-(int64_t)val2) < 0;
        break;
      }

      case InstructionMap::RSUBI: {
        newPredicate = ((int64_t)val2-(int64_t)val1) < 0;
        break;
      }

      // Otherwise, it holds the least significant bit of the result.
      default: newPredicate = result&1; // Store lowest bit in predicate register
    }

    setPred(newPredicate);
  }

  return true;

}

void ALU::setPred(bool val) {
  parent()->setPredicate(val);
}

/* Determine whether this instruction should be executed. */
bool ALU::shouldExecute(short predBits) const {
  bool pred = parent()->getPredicate();

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  pred) ||
                (predBits == Instruction::NOT_P && !pred);

  return result;

}

ExecuteStage* ALU::parent() const {
  return dynamic_cast<ExecuteStage*>(this->get_parent());
}

ALU::ALU(sc_module_name name) : Component(name) {

}

ALU::~ALU() {

}
