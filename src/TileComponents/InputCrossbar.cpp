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
  const AddressedWord& data = dataIn[input].read();
  ChannelIndex destination = data.channelID().getChannel();

  // If we are ready for data, then this method being called means data has
  // arrived.
  if(readyOut[input].read()) {
    assert(dataIn[input].valid());

    if(destination >= numOutputs) cout << "Trying to send to " << data.channelID() << endl;
    assert(destination < numOutputs);

    // Trigger a method which will send the data.
    dataSource[destination] = input;
    sendData[destination].notify();

    readyOut[input].write(false);
    next_trigger(sc_core::SC_ZERO_TIME);  // zero time, or clock edge?
  }
  // We are not ready for data - waiting for confirmation from the buffer that
  // there is space for more data.
  else {
    if(bufferHasSpace[destination].read()) {
      readyOut[input].write(true);
      next_trigger(dataIn[input].default_event());
    }
    else
      next_trigger(bufferHasSpace[destination].posedge_event());
  }
}

void InputCrossbar::writeToBuffer(ChannelIndex output) {
  // There is data to send.
  PortIndex source = dataSource[output];
  dataToBuffer[output].write(dataIn[source].read());
  dataIn[source].ack();
}

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    firstInput(ChannelID(id,0)),
    creditNet("credit", ID, numOutputs, 1, 1, Network::NONE, Dimension(1.0/CORES_PER_TILE, 0.05)),
    dataSource(numOutputs) {

  creditNet.initialise();

  dataIn           = new loki_in<DataType>[numInputs];
  readyOut         = new ReadyOutput[numInputs];

  dataOut          = new sc_out<Word>[numOutputs];
  bufferHasSpace   = new sc_in<bool>[numOutputs];

  // Possibly temporary: have only one credit output port, used for sending
  // credits to other tiles. Credits aren't used for local communication.
  creditsOut       = new loki_out<CreditType>[1];

  dataToBuffer     = new loki_signal<DataType>[numOutputs];
  creditsToNetwork = new loki_signal<CreditType>[numOutputs];

  sendData         = new sc_event[numOutputs];

  // Method for each input port, forwarding data to the correct buffer when it
  // arrives. Each channel end has a single writer, so it is impossible to
  // receive multiple data for the same channel end in one cycle.
  for(PortIndex i=0; i<numInputs; i++)
    SPAWN_METHOD(dataIn[i], InputCrossbar::newData, i, false);

  // Method for each output port, writing data into each buffer.
  for(ChannelIndex i=0; i<numOutputs; i++)
    SPAWN_METHOD(sendData[i], InputCrossbar::writeToBuffer, i, false);

  for(unsigned int i=0; i<numInputs; i++)
    readyOut[i].initialize(true);

  // Wire up the small networks.
  creditNet.clock(creditClock);
  creditNet.dataOut[0](creditsOut[0]);

  // Create and wire up all flow control units.
  for(unsigned int i=0; i<numOutputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_gen_unique_name("fc_in"), firstInput.getComponentID(), firstInput.addChannel(i, numOutputs));
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->dataOut(dataOut[i]);
    fc->dataIn(dataToBuffer[i]);
    fc->creditsOut(creditsToNetwork[i]);

    creditNet.dataIn[i](creditsToNetwork[i]);
  }
}

InputCrossbar::~InputCrossbar() {
  delete[] dataIn;            delete[] readyOut;
  delete[] creditsOut;
  delete[] dataOut;
  delete[] bufferHasSpace;

  delete[] dataToBuffer;
  delete[] creditsToNetwork;

  delete[] sendData;

  for(unsigned int i=0; i<flowControl.size(); i++) delete flowControl[i];
}
