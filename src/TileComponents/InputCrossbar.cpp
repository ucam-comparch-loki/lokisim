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

void InputCrossbar::dataArrived() {
  newData.notify();
}

void InputCrossbar::sendData() {
  while(true) {
    // We want to send data on to the buffers as soon as it arrives over the
    // network.
    wait(newData);

    // Generate a negative edge to trigger sending of data.
    sendDataSig.write(false);
    wait(sc_core::SC_ZERO_TIME);
    sendDataSig.write(true);
  }
}

InputCrossbar::InputCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs) :
    Component(name),
    dataXbar("data", ID, inputs, outputs, 1, 1, TileComponent::inputChannelID(ID,0)),
    creditXbar("credit", ID, outputs, inputs, inputs, TOTAL_OUTPUT_CHANNELS, 0) {

  id = ID;
  firstInput     = TileComponent::inputChannelID(id, 0);
  numInputs      = inputs;
  numOutputs     = outputs;

  dataIn         = new DataInput[inputs];
  dataOut        = new sc_out<Word>[outputs];
  creditsIn      = new sc_in<int>[outputs];
  creditsOut     = new CreditOutput[inputs];
  canSendCredits = new ReadyInput[inputs];
  canReceiveData = new ReadyOutput[inputs];

  dataToBuffer   = new sc_buffer<DataType>[outputs];
  creditsToNetwork = new sc_buffer<CreditType>[outputs];
  readyForData   = new sc_signal<ReadyType>[outputs];
  readyForCredit = new sc_signal<ReadyType>[outputs];

  creditXbar.clock(clock);
  dataXbar.clock(sendDataSig);
  sendDataSig.write(true);

  for(int i=0; i<inputs; i++) {
    dataXbar.dataIn[i](dataIn[i]);
    dataXbar.canReceiveData[i](canReceiveData[i]);
    creditXbar.dataOut[i](creditsOut[i]);
    creditXbar.canSendData[i](canSendCredits[i]);
  }

  // Create and wire up all flow control units.
  for(int i=0; i<outputs; i++) {
    FlowControlIn* fc = new FlowControlIn("fc_in", firstInput+i);
    flowControl.push_back(fc);

    fc->dataOut(dataOut[i]);
    fc->creditsIn(creditsIn[i]);

    fc->dataIn(dataToBuffer[i]);
    fc->canReceiveData(readyForData[i]);
    fc->creditsOut(creditsToNetwork[i]);
    fc->canSendCredits(readyForCredit[i]);

    dataXbar.dataOut[i](dataToBuffer[i]);
    dataXbar.canSendData[i](readyForData[i]);
    creditXbar.dataIn[i](creditsToNetwork[i]);
    creditXbar.canReceiveData[i](readyForCredit[i]);
  }

  SC_METHOD(dataArrived);
  for(int i=0; i<inputs; i++) sensitive << dataIn[i];
  dont_initialize();

  SC_THREAD(sendData);

}

InputCrossbar::~InputCrossbar() {
  delete[] dataIn;
  delete[] dataOut;
  delete[] creditsIn;
  delete[] creditsOut;
  delete[] canSendCredits;
  delete[] canReceiveData;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
