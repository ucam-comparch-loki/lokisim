/*
 * StallRegister.cpp
 *
 *  Created on: 25 Oct 2010
 *      Author: db434
 */

#include "StallRegister.h"
#include "../../Datatype/DecodedInst.h"

void StallRegister::newCycle() {
  if(!buffer.isEmpty() && readyIn.read()) {
    dataOut.write(buffer.read());
  }
}

void StallRegister::newData() {
  // We only allow data in when there is space for it, so we know that this
  // is safe.
  buffer.write(dataIn.read());
}

void StallRegister::receivedReady() {
  readyOut.write(readyIn.read());
}

StallRegister::StallRegister(sc_module_name name) :
    Component(name),
    buffer(2, std::string(name)) {  // Buffer with size 2 and a name for debug.

  SC_METHOD(newCycle);              // Behaves like:
  sensitive << clock.pos();         // always@(posedge clk)
  dont_initialize();                //   newCycle();

  SC_METHOD(newData);
  sensitive << dataIn;
  dont_initialize();

  SC_METHOD(receivedReady);
  sensitive << readyIn;
  // do initialise

}

StallRegister::~StallRegister() {

}
