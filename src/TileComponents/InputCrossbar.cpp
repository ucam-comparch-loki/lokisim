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
  assert(dataIn[input].valid());

  const AddressedWord& data = dataIn[input].read();
  ChannelIndex destination = data.channelID().getChannel();

  if(destination >= numOutputs) cout << "Trying to send to " << data.channelID() << endl;
  assert(destination < numOutputs);

  // Trigger a method which will write the data to the appropriate output.
  dataSource[destination] = input;
  sendData[destination].notify();
}

void InputCrossbar::writeToBuffer(ChannelIndex output) {
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
    creditNet("credit", ID, numOutputs, 1, 1, Network::NONE, Dimension(1.0/CORES_PER_TILE, 0.05)),
    dataSource(numOutputs) {

  creditNet.initialise();

  dataIn           = new DataInput[numInputs];
  readyOut         = new ReadyOutput[numOutputs];

  dataOut          = new sc_out<Word>[numOutputs];
  bufferHasSpace   = new ReadyInput[numOutputs];

  // Possibly temporary: have only one credit output port, used for sending
  // credits to other tiles. Credits aren't used for local communication.
  creditsOut       = new CreditOutput[1];

  dataToBuffer     = new DataSignal[numOutputs];
  creditsToNetwork = new CreditSignal[numOutputs];

  sendData         = new sc_event[numOutputs];

  // Method for each input port, forwarding data to the correct buffer when it
  // arrives. Each channel end has a single writer, so it is impossible to
  // receive multiple data for the same channel end in one cycle.
  for(PortIndex i=0; i<numInputs; i++)
    SPAWN_METHOD(dataIn[i], InputCrossbar::newData, i, false);

  // Method for each output port, writing data into each buffer.
  for(ChannelIndex i=0; i<numOutputs; i++)
    SPAWN_METHOD(sendData[i], InputCrossbar::writeToBuffer, i, false);

  for(ChannelIndex i=0; i<numOutputs; i++)
    SPAWN_METHOD(bufferHasSpace[i], InputCrossbar::updateFlowControl, i, true);

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
