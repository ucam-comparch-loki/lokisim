/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"

/* Simulate pipelining by only allowing signals through at the start of a cycle */
void ExecuteStage::newCycle() {
  rcet1.write(fromRChan1.read());
  rcet2.write(fromRChan2.read());
  reg1.write(fromReg1.read());
  reg2.write(fromReg2.read());
  sExtend.write(fromSExtend.read());
  ALU1.write(fromALU1.read());
  ALU2.write(fromALU2.read());

  in1Select.write(op1Select.read());
  in2Select.write(op2Select.read());
  ALUSelect.write(operation.read());

  writeOut.write(writeIn.read());
  indWriteOut.write(indWriteIn.read());
  remoteInstOut.write(remoteInstIn.read());
}

ExecuteStage::ExecuteStage(sc_core::sc_module_name name, int ID) :
    PipelineStage(name, ID),
    alu("alu", ID),
    in1Mux("ALUin1"),
    in2Mux("ALUin2") {

// Connect everything up
  in1Mux.result(toALU1); alu.in1(toALU1);
  in2Mux.result(toALU2); alu.in2(toALU2);
  alu.out(output);

  in1Mux.select(in1Select);
  in2Mux.select(in2Select);
  alu.select(ALUSelect);

  in1Mux.in1(rcet1);
  in1Mux.in2(reg1);
  in1Mux.in3(ALU1);

  in2Mux.in1(rcet2);
  in2Mux.in2(reg2);
  in2Mux.in3(ALU2);
  in2Mux.in4(sExtend);

}

ExecuteStage::~ExecuteStage() {

}
