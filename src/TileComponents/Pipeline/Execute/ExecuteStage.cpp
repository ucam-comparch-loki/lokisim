/*
 * ExecuteStage.cpp
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#include "ExecuteStage.h"

double ExecuteStage::area() const {
  return alu.area() + in1Mux.area() + in2Mux.area();
}

double ExecuteStage::energy() const {
  return alu.energy() + in1Mux.energy() + in2Mux.energy();
}

/* Simulate pipelining by only allowing signals through at the start of a cycle */
void ExecuteStage::newCycle() {
  while(true) {
    if(!readyIn.read()) {
      readyOut.write(false);
    }
    else if(!stall.read()) {
      if(operation.event()) {
        DecodedInst op = operation.read();
        bool success = alu.execute(op);

        if(success) result.write(op);

        readyOut.write(true); // readyOut.write(alu.busy())?
        idle.write(false);
      }
      else {
        idle.write(true);
        readyOut.write(true);
      }
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
//  in1Mux.result(toALU1); alu.in1(toALU1);
//  in2Mux.result(toALU2); alu.in2(toALU2);
//  alu.out(output);
//  alu.setPredicate(setPredSig);
//  alu.usePredicate(usePredSig);
//  alu.predicate(predicate);
//
//  in1Mux.select(in1Select);
//  in2Mux.select(in2Select);
//  alu.operation(ALUSelect);
//
//  in1Mux.in1(fromRChan1);
//  in1Mux.in2(fromReg1);
//  in1Mux.in3(fromALU1);
//
//  in2Mux.in1(fromRChan2);
//  in2Mux.in2(fromReg2);
//  in2Mux.in3(fromALU2);
//  in2Mux.in4(fromSExtend);

}

ExecuteStage::~ExecuteStage() {

}
