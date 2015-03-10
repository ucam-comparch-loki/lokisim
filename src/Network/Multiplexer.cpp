/*
 * Multiplexer.cpp
 *
 *  Created on: 7 Sep 2011
 *      Author: db434
 */

#include "Multiplexer.h"

int Multiplexer::inputs() const {
  return iData.length();
}

void Multiplexer::reportStalls(ostream& os) {
  if (oData.valid()) {
    os << this->name() << " hasn't received ack for " << oData.read() << endl;
  }
}

void Multiplexer::handleData() {
  MuxSelect selection = iSelect.read();

  // 1. Invalid selection -> wait for valid selection
  // 2. Valid selection -> wait for data (if it isn't already here)
  // 3. Data arrived -> send data, wait for ack
  // 4. Ack arrived -> send ack, wait for select/data

  if (selection == NO_SELECTION)
    next_trigger(iSelect.default_event());
  else if (iData[selection].valid()) { // Data arrived on the selected input
    if (!haveSentData) {
      oData.write(iData[selection].read());
//      cout << this->name() << " sent " << dataIn[selection].read() << endl;
      next_trigger(oData.ack_event());
      haveSentData = true;
    }
    else {
      iData[selection].ack();
//      cout << this->name() << " acknowledged " << dataIn[selection].read() << endl;
      next_trigger(iSelect.default_event() | iData[selection].default_event());
      haveSentData = false;
    }
  }
  else if (iSelect.event()) {
    next_trigger(iData[selection].default_event());
  }
}

Multiplexer::Multiplexer(const sc_module_name& name, int numInputs) :
    Component(name) {

  assert(numInputs > 0);

  iData.init(numInputs);

  haveSentData = false;

  SC_METHOD(handleData);

}
