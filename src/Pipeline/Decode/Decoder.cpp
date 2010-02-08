/*
 * Decoder.cpp
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#include "Decoder.h"
#include "../../InstructionMap.h"

void Decoder::doOp() {

  // TODO: deal with NULL instruction
  // TODO: tidy decode
  // TODO: send the predicate and remote channel along the pipeline

  Instruction i = instruction.read();

  // Extract useful information from the instruction
  short operation = InstructionMap::operation(i.getOp());
  short destination = i.getDest();
  short operand1 = i.getSrc1();
  short operand2 = i.getSrc2();   // May not be valid -- depends on instruction
  int immediate = i.getImmediate();
  short predicate = i.getPredicate();
  short remoteChannel = i.getRchannel();

  /*if(operation is an ALU operation)*/ this->operation.write(operation);
  if(operation == InstructionMap::IRDR) {
    indRead.write(operand2);
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
    operand1 = destination;  // Fetches have an operand in the destination position
  }

  // Determine where to read the first operand from: RCET or register file
  if(operand1 >= NUM_REGISTERS) {
    toRCET1.write(operand1 - NUM_REGISTERS);
    op1Select.write(0);         // ALU wants data from receive channel end table
  }
  else {
    regAddr1.write(operand1);
    op1Select.write(1);         // ALU wants data from registers
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
    regAddr2.write(operand2);
    op2Select.write(1);         // ALU wants data from registers
  }

  // TODO: Do something if the instruction specifies a remote channel



}

Decoder::Decoder(sc_core::sc_module_name name, int ID) : Component(name, ID) {

  SC_METHOD(doOp);
  sensitive << instruction;

}

Decoder::~Decoder() {

}
