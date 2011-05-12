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

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID, int inputs, int outputs) :
    Component(name, ID),
    dataXbar("data", ID, inputs, outputs, 1, 1, ChannelID(ID, 0), Dimension(1.0/CORES_PER_TILE, 0.05)),
    creditXbar("credit", ID, outputs, inputs, inputs, TOTAL_OUTPUT_CHANNELS, ChannelID(0), Dimension(1.0/CORES_PER_TILE, 0.05)) {

  firstInput       = ChannelID(id, 0);
  numInputs        = inputs;
  numOutputs       = outputs;

  dataIn           = new DataInput[inputs];
  validDataIn      = new ReadyInput[inputs];
  ackDataIn        = new ReadyOutput[inputs];

  dataOut          = new sc_out<Word>[outputs];

  creditsIn        = new sc_in<int>[outputs];

  creditsOut       = new CreditOutput[inputs];
  validCreditOut   = new ReadyOutput[inputs];
  ackCreditOut     = new ReadyInput[inputs];

  dataToBuffer     = new sc_buffer<DataType>[outputs];
  creditsToNetwork = new sc_buffer<CreditType>[outputs];
  readyForData     = new sc_signal<ReadyType>[outputs];
  readyForCredit   = new sc_signal<ReadyType>[outputs];
  validData        = new sc_signal<ReadyType>[outputs];
  validCredit      = new sc_signal<ReadyType>[outputs];

  creditXbar.clock(clock);
  dataXbar.clock(sendDataSig);
  sendDataSig.write(true);

  for(int i=0; i<inputs; i++) {
    dataXbar.dataIn[i](dataIn[i]);
    dataXbar.validDataIn[i](validDataIn[i]);
    dataXbar.ackDataIn[i](ackDataIn[i]);
    creditXbar.dataOut[i](creditsOut[i]);
    creditXbar.validDataOut[i](validCreditOut[i]);
    creditXbar.ackDataOut[i](ackCreditOut[i]);
  }

  // Create and wire up all flow control units.
  for(int i=0; i<outputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_core::sc_gen_unique_name("fc_in"), firstInput.getComponentID(), firstInput.addChannel(i, outputs));
    flowControl.push_back(fc);

    fc->dataOut(dataOut[i]);
    fc->creditsIn(creditsIn[i]);

    fc->dataIn(dataToBuffer[i]);
    fc->validDataIn(validData[i]);
    fc->ackDataIn(readyForData[i]);
    fc->creditsOut(creditsToNetwork[i]);
    fc->validCreditOut(validCredit[i]);
    fc->ackCreditOut(readyForCredit[i]);

    dataXbar.dataOut[i](dataToBuffer[i]);
    dataXbar.validDataOut[i](validData[i]);
    dataXbar.ackDataOut[i](readyForData[i]);
    creditXbar.dataIn[i](creditsToNetwork[i]);
    creditXbar.validDataIn[i](validCredit[i]);
    creditXbar.ackDataIn[i](readyForCredit[i]);
  }

  SC_METHOD(dataArrived);
  for(int i=0; i<inputs; i++) sensitive << dataIn[i];
  dont_initialize();

  SC_THREAD(sendData);

}

InputCrossbar::~InputCrossbar() {
  delete[] dataIn;
  delete[] validDataIn;
  delete[] ackDataIn;

  delete[] dataOut;

  delete[] creditsIn;

  delete[] creditsOut;
  delete[] validCreditOut;
  delete[] ackCreditOut;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
