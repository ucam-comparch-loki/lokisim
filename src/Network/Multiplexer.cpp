/*
 * Multiplexer.cpp
 *
 *  Created on: 7 Sep 2011
 *      Author: db434
 */

#include "Multiplexer.h"

int Multiplexer::inputs() const {
  return dataIn.length();
}

void Multiplexer::handleData() {
  SelectType selection = select.read();

  // 1. Invalid selection -> wait for valid selection
  // 2. Valid selection -> wait for data (if it isn't already here)
  // 3. Data arrived -> send data, wait for ack
  // 4. Ack arrived -> send ack, wait for select/data

  if(selection == NO_SELECTION)
    next_trigger(select.default_event());
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
  else if(select.event()) {
    next_trigger(dataIn[selection].default_event());
  }
}

Multiplexer::Multiplexer(const sc_module_name& name, int numInputs) :
    Component(name) {

  dataIn.init(numInputs);

  SC_METHOD(handleData);

}
