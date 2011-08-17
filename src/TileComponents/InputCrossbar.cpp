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
#include "../TileComponents/TileComponent.h"

const unsigned int InputCrossbar::numInputs = CORE_INPUT_PORTS;
const unsigned int InputCrossbar::numOutputs = CORE_INPUT_CHANNELS;

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    firstInput(ChannelID(id,0)),
    creditNet("credit", ID, numOutputs, 1, 1, Network::NONE, Dimension(1.0/CORES_PER_TILE, 0.05)),
    dataNet("data", ID, numInputs, numOutputs, 1, Network::CHANNEL, Dimension(1.0/CORES_PER_TILE, 0.05)) {

  creditNet.initialise();
  dataNet.initialise();

  dataIn           = new DataInput[numInputs];
  validDataIn      = new ReadyInput[numInputs];
  ackDataIn        = new ReadyOutput[numInputs];

  dataOut          = new sc_out<Word>[numOutputs];

  bufferHasSpace   = new sc_in<bool>[numOutputs];

  // Possibly temporary: have only one credit output port, used for sending
  // credits to other tiles. Credits aren't used for local communication.
  creditsOut       = new CreditOutput[1];
  validCreditOut   = new ReadyOutput[1];
  ackCreditOut     = new ReadyInput[1];

  dataToBuffer     = new sc_buffer<DataType>[numOutputs];
  creditsToNetwork = new sc_buffer<CreditType>[numOutputs];
  ackDataSig       = new sc_signal<ReadyType>[numOutputs];
  ackCreditSig     = new sc_signal<ReadyType>[numOutputs];
  validDataSig     = new sc_signal<ReadyType>[numOutputs];
  validCreditSig   = new sc_signal<ReadyType>[numOutputs];


  // Wire up the small networks.
  creditNet.clock(creditClock);
  dataNet.clock(dataClock);

  for(unsigned int i=0; i<numInputs; i++) {
    dataNet.dataIn[i](dataIn[i]);
    dataNet.validDataIn[i](validDataIn[i]);
    dataNet.ackDataIn[i](ackDataIn[i]);
  }

  creditNet.dataOut[0](creditsOut[0]);
  creditNet.validDataOut[0](validCreditOut[0]);
  creditNet.ackDataOut[0](ackCreditOut[0]);

  // Create and wire up all flow control units.
  for(unsigned int i=0; i<numOutputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_gen_unique_name("fc_in"), firstInput.getComponentID(), firstInput.addChannel(i, numOutputs));
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->dataOut(dataOut[i]);
    fc->bufferHasSpace(bufferHasSpace[i]);

    fc->dataIn(dataToBuffer[i]);
    fc->validDataIn(validDataSig[i]);
    fc->ackDataIn(ackDataSig[i]);
    fc->creditsOut(creditsToNetwork[i]);
    fc->validCreditOut(validCreditSig[i]);
    fc->ackCreditOut(ackCreditSig[i]);

    dataNet.dataOut[i](dataToBuffer[i]);
    dataNet.validDataOut[i](validDataSig[i]);
    dataNet.ackDataOut[i](ackDataSig[i]);
    creditNet.dataIn[i](creditsToNetwork[i]);
    creditNet.validDataIn[i](validCreditSig[i]);
    creditNet.ackDataIn[i](ackCreditSig[i]);
  }
}

InputCrossbar::~InputCrossbar() {
  delete[] dataIn;            delete[] validDataIn;     delete[] ackDataIn;
  delete[] creditsOut;        delete[] validCreditOut;  delete[] ackCreditOut;
  delete[] dataOut;
  delete[] bufferHasSpace;

  delete[] dataToBuffer;      delete[] validDataSig;    delete[] ackDataSig;
  delete[] creditsToNetwork;  delete[] validCreditSig;  delete[] ackCreditSig;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
