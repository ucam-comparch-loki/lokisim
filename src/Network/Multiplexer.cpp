/*
 * Multiplexer.cpp
 *
 *  Created on: 7 Sep 2011
 *      Author: db434
 */

#include "Multiplexer.h"

int Multiplexer::inputs() const {
  return numInputs;
}

void Multiplexer::handleData() {
  int selection = select.read();
  assert(selection < inputs());

  if(validIn[selection].read()) {  // Data arrived on the selected input
    dataOut.write(dataIn[selection].read());
    validOut.write(true);
  }

  // Wait until the selection changes, or new data is received.
  next_trigger(select.default_event() | validIn[selection].posedge_event());
}

void Multiplexer::handleAcks() {
  int selection = select.read();
  assert(selection < inputs());

  // Just copy the acknowledgement through to the appropriate input whenever
  // it changes.
  assert(ackIn[selection].read() != ackOut.read());
  ackIn[selection].write(ackOut.read());
}

Multiplexer::Multiplexer(const sc_module_name& name, int numInputs) :
    Component(name),
    numInputs(numInputs) {

  dataIn  = new DataInput[numInputs];
  validIn = new ReadyInput[numInputs];
  ackIn   = new ReadyOutput[numInputs];

  SC_METHOD(handleData);
  sensitive << select;
  dont_initialize();

  SC_METHOD(handleAcks);
  sensitive << ackOut;
  dont_initialize();

}

Multiplexer::~Multiplexer() {
  delete[] dataIn;
  delete[] validIn;
  delete[] ackIn;
}
