/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"
#include "ReceiveChannelEndTable.h"
#include "../IndirectRegisterFile.h"
#include "../../../Datatype/MemoryRequest.h"
#include "../../../Utility/InstructionMap.h"
#include "../../../Tile.h"

typedef IndirectRegisterFile Registers;

DecodedInst& Decoder::decodeInstruction(Instruction i) {

  if(DEBUG) cout << this->name() << " received Instruction: " << i << endl;

  if(currentlyWriting) {
    return completeWrite(i);
  }

  // Remember to delete in Write Stage.
  DecodedInst* dec = new DecodedInst(i);

  // TODO: move decision of whether or not an instruction should execute
  // into the execute stage/ALU
  bool execute = shouldExecute(dec->getPredicate());

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(dec->getPredicate() == Instruction::P) {
      i.setPredicate(Instruction::END_OF_PACKET); // Set it to always execute
//      instructionOut.write(i);
      dec->setChannelMap(sendChannel);
      return *dec;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // Need this in ALU instead? No - we change the operation sometimes.
  Instrumentation::operation(i, execute, this->id);

  uint8_t operation = dec->getOperation();

  switch(operation) {

    case InstructionMap::LD :
    case InstructionMap::LDB : {
//      setOperand1(operation, dec.getDestination());
//      setOperand2(operation, 0, dec.getImmediate());
      dec->setDestination(0);
      dec->setOperation(InstructionMap::ADDUI);
//      if(operation == InstructionMap::LD) memoryOp.write(MemoryRequest::LOAD);
//      else                                memoryOp.write(MemoryRequest::LOAD_B);
      break;
    }

    case InstructionMap::ST :
    case InstructionMap::STB : {
//      setOperand1(operation, dec.getSource1());
//      setOperand2(operation, 0, dec.getImmediate());
      dec->setDestination(0);
      dec->setOperation(InstructionMap::ADDUI);
//      stall.write(true);
      currentlyWriting = true;
//      if(operation == InstructionMap::ST) memoryOp.write(MemoryRequest::STORE);
//      else                                memoryOp.write(MemoryRequest::STORE_B);
      break;
    }

    case InstructionMap::STADDR :
    case InstructionMap::STBADDR : {
//      setOperand1(operation, dec.getDestination());
//      setOperand2(operation, 0, dec.getImmediate());
      dec->setDestination(0);
      dec->setOperation(InstructionMap::ADDUI);
//      if(operation == InstructionMap::STADDR)
//           memoryOp.write(MemoryRequest::STADDR);
//      else memoryOp.write(MemoryRequest::STBADDR);
      break;
    }

    case InstructionMap::TSTCH : {
//      channelOp.write(ReceiveChannelEndTable::TSTCH);
//      toRCET1.write(Registers::toChannelID(operand1));
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
      sendChannel = dec->getChannelMap();
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

    // Deprecated: use "setchmap 0 immed" instead.
    case InstructionMap::SETFETCHCH : {
      fetchChannel = dec->getImmediate();
      break;
    }

    case InstructionMap::SETCHMAP : {
    //   if(immediate == 0) fetchChannel = registers[operand1];
       break;
    }

    case InstructionMap::FETCH :
    case InstructionMap::FETCHPST : {
//      Address fetchAddr(dec.getImmediate() + registers[dec.getDestination()],
//                        fetchChannel);
//      fetch(fetchAddr);
//      cache.setPersistent(operation == InstructionMap::FETCHPST);
      break;
    }

    case InstructionMap::PSELFETCH : {
      uint32_t selected;
//      if(predicate) selected = registers[dec.getDestination()];
//      else          selected = registers[dec.getSource1()];
      Address fetchAddr(dec->getImmediate() + selected, fetchChannel);
//      fetch(fetchAddr);
//      cache.setPersistent(false);
      break;
    }

    case InstructionMap::IRDR : {
//      dec.setOperand1(registers.indirectRead(dec.getSource1()));
      break;
    }

//    case InstructionMap::IWTR : {
//      // Deal with indirect writes in the write stage
//    }

    default : {
      setOperand1(*dec);
      setOperand2(*dec);

      if(dec->getDestination() == 0) regLastWritten = -1;
      else regLastWritten = dec->getDestination();
    }

  }

  return *dec;

}

/* Determine where to read the first operand from: RCET, ALU or registers */
void Decoder::setOperand1(DecodedInst& dec) {
  if(Registers::isChannelEnd(dec.getSource1())) {
//    dec.setOperand1(rcet.read(Registers::toChannelID(dec.getSource1())));
  }
  else {
    if(readALUOutput(dec.getSource1())) {
//      dec.setOperand1(alu.getLastResult()); // beware of timing?
      // Possibly send a tag to the execute stage so this can be done there
    }
    else {
//      dec.setOperand1(regs.read(dec.getSource1()));
    }
  }
}

/* Determine where to get second operand from: immediate, RCET, ALU or regs */
void Decoder::setOperand2(DecodedInst& dec) {
  if(InstructionMap::hasImmediate(dec.getOperation())) {
    dec.setOperand2(dec.getImmediate());
  }
  else if(Registers::isChannelEnd(dec.getSource2())) {
    //    dec.setOperand2(rcet.read(Registers::toChannelID(dec.getSource2())));
  }
  else {
    if(readALUOutput(dec.getSource2())) {
//      dec.setOperand1(alu.getLastResult()); // beware of timing?
      // Possibly send a tag to the execute stage so this can be done there
    }
    else {
//      dec.setOperand2(regs.read(dec.getSource2()));
    }
  }
}

/* Sends the second part of a two-flit store operation (the data to send). */
DecodedInst& Decoder::completeWrite(Instruction i) {
//  short readReg = instructionIn.read().getDest();
//  short op = instructionIn.read().getOp();
//  short remoteChannel = instructionIn.read().getRchannel();
//
//  regLastWritten = -1; // Hack - we're executing the same instruction twice
//
//  rChannel.write(remoteChannel);
//  setOperand1(op, readReg);
//  operation.write(InstructionMap::ST);
//  currentlyWriting = false;
//  stall.write(false);

  return *new DecodedInst(i);
}

/* Determine whether this instruction should be executed. */
bool Decoder::shouldExecute(short predBits) {

  bool result = (predBits == Instruction::ALWAYS) ||
                (predBits == Instruction::END_OF_PACKET) ||
                (predBits == Instruction::P     &&  predicate.read()) ||
                (predBits == Instruction::NOT_P && !predicate.read());

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
