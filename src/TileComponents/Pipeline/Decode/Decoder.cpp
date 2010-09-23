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

bool Decoder::decodeInstruction(Instruction i, DecodedInst& dec) {

  if(currentlyWriting) {
    return completeWrite(i, dec);
  }

  dec = DecodedInst(i);

  // Instructions that never reach the ALU (e.g. fetch) need to know whether
  // they should execute in this stage.
  bool execute = shouldExecute(dec.getPredicate());

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(dec.getPredicate() == Instruction::P) {
      i.setPredicate(Instruction::END_OF_PACKET); // Set it to always execute
      dec.setResult(i.toLong());
      dec.setChannelMap(sendChannel);
      return true;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Need this in ALU instead? No - we change the operation sometimes.
  // Doesn't matter: the different operation is still being executed.
  Instrumentation::operation(i, execute, id);

  // Need a big try block in case we block when reading from a channel end.
  try {

    uint8_t operation = dec.getOperation();

    // Deal with all operations which require additional communication with
    // other components.
    switch(operation) {

      case InstructionMap::LD :
      case InstructionMap::LDB : {
        setOperand1ToValue(dec, dec.getSource1());
        setOperand2ToValue(dec, 0, dec.getImmediate());
        dec.setOperation(InstructionMap::ADDUI);
        if(operation == InstructionMap::LD)
             dec.setMemoryOp(MemoryRequest::LOAD);
        else dec.setMemoryOp(MemoryRequest::LOAD_B);
        break;
      }

      case InstructionMap::ST :
      case InstructionMap::STB : {
        setOperand1ToValue(dec, dec.getSource1());
        setOperand2ToValue(dec, 0, dec.getImmediate());
        dec.setOperation(InstructionMap::ADDUI);
        currentlyWriting = true;
        if(operation == InstructionMap::ST)
             dec.setMemoryOp(MemoryRequest::STORE);
        else dec.setMemoryOp(MemoryRequest::STORE_B);
        break;
      }

      case InstructionMap::STADDR :
      case InstructionMap::STBADDR : {
        setOperand1ToValue(dec, dec.getSource1());
        setOperand2ToValue(dec, 0, dec.getImmediate());
        dec.setOperation(InstructionMap::ADDUI);
        if(operation == InstructionMap::STADDR)
             dec.setMemoryOp(MemoryRequest::STADDR);
        else dec.setMemoryOp(MemoryRequest::STBADDR);
        break;
      }

      case InstructionMap::WOCHE : {
        dec.setResult(dec.getImmediate());
        break;
      }

      case InstructionMap::TSTCH : {
        dec.setResult(parent()->testChannel(dec.getSource1()));
        break;
      }

      case InstructionMap::SELCH : {
        dec.setResult(parent()->selectChannel());
        break;
      }

      case InstructionMap::IBJMP : {
        if(execute) {
          parent()->jump((int8_t)dec.getImmediate());
        }
        dec.setResult(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::RMTEXECUTE : {
        remoteExecute = true;
        sendChannel = dec.getChannelMap();
        if(DEBUG) cout << this->name() << " beginning remote execution" << endl;
        break;
      }

      case InstructionMap::RMTFETCH : {
        string opName = "fetch";
        i.setOp(InstructionMap::opcode(opName));
        i.setPredicate(Instruction::END_OF_PACKET);
        dec.setResult(i.toLong());
        break;
      }

      case InstructionMap::RMTFETCHPST : {
        string opName = "fetchpst";
        i.setOp(InstructionMap::opcode(opName));
        i.setPredicate(Instruction::END_OF_PACKET);
        dec.setResult(i.toLong());
        break;
      }

      //case InstructionMap::RMTFILL :
      //case InstructionMap::RMTNXIPK :

      // Deprecated: use "setchmap 0 rs" instead.
      case InstructionMap::SETFETCHCH : {
        // See if we should execute first?
        fetchChannel = dec.getImmediate();
        dec.setResult(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::SETCHMAP : {
        // Don't set the result because this register read may be out of date.
        dec.setOperand1(readRegs(dec.getSource1()));
        // The rest of the table is in the write stage, so we only deal with
        // entry 0 here. Note that at least two cycles are required between
        // storing the value to a register and reading it here.
        if(dec.getImmediate() == 0) {
          fetchChannel = dec.getOperand1();
        }
        break;
      }

      case InstructionMap::FETCH :
      case InstructionMap::FETCHPST : {
        // Fetches are unusual in that they never reach the execute stage, where
        // the predicate bit is usually checked. We have to do the check here
        // instead.
        if(execute) {
          Address fetchAddr(dec.getImmediate() + readRegs(dec.getSource1()),
                            fetchChannel);
          fetch(fetchAddr);
          parent()->setPersistent(operation == InstructionMap::FETCHPST);
        }

        dec.setResult(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::PSELFETCH : {
        // TODO: stall for one cycle to allow predicate bit to be written.

        uint32_t selected;
        if(parent()->predicate()) selected = readRegs(dec.getSource1());
        else                      selected = readRegs(dec.getSource2());
        Address fetchAddr(dec.getImmediate() + selected, fetchChannel);
        fetch(fetchAddr);
        parent()->setPersistent(false);
        dec.setResult(0);   // Stop the ALU doing anything by storing a result.
        break;
      }

      case InstructionMap::IRDR : {
        // Is there the possibility of requiring data forwarding here?
        dec.setResult(readRegs(dec.getSource1(), true));
        break;
      }

      default : {
        setOperand1(dec);
        setOperand2(dec);

        if(dec.getDestination() == 0) regLastWritten = -1;
        else regLastWritten = dec.getDestination();
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
  setOperand1ToValue(dec, dec.getSource1());
}

void Decoder::setOperand2(DecodedInst& dec) {
  setOperand2ToValue(dec, dec.getSource2(), dec.getImmediate());
}

/* Determine where to read the first operand from: RCET, ALU or registers */
void Decoder::setOperand1ToValue(DecodedInst& dec, int32_t reg) {
  // There are some cases where we are repeating the operation, and don't want
  // to store the operands again.
  // For example: both operands are read from the channel ends, but the second
  // one hasn't arrived yet, so we block. We don't want to read the first
  // channel end again because the data will have gone.
  if(dec.hasOperand1()) return;

  if(Registers::isChannelEnd(reg)) {
    dec.setOperand1(readRCET(Registers::toChannelID(reg)));
  }
  else {
    dec.setOperand1(readRegs(reg));
  }
}

/* Determine where to get second operand from: immediate, RCET, ALU or regs */
void Decoder::setOperand2ToValue(DecodedInst& dec, int32_t reg, int32_t immed) {
  if(InstructionMap::hasImmediate(dec.getOperation())) {
    dec.setOperand2(immed);
    // This would require use of the sign-extender.
    // Remember to factor in the energy use.
  }
  else if(Registers::isChannelEnd(reg)) {
    dec.setOperand2(readRCET(Registers::toChannelID(reg)));
  }
  else {
    dec.setOperand2(readRegs(reg));
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
  regLastWritten = -1; // Hack - we're executing the same instruction twice
  dec = DecodedInst(i);
  setOperand1ToValue(dec, dec.getDestination());
  currentlyWriting = false;

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

/* Determines whether to forward the output of the ALU when trying to read
 * from this register. This should happen when this register was written to
 * in the previous cycle, and the register isn't reserved. */
bool Decoder::readALUOutput(short reg) {
  return (reg != 0) && (reg == regLastWritten) && (regLastWritten != -1);
}

DecodeStage* Decoder::parent() const {
  return (DecodeStage*)(this->get_parent());
}

Decoder::Decoder(sc_module_name name, int ID) : Component(name) {

  this->id = ID;

  regLastWritten = sendChannel = fetchChannel = -1;
  remoteExecute = false;

}

Decoder::~Decoder() {

}
