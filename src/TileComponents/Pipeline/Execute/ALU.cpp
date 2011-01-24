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

bool ALU::execute(DecodedInst& dec) const {

  if(dec.hasResult()) return true;

  bool execute = shouldExecute(dec.predicate());

  if(dec.isALUOperation())
    Instrumentation::operation(dec, execute, id);

  if(dec.operation() == InstructionMap::NOP) return false;
  if(!execute) return false;

  bool pred = parent()->getPredicate();

  // Cast to 32 bits because our architecture is supposed to use 32-bit
  // arithmetic.
  int32_t val1 = dec.operand1();
  int32_t val2 = dec.operand2();
  int32_t result;

  if(DEBUG) cout << this->name() << ": executing " <<
    dec.name() << " on " << val1 << " and " << val2 << endl;

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
    case InstructionMap::MULHW:  result = ((int64_t)val1 * (int64_t)val2) >> 32; break;
                                 // Convert to unsigned ints, then pad to 64 bits.
    case InstructionMap::MULHWU: result = ((uint64_t)((uint32_t)val1) *
                                           (uint64_t)((uint32_t)val2)) >> 32; break;
    case InstructionMap::MULLW:  result = ((int64_t)val1 * (int64_t)val2) & -1; break;

    default: result = val1; // Is this a good default?
  }

  dec.result(result);

  if(dec.setsPredicate()) {

    bool newPredicate;

    switch(dec.operation()) {
      // For additions and subtractions, the predicate signals overflow or
      // underflow.
      case InstructionMap::ADDU:
      case InstructionMap::ADDUI: {
        int64_t result64 = (int64_t)val1 + (int64_t)val2;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      // The 68k and x86 set the borrow bit if a - b < 0 for subtractions.
      // The 6502 and PowerPC treat it as a carry bit.
      // http://en.wikipedia.org/wiki/Carry_flag
      case InstructionMap::SUBU: {
        int64_t result64 = (int64_t)val1 - (int64_t)val2;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      case InstructionMap::RSUBI: {
        int64_t result64 = (int64_t)val2 - (int64_t)val1;
        newPredicate = (result64 > INT_MAX) || (result64 < INT_MIN);
        break;
      }

      // Otherwise, it holds the least significant bit of the result.
      // Potential alternative: newPredicate = (result != 0)
      default: newPredicate = result&1;
    }

    setPred(newPredicate);
  }

  return true;

}

void ALU::setPred(bool val) const {
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
  id = parent()->id;
}

ALU::~ALU() {

}
