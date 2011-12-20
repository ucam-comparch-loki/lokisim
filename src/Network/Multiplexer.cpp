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

  if(selection < 0)
    next_trigger(select.default_event());
  else if(select.event())
    next_trigger(dataIn[selection].default_event());
  else if(dataIn[selection].valid()) { // Data arrived on the selected input
    dataOut.write(dataIn[selection].read());
    dataIn[selection].ack();

    // Is it possible for the selection to change before the data goes invalid?
    next_trigger(select.default_event() | dataIn[selection].default_event());
  }
}

Multiplexer::Multiplexer(const sc_module_name& name, int numInputs) :
    Component(name),
    numInputs(numInputs) {

  dataIn = new DataInput[numInputs];

  SC_METHOD(handleData);

}

Multiplexer::~Multiplexer() {
  delete[] dataIn;
}
