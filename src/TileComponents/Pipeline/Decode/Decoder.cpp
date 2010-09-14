/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"
#include "DecodeStage.h"
#include "ReceiveChannelEndTable.h"
#include "../IndirectRegisterFile.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Exceptions/BlockedException.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Tile.h"

typedef IndirectRegisterFile Registers;

bool Decoder::decodeInstruction(Instruction i, DecodedInst& dec) {

  if(DEBUG) cout << this->name() << " received Instruction: " << i << endl;

  if(currentlyWriting) {
    return completeWrite(i, dec);
  }

  dec = DecodedInst(i);

  // TODO: move decision of whether or not an instruction should execute
  // into the execute stage/ALU
  bool execute = shouldExecute(dec.getPredicate());

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(dec.getPredicate() == Instruction::P) {
      i.setPredicate(Instruction::END_OF_PACKET); // Set it to always execute
//      instructionOut.write(i);
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
  Instrumentation::operation(i, execute, id);

  // Need a big try block in case we block when reading from a channel end.
  try {

    uint8_t operation = dec.getOperation();

    // Deal with all operations which require additional communication with
    // other components.
    switch(operation) {

      case InstructionMap::LD :
      case InstructionMap::LDB : {
        setOperand1ToValue(dec, dec.getDestination());
        setOperand2ToValue(dec, 0, dec.getImmediate());
        dec.setDestination(0);
        dec.setOperation(InstructionMap::ADDUI);
  //      if(operation == InstructionMap::LD) memoryOp.write(MemoryRequest::LOAD);
  //      else                                memoryOp.write(MemoryRequest::LOAD_B);
        break;
      }

      case InstructionMap::ST :
      case InstructionMap::STB : {
        setOperand1ToValue(dec, dec.getSource1());
        setOperand2ToValue(dec, 0, dec.getImmediate());
        dec.setDestination(0);
        dec.setOperation(InstructionMap::ADDUI);
        currentlyWriting = true;
  //      if(operation == InstructionMap::ST) memoryOp.write(MemoryRequest::STORE);
  //      else                                memoryOp.write(MemoryRequest::STORE_B);
        break;
      }

      case InstructionMap::STADDR :
      case InstructionMap::STBADDR : {
        setOperand1ToValue(dec, dec.getDestination());
        setOperand2ToValue(dec, 0, dec.getImmediate());
        dec.setDestination(0);
        dec.setOperation(InstructionMap::ADDUI);
  //      if(operation == InstructionMap::STADDR)
  //           memoryOp.write(MemoryRequest::STADDR);
  //      else memoryOp.write(MemoryRequest::STBADDR);
        break;
      }

      case InstructionMap::TSTCH : {
  //      channelOp.write(ReceiveChannelEndTable::TSTCH);
        readRCET(Registers::toChannelID(dec.getSource1()));
        break;
      }

      case InstructionMap::SELCH : {
  //      channelOp.write(ReceiveChannelEndTable::SELCH);
        break;
      }

      case InstructionMap::IBJMP : {
  //      jumpOffset.write(immediate);
        break;
      }

  //    case InstructionMap::WOCHE : {
  //      // In write stage, detect woche and act there.
  //      break;
  //    }

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
        // send i
        break;
      }

      case InstructionMap::RMTFETCHPST : {
        string opName = "fetchpst";
        i.setOp(InstructionMap::opcode(opName));
        i.setPredicate(Instruction::END_OF_PACKET);
        // send i
        break;
      }

      //case InstructionMap::RMTFILL :
      //case InstructionMap::RMTNXIPK :

      // Deprecated: use "setchmap 0 rs" instead.
      case InstructionMap::SETFETCHCH : {
        fetchChannel = dec.getImmediate();
        break;
      }

      case InstructionMap::SETCHMAP : {
        // The rest of the table is in the write stage.
        if(dec.getImmediate() == 0) fetchChannel = readRegs(dec.getSource1());
        break;
      }

      case InstructionMap::FETCH :
      case InstructionMap::FETCHPST : {
        Address fetchAddr(dec.getImmediate() + readRegs(dec.getDestination()),
                          fetchChannel);
        fetch(fetchAddr);
  //      cache.setPersistent(operation == InstructionMap::FETCHPST);
        break;
      }

      case InstructionMap::PSELFETCH : {
        uint32_t selected;
  //      if(predicate()) selected = readReg(dec.getDestination());
  //      else            selected = readReg(dec.getSource1());
        Address fetchAddr(dec.getImmediate() + selected, fetchChannel);
        fetch(fetchAddr);
  //      cache.setPersistent(false);
        break;
      }

      case InstructionMap::IRDR : {
        dec.setOperand1(readRegs(dec.getSource1(), true));
        break;
      }

  //    case InstructionMap::IWTR : {
  //      // Deal with indirect writes in the write stage
  //    }

      default : {
        setOperand1(dec);
        setOperand2(dec);

        if(dec.getDestination() == 0) regLastWritten = -1;
        else regLastWritten = dec.getDestination();
      }

    }

  }
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
  if(Registers::isChannelEnd(reg)) {
    dec.setOperand1(readRCET(Registers::toChannelID(reg)));
  }
  else {
    if(readALUOutput(reg)) {
//      dec.setOperand1(alu.getLastResult()); // beware of timing?
      // Possibly send a tag to the execute stage so this can be done there
    }
    else {
      dec.setOperand1(readRegs(reg));
    }
  }
}

/* Determine where to get second operand from: immediate, RCET, ALU or regs */
void Decoder::setOperand2ToValue(DecodedInst& dec, int32_t reg, int32_t immed) {
  if(InstructionMap::hasImmediate(dec.getOperation())) {
    dec.setOperand2(immed);
  }
  else if(Registers::isChannelEnd(reg)) {
        dec.setOperand2(readRCET(Registers::toChannelID(reg)));
  }
  else {
    if(readALUOutput(reg)) {
//      dec.setOperand1(alu.getLastResult()); // beware of timing?
      // Possibly send a tag to the execute stage so this can be done there
    }
    else {
      dec.setOperand2(readRegs(reg));
    }
  }
}

#define PARENT ((DecodeStage*)(this->get_parent()))

int32_t Decoder::readRegs(uint8_t index, bool indirect) {
  return PARENT->readReg(index, indirect);
}

int32_t Decoder::readRCET(uint8_t index) {
  return PARENT->readRCET(index);
}

void Decoder::fetch(Address a) {
  PARENT->fetch(a);
}

#undef PARENT

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
                (predBits == Instruction::END_OF_PACKET)/* ||
                (predBits == Instruction::P     &&  predicate()) ||
                (predBits == Instruction::NOT_P && !predicate())*/;

  return result;

}

/* Determines whether to forward the output of the ALU when trying to read
 * from this register. This should happen when this register was written to
 * in the previous cycle, and the register isn't reserved. */
bool Decoder::readALUOutput(short reg) {
  return (reg != 0) && (reg == regLastWritten) && (regLastWritten != -1);
}

Decoder::Decoder(sc_module_name name, int ID) : Component(name) {

  this->id = ID;

  regLastWritten = sendChannel = fetchChannel = -1;
  remoteExecute = false;

}

Decoder::~Decoder() {

}
