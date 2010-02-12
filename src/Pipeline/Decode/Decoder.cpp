/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"
#include "../../InstructionMap.h"

void Decoder::doOp() {

  // TODO: tidy decode
  // TODO: determine when to send a remote instruction, and what it should be

  Instruction i = instructionIn.read();

  if(DEBUG) std::cout << "DecodeStage received Instruction: " << i << "\n";

  // Extract useful information from the instruction
  short operation = InstructionMap::operation(i.getOp());
  short destination = i.getDest();
  short operand1 = i.getSrc1();
  short operand2 = i.getSrc2();
  int immediate = i.getImmediate();
  short pred = i.getPredicate();
  short remoteChannel = i.getRchannel();

  predicate.write(pred);

  if(operation == InstructionMap::IRDR) {
    regAddr2.write(operand2);
    isIndirectRead.write(true);
  }
  else isIndirectRead.write(false);

  if(operation == InstructionMap::IWTR) indWrite.write(destination);
  else writeAddr.write(destination);

  // How do we know if data will be coming directly from the ALU?

  if(operation==InstructionMap::SETFETCHCH) {
    fetchChannel = immediate;   // Is it an immediate or read from a register?
  }

  // Send something to FetchLogic
  if(operation==InstructionMap::FETCH || operation == InstructionMap::FETCHPST) {
    // Need to modify once we have read the base address from the register file
    Address *a = new Address(immediate, fetchChannel);
    toFetchLogic.write(*a);
    regAddr1.write(destination);  // Fetch has an operand in the dest position
  }


  if(InstructionMap::isALUOperation(operation)) {
    this->operation.write(operation);
  }
  else this->operation.write(InstructionMap::NOP);

  // Determine where to read the first operand from: RCET or register file
  if(operand1 >= NUM_REGISTERS) {
    toRCET1.write(operand1 - NUM_REGISTERS);
    op1Select.write(0);         // ALU wants data from receive channel end table
  }
  else {
    if(operand1 == regLastWritten) {
      op1Select.write(2);       // ALU wants data from itself
    }
    else {
      regAddr1.write(operand1);
      op1Select.write(1);       // ALU wants data from registers
    }
  }

  // Determine where to get second operand from: immediate, RCET or registers
  if(InstructionMap::hasImmediate(operation)) {
    Data* d = new Data(immediate);
    toSignExtend.write(*d);
    op2Select.write(3);         // ALU wants data from sign extender
  }
  else if(operand2 >= NUM_REGISTERS) {
    toRCET2.write(operand2 - NUM_REGISTERS);
    op2Select.write(0);         // ALU wants data from receive channel end table
  }
  else {
    if(operand2 == regLastWritten) {
      op2Select.write(2);       // ALU wants data from itself
    }
    else {
      regAddr2.write(operand2);
      op2Select.write(1);       // ALU wants data from registers
    }
  }


  if(InstructionMap::hasRemoteChannel(operation)) {
    /*if(valid channel ID)*/ rChannel.write(remoteChannel);
    newRChannel.write(!newRChannel.read());
  }

  regLastWritten = destination;

}

Decoder::Decoder(sc_core::sc_module_name name, int ID) : Component(name, ID) {

  regLastWritten = -1;

  SC_METHOD(doOp);
  sensitive << instructionIn;
  dont_initialize();

}

Decoder::~Decoder() {

}
