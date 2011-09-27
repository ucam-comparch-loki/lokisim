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

    cout << this->name() << " select changed to " << select.read() << endl;

    if(selection >= 0) {
      next_trigger(validIn[selection].posedge_event());
      cout << this->name() << " waiting only for valid signal\n";
    }
    else               next_trigger(select.default_event());
  }
  else if(validIn[selection].read()) {  // Data arrived on the selected input
    dataOut.write(dataIn[selection].read());
    cout << this->name() << " received " << dataIn[selection].read() << endl;
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

void Multiplexer::handleAcks() {
  int selection = select.read();
  assert(selection < inputs());
  assert(selection >= 0);

  if(clock.posedge()) {
    // Clear the ack on the clock edge, and wait for the next ack to arrive.
    cout << this->name() << " waiting for ack\n";
    ackIn[selection].write(false);
    next_trigger(ackOut.posedge_event());
  }
  else {
    // When an ack arrives, forward it to the correct port.
    assert(ackOut.posedge());
cout << this->name() << " received ack - sent to output " << selection <<  " - waiting for clock\n";
    ackIn[selection].write(true);
    next_trigger(clock.posedge_event());
  }
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
  sensitive << ackOut.pos();
  dont_initialize();

}

Multiplexer::~Multiplexer() {
  delete[] dataIn;
  delete[] validIn;
  delete[] ackIn;
}
