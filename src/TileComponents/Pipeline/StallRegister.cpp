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
    if(buffer.full()) bufferFillChanged.notify();
    bufferContentsChanged.notify();

    buffer.discardTop();
    return true;
  }
}

void StallRegister::newCycle() {
  // Before we can send an instruction, we must wait until:
  //  * we have an instruction
  //  * the next stall register and the next pipeline stage are ready
  //  * a positive clock edge

  // This isn't very efficient, but simplifying it seems to break things somehow.
  if(buffer.empty())               next_trigger(bufferContentsChanged);
  else if(!clock.posedge())        next_trigger(clock.posedge_event());
  else if(!readyIn.read())         next_trigger(readyIn.posedge_event());
  else if(!localStageReady.read()) next_trigger(localStageReady.posedge_event());
  else {
    // If the buffer is full, it will soon not be full, so trigger the event.
    if(buffer.full()) bufferFillChanged.notify();
    bufferContentsChanged.notify();

    dataOut.write(buffer.read());

    next_trigger(clock.posedge_event());
  }
}

void StallRegister::newData() {
  assert(!buffer.full());
  buffer.write(dataIn.read());

  if(buffer.full()) bufferFillChanged.notify();
  bufferContentsChanged.notify();

  Instrumentation::stallRegUse(id);
}

void StallRegister::receivedReady() {
  if(clock.posedge()) {
    bool newReady = readyIn.read() && localStageReady.read() && !buffer.full();

    if(ready != newReady) {
      ready = newReady;
      readyOut.write(ready);
    }

    next_trigger(readyIn.default_event() | localStageReady.default_event() | bufferFillChanged);
  }
  else {
    // Wait until the clock edge to update the ready signal. If we flip to not
    // being ready too quickly, no instructions ever get buffered up, reducing
    // performance.
    next_trigger(clock.posedge_event());
  }
}

StallRegister::StallRegister(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    buffer(2, std::string(name)) {  // Buffer with size 2 and a name for debug.

  SC_METHOD(newCycle);
  // do initialise

  SC_METHOD(newData);
  sensitive << dataIn;
  dont_initialize();

  SC_METHOD(receivedReady);
  // do initialise

  end_module();

}
