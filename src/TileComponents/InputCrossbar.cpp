/*
 * InputCrossbar.cpp
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "InputCrossbar.h"
#include "../Datatype/AddressedWord.h"
#include "../Network/FlowControl/FlowControlIn.h"
#include "../Network/Topologies/Crossbar.h"
#include "../TileComponents/TileComponent.h"

const unsigned int InputCrossbar::numInputs = CORE_INPUT_PORTS;
const unsigned int InputCrossbar::numOutputs = CORE_INPUT_CHANNELS;

void InputCrossbar::newData(PortIndex input) {
  assert(validDataIn[input].read());

  // We have new data - forward it to the correct channel end.
  const AddressedWord& data = dataIn[input].read();
  ChannelIndex destination = data.channelID().getChannel();

  if(destination >= numOutputs) cout << "Trying to send to " << data.channelID() << endl;
  assert(destination < numOutputs);

  // Trigger a method which will send the data.
  dataSource[destination] = input;
  sendData[destination].notify();
}

void InputCrossbar::writeToBuffer(ChannelIndex output) {
  if(clock.posedge()) {
    // Invalidate data on the clock edge.
    assert(validDataSig[output].read() == true);
    validDataSig[output].write(false);

    // Wait until there is new data to send.
    next_trigger(sendData[output]);
  }
  else {
    // There is data to send.
    PortIndex source = dataSource[output];
    dataToBuffer[output].write(dataIn[source].read());
    validDataSig[output].write(true);

    // Wait until the clock edge to invalidate the data.
    next_trigger(clock.posedge_event());
  }
}

void InputCrossbar::readyChanged() {
  for(unsigned int i=0; i<numOutputs; i++) {
    if(!bufferHasSpace[i].read()) {
      // We have found a buffer which can't receive any more data. Since we
      // don't know which buffer new data will be heading to, we must block
      // all new data until there is space again.
      readyOut.write(false);
      return;
    }
  }

  // If we have got this far, all buffers have space.
  readyOut.write(true);
}

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    firstInput(ChannelID(id,0)),
    creditNet("credit", ID, numOutputs, 1, 1, Network::NONE, Dimension(1.0/CORES_PER_TILE, 0.05)),
    dataSource(numOutputs) {

  creditNet.initialise();

  dataIn           = new DataInput[numInputs];
  validDataIn      = new ReadyInput[numInputs];

  dataOut          = new sc_out<Word>[numOutputs];
  bufferHasSpace   = new sc_in<bool>[numOutputs];

  // Possibly temporary: have only one credit output port, used for sending
  // credits to other tiles. Credits aren't used for local communication.
  creditsOut       = new CreditOutput[1];
  validCreditOut   = new ReadyOutput[1];
  ackCreditOut     = new ReadyInput[1];

  dataToBuffer     = new sc_buffer<DataType>[numOutputs];
  creditsToNetwork = new sc_buffer<CreditType>[numOutputs];
  ackCreditSig     = new sc_signal<ReadyType>[numOutputs];
  validDataSig     = new sc_signal<ReadyType>[numOutputs];
  validCreditSig   = new sc_signal<ReadyType>[numOutputs];

  sendData         = new sc_event[numOutputs];

  SC_METHOD(readyChanged);
  for(unsigned int i=0; i<numOutputs; i++) sensitive << bufferHasSpace[i];
  // do initialise

  // Method for each input port, forwarding data to the correct buffer when it
  // arrives. Each channel end has a single writer, so it is impossible to
  // receive multiple data for the same channel end in one cycle.
  for(PortIndex i=0; i<numInputs; i++)
    SPAWN_METHOD(validDataIn[i].pos(), InputCrossbar::newData, i, false);

  // Method for each output port, writing data into each buffer.
  for(ChannelIndex i=0; i<numOutputs; i++)
    SPAWN_METHOD(sendData[i], InputCrossbar::writeToBuffer, i, false);

  // Wire up the small networks.
  creditNet.clock(creditClock);
  creditNet.dataOut[0](creditsOut[0]);
  creditNet.validDataOut[0](validCreditOut[0]);
  creditNet.ackDataOut[0](ackCreditOut[0]);

  // Create and wire up all flow control units.
  for(unsigned int i=0; i<numOutputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_gen_unique_name("fc_in"), firstInput.getComponentID(), firstInput.addChannel(i, numOutputs));
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->dataOut(dataOut[i]);

    fc->dataIn(dataToBuffer[i]);
    fc->validDataIn(validDataSig[i]);
    fc->creditsOut(creditsToNetwork[i]);
    fc->validCreditOut(validCreditSig[i]);
    fc->ackCreditOut(ackCreditSig[i]);

    creditNet.dataIn[i](creditsToNetwork[i]);
    creditNet.validDataIn[i](validCreditSig[i]);
    creditNet.ackDataIn[i](ackCreditSig[i]);
  }
}

InputCrossbar::~InputCrossbar() {
  delete[] dataIn;            delete[] validDataIn;
  delete[] creditsOut;        delete[] validCreditOut;  delete[] ackCreditOut;
  delete[] dataOut;
  delete[] bufferHasSpace;

  delete[] dataToBuffer;      delete[] validDataSig;
  delete[] creditsToNetwork;  delete[] validCreditSig;  delete[] ackCreditSig;

  delete[] sendData;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
