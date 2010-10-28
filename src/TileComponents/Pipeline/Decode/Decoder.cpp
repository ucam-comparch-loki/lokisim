/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"
#include "DecodeStage.h"
#include "../IndirectRegisterFile.h"
#include "../../../Datatype/Address.h"
#include "../../../Datatype/Instruction.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Exceptions/BlockedException.h"
#include "../../../Utility/InstructionMap.h"

typedef IndirectRegisterFile Registers;

bool Decoder::decodeInstruction(const DecodedInst& input, DecodedInst& output) {

  if(currentlyWriting) {
    return completeWrite(input, output);
  }

  output = input;

  // Instructions that never reach the ALU (e.g. fetch) need to know whether
  // they should execute in this stage.
  bool execute = shouldExecute(output.predicate());

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(output.predicate() == Instruction::P) {
      Instruction copy = input.toInstruction();

      copy.predicate(Instruction::END_OF_PACKET); // Set it to always execute
      output.result(copy.toLong());
      output.channelMapEntry(sendChannel);

      // Prevent other stages from trying to execute this instruction.
      output.operation(InstructionMap::OR);
      output.destinationReg(0);

      return true;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Only perform this if we aren't blocked to prevent repeated occurrences.
  if(!blocked && !InstructionMap::isALUOperation(output.operation()))
    Instrumentation::operation(output, execute, id);

  // Need a big try block in case we block when reading from a channel end.
  try {

    uint8_t operation = output.operation();

    // Deal with all operations which require additional communication with
    // other components.
    switch(operation) {

      case InstructionMap::LDW :
      case InstructionMap::LDB : {
        setOperand1ToValue(output, output.sourceReg1());
        setOperand2ToValue(output, 0, output.immediate());
        if(operation == InstructionMap::LDW)
             output.memoryOp(MemoryRequest::LOAD);
        else output.memoryOp(MemoryRequest::LOAD_B);
        break;
      }

      case InstructionMap::STW :
      case InstructionMap::STB : {
        setOperand1ToValue(output, output.sourceReg2());
        setOperand2ToValue(output, 0, output.immediate());
        output.operation(InstructionMap::ADDUI);
        output.sourceReg1(output.sourceReg2()); output.sourceReg2(0);
        currentlyWriting = true;
        if(operation == InstructionMap::STW)
             output.memoryOp(MemoryRequest::STORE);
        else output.memoryOp(MemoryRequest::STORE_B);
        break;
      }

      case InstructionMap::STWADDR :
      case InstructionMap::STBADDR : {
        setOperand1ToValue(output, output.sourceReg1());
        setOperand2ToValue(output, 0, output.immediate());
        if(operation == InstructionMap::STWADDR)
             output.memoryOp(MemoryRequest::STADDR);
        else output.memoryOp(MemoryRequest::STBADDR);
        break;
      }

      case InstructionMap::WOCHE : {
        output.result(output.immediate());
        break;
      }

      case InstructionMap::TSTCH : {
        output.result(parent()->testChannel(output.immediate()));
        break;
      }

      case InstructionMap::SELCH : {
        output.result(parent()->selectChannel());
        break;
      }

      case InstructionMap::IBJMP : {
        if(execute) {
          parent()->jump((int8_t)output.immediate());
        }
        output.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::RMTEXECUTE : {
        remoteExecute = true;
        sendChannel = output.channelMapEntry();
        output.channelMapEntry(Instruction::NO_CHANNEL);
        output.result(0);
        if(DEBUG) cout << this->name() << " beginning remote execution" << endl;
        break;
      }

      case InstructionMap::RMTFETCH : {
        string opName = "fetch";
        Instruction copy = input.toInstruction();
        copy.opcode(InstructionMap::opcode(opName));
        copy.predicate(Instruction::END_OF_PACKET);
        output.result(copy.toLong());
        break;
      }

      case InstructionMap::RMTFETCHPST : {
        string opName = "fetchpst";
        Instruction copy = input.toInstruction();
        copy.opcode(InstructionMap::opcode(opName));
        copy.predicate(Instruction::END_OF_PACKET);
        output.result(copy.toLong());
        break;
      }

      //case InstructionMap::RMTFILL :
      //case InstructionMap::RMTNXIPK :

      // Deprecated: use "setchmap 0 rs" instead.
      case InstructionMap::SETFETCHCH : {
        // See if we should execute first?
        setFetchChannel(output.immediate());
        output.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::SETCHMAP : {
        // Don't set the result because this register read may be out of date.
        output.operand1(readRegs(output.sourceReg1()));
        // The rest of the table is in the write stage, so we only deal with
        // entry 0 here. Note that at least two cycles are required between
        // storing the value to a register and reading it here.
        if(output.immediate() == 0) {
          setFetchChannel(output.operand1());
        }
        break;
      }

      case InstructionMap::FETCH :
      case InstructionMap::FETCHPST : {
        // Fetches are unusual in that they never reach the execute stage, where
        // the predicate bit is usually checked. We have to do the check here
        // instead.
        if(execute) {
          fetch(output.immediate() + readRegs(output.sourceReg1()));
          parent()->setPersistent(operation == InstructionMap::FETCHPST);
        }

        output.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::PSELFETCH : {
        // TODO: stall for one cycle to allow predicate bit to be written.
        // (Optimisation: only if the predicate was written last cycle.)

        uint32_t selected;
        if(parent()->predicate()) selected = readRegs(output.sourceReg1());
        else                      selected = readRegs(output.sourceReg2());
        fetch(output.immediate() + selected);
        parent()->setPersistent(false);
        output.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::IRDR : {
        // Is there the possibility of requiring data forwarding here?
        output.result(readRegs(output.sourceReg1(), true));
        break;
      }

      default : {
        setOperand1(output);
        setOperand2(output);
      }

    } // end switch

  } // end try
  catch(BlockedException& b) {
    blocked = true;
    return false;
  }

  blocked = false;
  return true;

}

bool Decoder::ready() const {
  return !currentlyWriting && !blocked;
}

void Decoder::setOperand1(DecodedInst& dec) {
  setOperand1ToValue(dec, dec.sourceReg1());
}

void Decoder::setOperand2(DecodedInst& dec) {
  setOperand2ToValue(dec, dec.sourceReg2(), dec.immediate());
}

/* Determine where to read the first operand from: RCET or registers */
void Decoder::setOperand1ToValue(DecodedInst& dec, int32_t reg) {
  // There are some cases where we are repeating the operation, and don't want
  // to store the operands again.
  // For example: both operands are read from the channel ends, but the second
  // one hasn't arrived yet, so we block. We don't want to read the first
  // channel end again because the data will have gone.
  if(dec.hasOperand1()) return;

  if(Registers::isChannelEnd(reg)) {
    dec.operand1(readRCET(Registers::toChannelID(reg)));
  }
  else {
    dec.operand1(readRegs(reg));
  }
}

/* Determine where to get second operand from: immediate, RCET or regs */
void Decoder::setOperand2ToValue(DecodedInst& dec, int32_t reg, int32_t immed) {
  if(InstructionMap::hasImmediate(dec.operation())) {
    dec.operand2(immed);
    // This would require use of the sign-extender.
    // Remember to factor in the energy use.
  }
  else if(Registers::isChannelEnd(reg)) {
    dec.operand2(readRCET(Registers::toChannelID(reg)));
  }
  else {
    dec.operand2(readRegs(reg));
  }
}

int32_t Decoder::readRegs(RegisterIndex index, bool indirect) {
  return parent()->readReg(index, indirect);
}

int32_t Decoder::readRCET(ChannelIndex index) {
  return parent()->readRCET(index);
}

void Decoder::fetch(uint16_t addr) {
  parent()->fetch(addr);
}

void Decoder::setFetchChannel(ChannelID channelID) {
  parent()->setFetchChannel(channelID);
}

/* Sends the second part of a two-flit store operation (the data to send). */
bool Decoder::completeWrite(const DecodedInst& input, DecodedInst& output) {
  output = input;

  try {
    setOperand1ToValue(output, output.sourceReg1());
  }
  catch(BlockedException& b) {
    blocked = true;
    return false;
  }

  currentlyWriting = false;
  blocked = false;

  return true;
}

/* Determine whether this instruction should be executed. */
bool Decoder::shouldExecute(short predBits) {

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  parent()->predicate()) ||
                (predBits == Instruction::NOT_P && !parent()->predicate());

  return result;

}

DecodeStage* Decoder::parent() const {
  // Need a dynamic cast because DecodeStage uses virtual inheritance.
  return dynamic_cast<DecodeStage*>(this->get_parent());
}

Decoder::Decoder(sc_module_name name, ComponentID ID) : Component(name) {

  this->id = ID;

  sendChannel = -1;
  remoteExecute = false;

}

Decoder::~Decoder() {

}
