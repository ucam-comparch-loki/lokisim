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

  if(select.event()) {
    if(validOut.read())
      validOut.write(false);

    if(selection >= 0) next_trigger(validIn[selection].posedge_event());
    else               next_trigger(select.default_event());
  }
  else if(validIn[selection].read()) {  // Data arrived on the selected input
    dataOut.write(dataIn[selection].read());
    validOut.write(true);

    // Is it possible for the selection to change before the data goes invalid?
    next_trigger(select.default_event() | validIn[selection].negedge_event());
  }
  else {                                // The input data is no longer valid
    validOut.write(false);

    // Wait until the selection changes, or new data is received.
    next_trigger(select.default_event() | validIn[selection].posedge_event());
  }
}

Multiplexer::Multiplexer(const sc_module_name& name, int numInputs) :
    Component(name),
    numInputs(numInputs) {

  dataIn  = new DataInput[numInputs];
  validIn = new ReadyInput[numInputs];

  SC_METHOD(handleData);
  sensitive << select;
  dont_initialize();

}

Multiplexer::~Multiplexer() {
  delete[] dataIn;
  delete[] validIn;
}
