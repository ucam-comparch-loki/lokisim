/*
 * InputCrossbar.cpp
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "InputCrossbar.h"
#include "../../Utility/Assert.h"

using sc_core::sc_module_name;

const PortIndex INACTIVE = -1;

void InputCrossbar::newData(PortIndex input) {
  loki_assert(iData[input].valid());

  const NetworkData& data = iData[input].read();

  ChannelIndex destination = data.channelID().channel;
  if (destination >= oData.size())
    LOKI_WARN << this->name() << " trying to receive data for " << data.channelID() << std::endl;
  loki_assert(destination < oData.size());

  if (dataSource[destination] != INACTIVE)
    LOKI_WARN << this->name() << " has multiple sources sending simultaneously to channel " << destination << "; packet dropped." << std::endl;

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
  dataSource[output] = INACTIVE;
}

void InputCrossbar::updateFlowControl(ChannelIndex input) {
  // I would prefer to connect the bufferHasSpace inputs directly to the
  // readyOut outputs, but SystemC does not allow this.
  oReady[input].write(iFlowControl[input].read());
}

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID,
                             size_t numInputs, size_t numOutputs) :
    LokiComponent(name),
    clock("clock"),
    iData("iData", numInputs),
    oReady("oReady", numOutputs),
    oData("oData", numOutputs),
    iFlowControl("iFlowControl", numOutputs),
    iDataConsumed("iDataConsumed", numOutputs),
    oCredit("oCredit", 1),
    creditNet("credit", numOutputs),
    constantLow("constantLow"),
    dataToBuffer("dataToBuffer", numOutputs),
    creditsToNetwork("creditsToNetwork", numOutputs),
    sendData("sendDataEvent", numOutputs),
    dataSource(numOutputs, INACTIVE) {

  // Method for each input port, forwarding data to the correct buffer when it
  // arrives. Each channel end has a single writer, so it is impossible to
  // receive multiple data for the same channel end in one cycle.
  for (PortIndex i=0; i<iData.size(); i++)
    SPAWN_METHOD(iData[i], InputCrossbar::newData, i, false);

  // Method for each output port, writing data into each buffer.
  for (ChannelIndex i=0; i<sendData.size(); i++)
    SPAWN_METHOD(sendData[i], InputCrossbar::writeToBuffer, i, false);

  for (ChannelIndex i=0; i<iFlowControl.size(); i++)
    SPAWN_METHOD(iFlowControl[i], InputCrossbar::updateFlowControl, i, true);

  // Wire up the small networks.
  creditNet.iData(creditsToNetwork);
  creditNet.oData(oCredit[0]);
  creditNet.iHold(constantLow); // Wormhole routing not needed within the core..
  constantLow.write(false);

  // Create and wire up all flow control units.
  for (unsigned int i=0; i<oData.size(); i++) {
    FlowControlIn* fc = new FlowControlIn(sc_core::sc_gen_unique_name("fc_in"), ChannelID(ID, i));
    flowControl.push_back(fc);

    fc->clock(clock);

    fc->oData(oData[i]);
    fc->iData(dataToBuffer[i]);
    fc->oCredit(creditsToNetwork[i]);
    fc->iDataConsumed(iDataConsumed[i]);
  }
}
