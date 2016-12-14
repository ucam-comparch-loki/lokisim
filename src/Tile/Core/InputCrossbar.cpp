/*
 * InputCrossbar.cpp
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "InputCrossbar.h"
#include "../../Network/FlowControl/FlowControlIn.h"
#include "../../Utility/Assert.h"

// One multicast input from each core, instructions, data, and router.
const unsigned int InputCrossbar::numInputs = 3 + CORES_PER_TILE;
const unsigned int InputCrossbar::numOutputs = CORE_INPUT_CHANNELS;

void InputCrossbar::newData(PortIndex input) {
  loki_assert(iData[input].valid());

  const NetworkData& data = iData[input].read();

  ChannelIndex destination = data.channelID().channel;
  if (destination >= numOutputs)
    LOKI_WARN << this->name() << " trying to receive data for " << data.channelID() << endl;
  loki_assert(destination < numOutputs);

  // Trigger a method which will write the data to the appropriate output.
  dataSource[destination] = input;
  sendData[destination].notify();
}

// TODO: create a newCredit method to replace the Crossbar? Or a SharedBus
// which could also be used to get credits from cores to the router.

void InputCrossbar::writeToBuffer(ChannelIndex output) {
  if (!iFlowControl[output].read()) {
    next_trigger(iFlowControl[output].default_event());
    return;
  }
  if (dataToBuffer[output].valid()) {
    // TODO: is it safe to stall here, or could it end up blocking other inputs?
    next_trigger(dataToBuffer[output].ack_event());
    return;
  }

  loki_assert(!dataToBuffer[output].valid());

  // There is data to send.
  PortIndex source = dataSource[output];
  dataToBuffer[output].write(iData[source].read());
  iData[source].ack();
}

void InputCrossbar::updateFlowControl(ChannelIndex input) {
  // I would prefer to connect the bufferHasSpace inputs directly to the
  // readyOut outputs, but SystemC does not allow this.
  oReady[input].write(iFlowControl[input].read());
}

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID) :
    LokiComponent(name, ID),
    creditNet("credit", ID, numOutputs, 1, 1, Network::NONE, 1),
    dataSource(numOutputs) {

  //creditNet.initialise();

  iData.init(numInputs);
  oReady.init(CORE_INPUT_CHANNELS);
  oData.init(numOutputs);
  iFlowControl.init(numOutputs);
  iDataConsumed.init(numOutputs);

  // Possibly temporary: have only one credit output port, used for sending
  // credits to other tiles. Credits aren't used for local communication.
  oCredit.init(1);

  dataToBuffer.init(numOutputs);
  creditsToNetwork.init(numOutputs);

  sendData.init(numOutputs);

  // Method for each input port, forwarding data to the correct buffer when it
  // arrives. Each channel end has a single writer, so it is impossible to
  // receive multiple data for the same channel end in one cycle.
  for (PortIndex i=0; i<iData.length(); i++)
    SPAWN_METHOD(iData[i], InputCrossbar::newData, i, false);

  // Method for each output port, writing data into each buffer.
  for (ChannelIndex i=0; i<sendData.length(); i++)
    SPAWN_METHOD(sendData[i], InputCrossbar::writeToBuffer, i, false);

  for (ChannelIndex i=0; i<iFlowControl.length(); i++)
    SPAWN_METHOD(iFlowControl[i], InputCrossbar::updateFlowControl, i, true);

  // Wire up the small networks.
  creditNet.clock(creditClock);
  creditNet.oData[0](oCredit[0]);
  creditNet.iReady[0][0](constantHigh); // Can always send credits.
  constantHigh.write(true);

  // Create and wire up all flow control units.
  for (unsigned int i=0; i<numOutputs; i++) {
    FlowControlIn* fc = new FlowControlIn(sc_gen_unique_name("fc_in"), id, ChannelID(id, i));
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->oData(oData[i]);
    fc->iData(dataToBuffer[i]);
    fc->oCredit(creditsToNetwork[i]);
    fc->iDataConsumed(iDataConsumed[i]);

    creditNet.iData[i](creditsToNetwork[i]);
  }
}

InputCrossbar::~InputCrossbar() {
  for (unsigned int i=0; i<flowControl.size(); i++)
    delete flowControl[i];
}
