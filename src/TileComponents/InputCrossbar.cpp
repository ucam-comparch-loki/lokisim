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
#include "../TileComponents/TileComponent.h"

const unsigned int InputCrossbar::numInputs = CORE_INPUT_PORTS;
const unsigned int InputCrossbar::numOutputs = CORE_INPUT_CHANNELS;

void InputCrossbar::newData(PortIndex input) {
  assert(dataIn[input].valid());

  const AddressedWord& data = dataIn[input].read();

  ChannelIndex destination = data.channelID().getChannel();
  if (destination >= numOutputs)
    cerr << this->name() << " trying to send to " << data.channelID() << endl;
  assert(destination < numOutputs);

  // Trigger a method which will write the data to the appropriate output.
  dataSource[destination] = input;
  sendData[destination].notify();
}

// TODO: create a newCredit method to replace the Crossbar? Or a SharedBus
// which could also be used to get credits from cores to the router.

void InputCrossbar::writeToBuffer(ChannelIndex output) {
  if (!bufferHasSpace[output].read()) cout << this->name() << " " << (int)output << endl;
  assert(bufferHasSpace[output].read());

  // There is data to send.
  PortIndex source = dataSource[output];
  dataToBuffer[output].write(dataIn[source].read());
  dataIn[source].ack();
}

void InputCrossbar::updateFlowControl(ChannelIndex input) {
  // I would prefer to connect the bufferHasSpace inputs directly to the
  // readyOut outputs, but SystemC does not allow this.
  readyOut[input].write(bufferHasSpace[input].read());
}

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID) :
    Component(name, ID),
    firstInput(ChannelID(id,0)),
    creditNet("credit", ID, numOutputs, 1, 1, Network::NONE, 1),
    dataSource(numOutputs) {

  //creditNet.initialise();

  dataIn.init(numInputs);
  readyOut.init(numOutputs);
  dataOut.init(numOutputs);
  bufferHasSpace.init(numOutputs);
  dataConsumed.init(numOutputs);

  // Possibly temporary: have only one credit output port, used for sending
  // credits to other tiles. Credits aren't used for local communication.
  creditsOut.init(1);

  dataToBuffer.init(numOutputs);
  creditsToNetwork.init(numOutputs);

  sendData.init(numOutputs);

  // Method for each input port, forwarding data to the correct buffer when it
  // arrives. Each channel end has a single writer, so it is impossible to
  // receive multiple data for the same channel end in one cycle.
  for (PortIndex i=0; i<numInputs; i++)
    SPAWN_METHOD(dataIn[i], InputCrossbar::newData, i, false);

  // Method for each output port, writing data into each buffer.
  for (ChannelIndex i=0; i<numOutputs; i++)
    SPAWN_METHOD(sendData[i], InputCrossbar::writeToBuffer, i, false);

  for (ChannelIndex i=0; i<numOutputs; i++)
    SPAWN_METHOD(bufferHasSpace[i], InputCrossbar::updateFlowControl, i, true);

  // Wire up the small networks.
  creditNet.clock(creditClock);
  creditNet.dataOut[0](creditsOut[0]);
  creditNet.readyIn[0][0](constantHigh); // Can always send credits.
  constantHigh.write(true);

  // Create and wire up all flow control units.
  for (unsigned int i=0; i<numOutputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_gen_unique_name("fc_in"), firstInput.getComponentID(), firstInput.addChannel(i, numOutputs));
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->dataOut(dataOut[i]);
    fc->dataIn(dataToBuffer[i]);
    fc->creditsOut(creditsToNetwork[i]);
    fc->dataConsumed(dataConsumed[i]);

    creditNet.dataIn[i](creditsToNetwork[i]);
  }
}

InputCrossbar::~InputCrossbar() {
  for (unsigned int i=0; i<flowControl.size(); i++)
    delete flowControl[i];
}
