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
#include "../../../Utility/Instrumentation/Stalls.h"

typedef IndirectRegisterFile Registers;

bool Decoder::decodeInstruction(const DecodedInst& input, DecodedInst& output) {

  if(currentlyWriting) {
    return completeWrite(input, output);
  }

  output = input;
  haveStalled = false;

  if(discardNextInst) {
    output.result(0);
    output.destination(0);
    discardNextInst = false;
    return true;
  }

  // Instructions that never reach the ALU (e.g. fetch) need to know whether
  // they should execute in this stage.
  bool execute = shouldExecute(input);

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(output.predicate() == Instruction::P) {
      Instruction copy = input.toInstruction();

      copy.predicate(Instruction::END_OF_PACKET); // Set it to always execute
      output.result(copy.toLong());
      output.channelMapEntry(sendChannel);

      // Prevent other stages from trying to execute this instruction.
      output.operation(InstructionMap::OR);
      output.destination(0);

      return true;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Only perform this if we aren't blocked to prevent repeated occurrences.
  if(!blocked && !InstructionMap::isALUOperation(input.operation()))
    Instrumentation::operation(input, execute, id);

  // If the instruction may perform destructive reads from a channel-end,
  // and we know it won't execute, stop it here.
  if(!execute && InstructionMap::isALUOperation(input.operation()) &&
                (Registers::isChannelEnd(input.sourceReg1()) ||
                 Registers::isChannelEnd(input.sourceReg2()))) {
    blocked = false;
    return true;
  }

  settingPredicate = input.setsPredicate();

  // Need a big try block in case we block when reading from a channel end.
  try {

    uint8_t operation = output.operation();

    // Deal with all operations which require additional communication with
    // other components.
    switch(operation) {

      case InstructionMap::LDW :
      case InstructionMap::LDBU : {
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
          JumpOffset jump = (JumpOffset)output.immediate();

          // We need to go back by one more instruction if we have taken a
          // cycle here, because the fetch stage will have already supplied
          // the next instruction.
          if(haveStalled) {
            jump -= BYTES_PER_WORD;
            discardNextInst = true;
          }

          parent()->jump(jump);
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
void Decoder::setOperand1ToValue(DecodedInst& dec, RegisterIndex reg) {
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
void Decoder::setOperand2ToValue(DecodedInst& dec, RegisterIndex reg, int32_t immed) {
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
  // Block until we have the result (may take multiple cycles).
  blocked = true;
  int32_t result = parent()->readRCET(index);
  blocked = false;
  return result;
}

void Decoder::fetch(MemoryAddr addr) {
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
bool Decoder::shouldExecute(const DecodedInst& inst) {

  // If the previous instruction sets the predicate bit, and we need to access
  // it in this pipeline stage, we must stall for a cycle until it has been
  // written.
  if(settingPredicate && inst.usesPredicate()) {

    // We need to know the predicate value in this cycle if we are doing
    // something like a fetch, which is carried out in the decode stage, or if
    // we read from a channel-end, as these reads are destructive.
    if(!InstructionMap::isALUOperation(inst.operation()) ||
       Registers::isChannelEnd(inst.sourceReg1()) ||
       Registers::isChannelEnd(inst.sourceReg2())) {
      if(DEBUG) cout << this->name()
          << " waiting one cycle for predicate to be set." << endl;

      blocked = true;
      Instrumentation::stalled(id, true, Stalls::PREDICATE);
      wait(1, sc_core::SC_NS);
      blocked = false;
      Instrumentation::stalled(id, false);

      haveStalled = true;
    }
  }

  short predBits = inst.predicate();

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
