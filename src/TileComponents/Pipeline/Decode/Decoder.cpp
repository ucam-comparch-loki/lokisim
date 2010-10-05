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

bool Decoder::decodeInstruction(const Instruction inst, DecodedInst& dec) {

  if(currentlyWriting) {
    return completeWrite(inst, dec);
  }

  dec = DecodedInst(inst);

  // Instructions that never reach the ALU (e.g. fetch) need to know whether
  // they should execute in this stage.
  bool execute = shouldExecute(dec.predicate());

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(dec.predicate() == Instruction::P) {
      Instruction copy = inst;

      copy.predicate(Instruction::END_OF_PACKET); // Set it to always execute
      dec.result(copy.toLong());
      dec.channelMapEntry(sendChannel);

      // Prevent other stages from trying to execute this instruction.
      dec.operation(InstructionMap::OR);
      dec.destination(0);

      return true;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Only perform this if we aren't blocked to prevent repeated occurrences.
  if(!blocked && !InstructionMap::isALUOperation(dec.operation()))
    Instrumentation::operation(dec, execute, id);

  // Need a big try block in case we block when reading from a channel end.
  try {

    uint8_t operation = dec.operation();

    // Deal with all operations which require additional communication with
    // other components.
    switch(operation) {

      case InstructionMap::LD :
      case InstructionMap::LDB : {
        setOperand1ToValue(dec, dec.sourceReg1());
        setOperand2ToValue(dec, 0, dec.immediate());
        dec.operation(InstructionMap::ADDUI);
        if(operation == InstructionMap::LD)
             dec.memoryOp(MemoryRequest::LOAD);
        else dec.memoryOp(MemoryRequest::LOAD_B);
        break;
      }

      case InstructionMap::ST :
      case InstructionMap::STB : {
        setOperand1ToValue(dec, dec.sourceReg2());
        setOperand2ToValue(dec, 0, dec.immediate());
        dec.operation(InstructionMap::ADDUI);
        dec.sourceReg1(0); dec.sourceReg2(0);
        currentlyWriting = true;
        if(operation == InstructionMap::ST)
             dec.memoryOp(MemoryRequest::STORE);
        else dec.memoryOp(MemoryRequest::STORE_B);
        break;
      }

      case InstructionMap::STADDR :
      case InstructionMap::STBADDR : {
        setOperand1ToValue(dec, dec.sourceReg1());
        setOperand2ToValue(dec, 0, dec.immediate());
        dec.operation(InstructionMap::ADDUI);
        if(operation == InstructionMap::STADDR)
             dec.memoryOp(MemoryRequest::STADDR);
        else dec.memoryOp(MemoryRequest::STBADDR);
        break;
      }

      case InstructionMap::WOCHE : {
        dec.result(dec.immediate());
        break;
      }

      case InstructionMap::TSTCH : {
        dec.result(parent()->testChannel(dec.sourceReg1()));
        break;
      }

      case InstructionMap::SELCH : {
        dec.result(parent()->selectChannel());
        break;
      }

      case InstructionMap::IBJMP : {
        if(execute) {
          parent()->jump((int8_t)dec.immediate());
        }
        dec.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::RMTEXECUTE : {
        remoteExecute = true;
        sendChannel = dec.channelMapEntry();
        dec.channelMapEntry(Instruction::NO_CHANNEL);
        dec.result(0);
        if(DEBUG) cout << this->name() << " beginning remote execution" << endl;
        break;
      }

      case InstructionMap::RMTFETCH : {
        string opName = "fetch";
        Instruction copy = inst;
        copy.opcode(InstructionMap::opcode(opName));
        copy.predicate(Instruction::END_OF_PACKET);
        dec.result(copy.toLong());
        break;
      }

      case InstructionMap::RMTFETCHPST : {
        string opName = "fetchpst";
        Instruction copy = inst;
        copy.opcode(InstructionMap::opcode(opName));
        copy.predicate(Instruction::END_OF_PACKET);
        dec.result(copy.toLong());
        break;
      }

      //case InstructionMap::RMTFILL :
      //case InstructionMap::RMTNXIPK :

      // Deprecated: use "setchmap 0 rs" instead.
      case InstructionMap::SETFETCHCH : {
        // See if we should execute first?
        fetchChannel = dec.immediate();
        dec.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::SETCHMAP : {
        // Don't set the result because this register read may be out of date.
        dec.operand1(readRegs(dec.sourceReg1()));
        // The rest of the table is in the write stage, so we only deal with
        // entry 0 here. Note that at least two cycles are required between
        // storing the value to a register and reading it here.
        if(dec.immediate() == 0) {
          fetchChannel = dec.operand1();
        }
        break;
      }

      case InstructionMap::FETCH :
      case InstructionMap::FETCHPST : {
        // Fetches are unusual in that they never reach the execute stage, where
        // the predicate bit is usually checked. We have to do the check here
        // instead.
        if(execute) {
          Address fetchAddr(dec.immediate() + readRegs(dec.sourceReg1()),
                            fetchChannel);
          fetch(fetchAddr);
          parent()->setPersistent(operation == InstructionMap::FETCHPST);
        }

        dec.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::PSELFETCH : {
        // TODO: stall for one cycle to allow predicate bit to be written.

        uint32_t selected;
        if(parent()->predicate()) selected = readRegs(dec.sourceReg1());
        else                      selected = readRegs(dec.sourceReg2());
        Address fetchAddr(dec.immediate() + selected, fetchChannel);
        fetch(fetchAddr);
        parent()->setPersistent(false);
        dec.result(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::IRDR : {
        // Is there the possibility of requiring data forwarding here?
        dec.result(readRegs(dec.sourceReg1(), true));
        break;
      }

      default : {
        setOperand1(dec);
        setOperand2(dec);
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

bool Decoder::ready() {
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

int32_t Decoder::readRegs(uint8_t index, bool indirect) {
  return parent()->readReg(index, indirect);
}

int32_t Decoder::readRCET(uint8_t index) {
  return parent()->readRCET(index);
}

void Decoder::fetch(Address a) {
  parent()->fetch(a);
}

/* Sends the second part of a two-flit store operation (the data to send). */
bool Decoder::completeWrite(Instruction i, DecodedInst& dec) {
  dec = DecodedInst(i);

  try {
    setOperand1ToValue(dec, dec.sourceReg1());
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
  return (DecodeStage*)(this->get_parent());
}

Decoder::Decoder(sc_module_name name, int ID) : Component(name) {

  this->id = ID;

  sendChannel = fetchChannel = -1;
  remoteExecute = false;

}

Decoder::~Decoder() {

}
