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
    bufferFillChanged.notify();
    return true;
  }
}

void StallRegister::newCycle() {
  // Before we can send an instruction, we must wait until:
  //  * we have an instruction
  //  * the next stall register and the next pipeline stage are ready
  //  * a positive clock edge

  // This isn't very efficient, but simplifying it seems to break things somehow.
  if(buffer.empty())               next_trigger(bufferFillChanged);
  else if(!clock.posedge())        next_trigger(clock.posedge_event());
  else if(!readyIn.read())         next_trigger(readyIn.posedge_event());
  else if(!localStageReady.read()) next_trigger(localStageReady.posedge_event());
  else {
    dataOut.write(buffer.read());
    bufferFillChanged.notify();
    next_trigger(clock.posedge_event());
  }
}

void StallRegister::newData() {
  assert(!buffer.full());
  buffer.write(dataIn.read());
  bufferFillChanged.notify();
  Instrumentation::stallRegUse(id);
}

void StallRegister::receivedReady() {
  readyOut.write(readyIn.read() && localStageReady.read() && !buffer.full());
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
  sensitive << readyIn << localStageReady << bufferFillChanged;//clock.pos();
  // do initialise

//  readyOut.initialize(true);

  end_module();

}

StallRegister::~StallRegister() {

}
