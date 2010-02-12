/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"

/* Simulate pipelining by only allowing signals through at the start of a cycle */
void ExecuteStage::newCycle() {
  while(true) {
    in1Select.write(op1Select.read());
    in2Select.write(op2Select.read());

    wait(0, sc_core::SC_NS);  // Allow time for the multiplexors to select values
    if(newOperation) ALUSelect.write(operation.read());

    writeOut.write(writeIn.read());
    indWriteOut.write(indWriteIn.read());
    remoteChannelOut.write(remoteChannelIn.read());
    if(newInst) remoteInstOut.write(remoteInstIn.read());
    newRChannelOut.write(newRChannelIn.read());

    newInst = newOperation = false;
    wait(clock.posedge_event());
  }
}

void ExecuteStage::receivedInstruction() {
  newInst = true;
}

void ExecuteStage::receivedOperation() {
  newOperation = true;
}

ExecuteStage::ExecuteStage(sc_core::sc_module_name name, int ID) :
    PipelineStage(name, ID),
    alu("alu", ID),
    in1Mux("ALUin1"),
    in2Mux("ALUin2") {

  newInst = false;
  newOperation = false;

// Register methods
  SC_METHOD(receivedInstruction);
  sensitive << remoteInstIn;
  dont_initialize();

  SC_METHOD(receivedOperation);
  sensitive << operation;
  dont_initialize();

// Connect everything up
  in1Mux.result(toALU1); alu.in1(toALU1);
  in2Mux.result(toALU2); alu.in2(toALU2);
  alu.out(output);

  in1Mux.select(in1Select);
  in2Mux.select(in2Select);
  alu.select(ALUSelect);

  in1Mux.in1(fromRChan1);
  in1Mux.in2(fromReg1);
  in1Mux.in3(fromALU1);

  in2Mux.in1(fromRChan2);
  in2Mux.in2(fromReg2);
  in2Mux.in3(fromALU2);
  in2Mux.in4(fromSExtend);

}

ExecuteStage::~ExecuteStage() {

}
