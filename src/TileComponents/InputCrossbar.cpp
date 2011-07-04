/*
 * InputCrossbar.cpp
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#include "InputCrossbar.h"
#include "../Datatype/AddressedWord.h"
#include "../Network/FlowControl/FlowControlIn.h"
#include "../Network/Topologies/Crossbar.h"
#include "../Network/UnclockedNetwork.h"
#include "../TileComponents/TileComponent.h"

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID, int inputs, int outputs) :
    Component(name, ID),
    creditNet("credit", ID, outputs, inputs, inputs, Network::NONE, Dimension(1.0/CORES_PER_TILE, 0.05)),
    dataNet("data", ID, inputs, outputs, 1, Network::CHANNEL, Dimension(1.0/CORES_PER_TILE, 0.05)) {

  creditNet.initialise();
  dataNet.initialise();

  firstInput       = ChannelID(id, 0);
  numInputs        = inputs;
  numOutputs       = outputs;

  dataIn           = new DataInput[inputs];
  validDataIn      = new ReadyInput[inputs];
  ackDataIn        = new ReadyOutput[inputs];

  dataOut          = new sc_out<Word>[outputs];

  bufferHasSpace   = new sc_in<bool>[outputs];

  creditsOut       = new CreditOutput[inputs];
  validCreditOut   = new ReadyOutput[inputs];
  ackCreditOut     = new ReadyInput[inputs];

  dataToBuffer     = new sc_buffer<DataType>[outputs];
  creditsToNetwork = new sc_buffer<CreditType>[outputs];
  readyForData     = new sc_signal<ReadyType>[outputs];
  readyForCredit   = new sc_signal<ReadyType>[outputs];
  validData        = new sc_signal<ReadyType>[outputs];
  validCredit      = new sc_signal<ReadyType>[outputs];

  creditNet.clock(creditClock);
  dataNet.clock(dataClock);

  for(int i=0; i<inputs; i++) {
    dataNet.dataIn[i](dataIn[i]);
    dataNet.validDataIn[i](validDataIn[i]);
    dataNet.ackDataIn[i](ackDataIn[i]);
    creditNet.dataOut[i](creditsOut[i]);
    creditNet.validDataOut[i](validCreditOut[i]);
    creditNet.ackDataOut[i](ackCreditOut[i]);
  }

  // Create and wire up all flow control units.
  for(int i=0; i<outputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_gen_unique_name("fc_in"), firstInput.getComponentID(), firstInput.addChannel(i, outputs));
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->dataOut(dataOut[i]);
    fc->bufferHasSpace(bufferHasSpace[i]);

    fc->dataIn(dataToBuffer[i]);
    fc->validDataIn(validData[i]);
    fc->ackDataIn(readyForData[i]);
    fc->creditsOut(creditsToNetwork[i]);
    fc->validCreditOut(validCredit[i]);
    fc->ackCreditOut(readyForCredit[i]);

    dataNet.dataOut[i](dataToBuffer[i]);
    dataNet.validDataOut[i](validData[i]);
    dataNet.ackDataOut[i](readyForData[i]);
    creditNet.dataIn[i](creditsToNetwork[i]);
    creditNet.validDataIn[i](validCredit[i]);
    creditNet.ackDataIn[i](readyForCredit[i]);
  }
}

InputCrossbar::~InputCrossbar() {
  delete[] dataIn;
  delete[] validDataIn;
  delete[] ackDataIn;
  delete[] dataOut;
  delete[] bufferHasSpace;
  delete[] creditsOut;
  delete[] validCreditOut;
  delete[] ackCreditOut;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
