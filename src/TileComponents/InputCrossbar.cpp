/*
 * InputCrossbar.cpp
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#include "InputCrossbar.h"
#include "../Datatype/AddressedWord.h"
#include "../Network/FlowControl/FlowControlIn.h"
#include "../TileComponents/TileComponent.h"

InputCrossbar::InputCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs) :
    Component(name) {

  id = ID;
  firstInput    = TileComponent::inputChannelID(id, 0);
  numInputs     = inputs;
  numOutputs    = outputs;

  dataIn        = new sc_in<AddressedWord>[inputs];
  dataOut       = new sc_out<Word>[outputs];
  flowControlIn = new sc_in<int>[outputs];
  creditsOut    = new sc_out<AddressedWord>[inputs];
  readyIn       = new sc_in<bool>[outputs];
  readyOut      = new sc_out<bool>[inputs];

  // Create and wire up all flow control units.
  for(int i=0; i<outputs; i++) {
    FlowControlIn* fc = new FlowControlIn("fc_in", firstInput+i);
    flowControl.push_back(fc);

    // Currently assuming same number of inputs as outputs -- break this
    // assumption ASAP.
    fc->dataIn(dataIn[i]);
    fc->dataOut(dataOut[i]);
    fc->flowControlIn(flowControlIn[i]);
    fc->creditsOut(creditsOut[i]);
    fc->readyIn(readyIn[i]);
    fc->readyOut(readyOut[i]);
  }

}

InputCrossbar::~InputCrossbar() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] flowControlIn;
  delete[] creditsOut;
  delete[] readyIn;
  delete[] readyOut;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
