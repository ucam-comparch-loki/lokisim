/*
 * StallRegister.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "StallRegister.h"
#include "../../Datatype/DecodedInst.h"

bool StallRegister::discard() {
  if(buffer.empty()) return false;
  else {
    buffer.discardTop();
    return true;
  }
}

void StallRegister::newCycle() {
  if(!buffer.empty() && readyIn.read() && localStageReady.read()) {
    dataOut.write(buffer.read());
  }
}

void StallRegister::newData() {
  assert(!buffer.full());
  buffer.write(dataIn.read());
  Instrumentation::stallRegUse(id);
}

void StallRegister::receivedReady() {
  readyOut.write(readyIn.read() && localStageReady.read());
}

StallRegister::StallRegister(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    buffer(2, std::string(name)) {  // Buffer with size 2 and a name for debug.

  SC_METHOD(newCycle);              // Behaves like:
  sensitive << clock.pos();         // always@(posedge clk)
  dont_initialize();                //   newCycle();

  SC_METHOD(newData);
  sensitive << dataIn;
  dont_initialize();

  SC_METHOD(receivedReady);
  sensitive << readyIn << localStageReady;
  // do initialise

//  readyOut.initialize(true);

  end_module();

}

StallRegister::~StallRegister() {

}
