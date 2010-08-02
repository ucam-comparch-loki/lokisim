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

typedef IndirectRegisterFile Registers;

void Decoder::decodeInstruction() {

  // TODO: tidy decode

  Instruction i = instructionIn.read();

  if(DEBUG) cout << this->name() << " received Instruction: " << i << endl;

  if(currentlyWriting) {
    completeWrite();
    return;
  }

  // Extract useful information from the instruction
  short operation     = InstructionMap::operation(i.getOp());
  short destination   = i.getDest();
  short operand1      = i.getSrc1();
  short operand2      = i.getSrc2();
  int   immediate     = i.getImmediate();
  short pred          = i.getPredicate();
  bool  setPred       = i.getSetPredicate();
  short remoteChannel = i.getRchannel();

  bool execute = shouldExecute(pred);

  Instrumentation::operation(operation, execute);

  // If we are in remote execution mode, send all marked instructions.
  if(remoteExecute) {
    if(pred == Instruction::P) {
      i.setPredicate(Instruction::END_OF_PACKET); // Set it to always execute
      instructionOut.write(i);
      rChannel.write(sendChannel);
      return;
    }
    // Drop out of remote execution mode when we find an unmarked instruction.
    else {
      remoteExecute = false;
      if(DEBUG) cout << this->name() << " ending remote execution" << endl;
    }
  }

  // COMMON CAUSE OF PROBLEMS: INSTRUCTION NOT EXECUTED BECAUSE OF PREDICATE
  if(!execute) {
    if(DEBUG) cout << this->name() << " not executing instruction" << endl;
    return;
  }

  // Send the remote channel to the write stage
  if(InstructionMap::hasRemoteChannel(operation)) {
    if(remoteChannel != Instruction::NO_CHANNEL) rChannel.write(remoteChannel);
  }

  setPredicate.write(setPred);
  usePredicate.write(pred);

  if(operation == InstructionMap::LD || operation == InstructionMap::LDB) {
    setOperand1(operation, destination);
    setOperand2(operation, 0, immediate);
    setDestination(0); // Don't want to write
    this->operation.write(InstructionMap::ORI);
    memoryOp.write(MemoryRequest::LOAD);
    return;
  }

  if(operation == InstructionMap::ST || operation == InstructionMap::STB) {
    setOperand1(operation, operand1);
    setOperand2(operation, 0, immediate);
    setDestination(0);
    this->operation.write(InstructionMap::ORI);
    stall.write(true);
    currentlyWriting = true;
    memoryOp.write(MemoryRequest::STORE);
    return;
  }

  if(operation == InstructionMap::STADDR || operation == InstructionMap::STBADDR) {
    setOperand1(operation, destination);
    setOperand2(operation, 0, immediate);
    setDestination(0);
    this->operation.write(InstructionMap::ORI);
    memoryOp.write(MemoryRequest::STADDR);
    return;
  }

  if(operation == InstructionMap::TSTCH) {
    channelOp.write(ReceiveChannelEndTable::TSTCH);
    setDestination(destination);
    toRCET1.write(Registers::toChannelID(operand1));
    op1Select.write(RCET);
    this->operation.write(operation);
    return;
  }

  if(operation == InstructionMap::SELCH) {
    channelOp.write(ReceiveChannelEndTable::SELCH);
    setDestination(destination);
    op1Select.write(RCET);
    this->operation.write(operation);
    return;
  }

  if(operation == InstructionMap::IBJMP) {
    jumpOffset.write(immediate);
    return;
  }

  if(operation == InstructionMap::WOCHE) {
    waitOnChannel.write(immediate);
    return;
  }

  if(operation == InstructionMap::RMTEXECUTE) {
    remoteExecute = true;
    sendChannel = remoteChannel;
    if(DEBUG) cout << this->name() << " beginning remote execution" << endl;
    return;
  }

  // Remote instructions
  if(operation>=InstructionMap::RMTFETCH && operation<=InstructionMap::RMTNXIPK) {
    Instruction inst;
    std::string opName;
    inst.setDest(destination);
    inst.setImmediate(immediate);
    inst.setPredicate(Instruction::END_OF_PACKET);

    switch(operation) {
      case InstructionMap::RMTFETCH :
        opName = "fetch";
        inst.setOp(InstructionMap::opcode(opName));
        break;
      case InstructionMap::RMTFETCHPST :
        opName = "fetchpst";
        inst.setOp(InstructionMap::opcode(opName));
        break;
//      case InstructionMap::RMTFILL :
//        inst.setOp(???);
//      case InstructionMap::RMTNXIPK :
//        inst.setOp(???);
      default: cerr<<"Haven't implemented instruction "<<operation<<" yet."<<endl;
    }

    instructionOut.write(inst);
    rChannel.write(remoteChannel);

    return; // Skip the rest of this complicated method
  }

  if(operation==InstructionMap::SETFETCHCH) {
    fetchChannel = immediate;   // Is it an immediate or read from a register?
  }

  // Send something to FetchLogic
  if(operation>=InstructionMap::FETCH && operation <= InstructionMap::FETCHPST) {

    // Need to modify once we have read the base address from the register file
    Address a(immediate, fetchChannel);
    toFetchLogic.write(a);

    // Fetch doesn't store anything, so has operands in the destination position
    if(operation == InstructionMap::PSELFETCH) {
      regAddr1.write(predicate.read() ? destination : operand1);
    }
    else regAddr1.write(destination);

    persistent.write(operation == InstructionMap::FETCHPST);

    return;
  }

  if(InstructionMap::isALUOperation(operation)) {
    this->operation.write(operation);
  }

  setOperand1(operation, operand1);
  setOperand2(operation, operand2, immediate);

  if(operation == InstructionMap::IRDR) {
    regAddr2.write(operand1);
    isIndirectRead.write(true);
  }
  else isIndirectRead.write(false);

  if(operation == InstructionMap::IWTR) indWrite.write(destination);
  else setDestination(destination);

}

/* Determine where to read the first operand from: RCET, ALU or registers */
void Decoder::setOperand1(short operation, int operand) {
  if(Registers::isChannelEnd(operand)) {
    toRCET1.write(Registers::toChannelID(operand));
    op1Select.write(RCET);          // ALU wants data from channel-end
  }
  else {
    if(readALUOutput(operand)) {
      op1Select.write(ALU);         // ALU wants data from itself
    }
    else {
      regAddr1.write(operand);
      op1Select.write(REGISTERS);   // ALU wants data from registers
    }
  }
}

/* Determine where to get second operand from: immediate, RCET, ALU or regs */
void Decoder::setOperand2(short operation, int operand, int immediate) {
  if(InstructionMap::hasImmediate(operation)) {
    toSignExtend.write(Data(immediate));
    op2Select.write(SIGN_EXTEND);   // ALU wants data from sign extender
  }
  else if(Registers::isChannelEnd(operand)) {
    toRCET2.write(Registers::toChannelID(operand));
    op2Select.write(RCET);          // ALU wants data from channel-end
  }
  else {
    if(readALUOutput(operand)) {
      op2Select.write(ALU);         // ALU wants data from itself
    }
    else {
      regAddr2.write(operand);
      op2Select.write(REGISTERS);   // ALU wants data from registers
    }
  }
}

void Decoder::setDestination(int destination) {
  writeAddr.write(destination);

  if(destination == 0) regLastWritten = -1;
  else regLastWritten = destination;
}

/* Sends the second part of a two-flit store operation (the data to send). */
void Decoder::completeWrite() {
  short readReg = instructionIn.read().getDest();
  short op = instructionIn.read().getOp();
  short remoteChannel = instructionIn.read().getRchannel();

  regLastWritten = -1; // Hack - we're executing the same instruction twice

  rChannel.write(remoteChannel);
  setOperand1(op, readReg);
  operation.write(InstructionMap::ST);
  currentlyWriting = false;
  stall.write(false);
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

Decoder::Decoder(sc_module_name name) : Component(name) {

  regLastWritten = sendChannel = fetchChannel = -1;
  remoteExecute = false;

  SC_METHOD(decodeInstruction);
  sensitive << instructionIn;
  dont_initialize();

}

Decoder::~Decoder() {

}
