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
    if(!stall.read()) {
      COPY_IF_NEW(op1Select, in1Select);
      COPY_IF_NEW(op2Select, in2Select);

      wait(0, sc_core::SC_NS);  // Allow time for the multiplexors to select values
      COPY_IF_NEW(operation, ALUSelect);

      COPY_IF_NEW(remoteInstIn, remoteInstOut);
      COPY_IF_NEW(writeIn, writeOut);
      COPY_IF_NEW(indWriteIn, indWriteOut);
      COPY_IF_NEW(remoteChannelIn, remoteChannelOut);
      COPY_IF_NEW(memoryOpIn, memoryOpOut);
      COPY_IF_NEW(waitOnChannelIn, waitOnChannelOut);

      setPredSig.write(setPredicate.read());  // Not using COPY_IF_NEW
    }
    wait(clock.posedge_event());
  }
}

ExecuteStage::ExecuteStage(sc_module_name name) :
    PipelineStage(name),
    alu("alu"),
    in1Mux("ALUin1"),
    in2Mux("ALUin2") {

// Connect everything up
  in1Mux.result(toALU1); alu.in1(toALU1);
  in2Mux.result(toALU2); alu.in2(toALU2);
  alu.out(output);
  alu.setPredicate(setPredSig);
  alu.predicate(predicate);

  in1Mux.select(in1Select);
  in2Mux.select(in2Select);
  alu.operation(ALUSelect);

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
