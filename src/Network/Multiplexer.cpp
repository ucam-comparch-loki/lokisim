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
  SelectType selection = select.read();

  // 1. Invalid selection -> wait for valid selection
  // 2. Valid selection -> wait for data
  // 3. Data arrived -> send data, wait for ack
  // 4. Ack arrived -> send ack, wait for select/data

  if(selection == NO_SELECTION)
    next_trigger(select.default_event());
  else if(select.event())
    next_trigger(dataIn[selection].default_event());
  else if(dataIn[selection].valid()) { // Data arrived on the selected input
    if(!haveSentData) {
      dataOut.write(dataIn[selection].read());
      next_trigger(dataOut.ack_event());
      haveSentData = true;
    }
    else {
      dataIn[selection].ack();
      next_trigger(select.default_event() | dataIn[selection].default_event());
      haveSentData = false;
    }
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
