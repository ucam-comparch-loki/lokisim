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

const PortIndex INACTIVE = -1;

void InputCrossbar::newData(PortIndex input) {
  loki_assert(iData[input].valid());

  const NetworkData& data = iData[input].read();

  ChannelIndex destination = data.channelID().channel;
  if (destination >= oData.size())
    LOKI_WARN << this->name() << " trying to receive data for " << data.channelID() << endl;
  loki_assert(destination < oData.size());

  if (dataSource[destination] != INACTIVE)
    LOKI_WARN << "multiple sources sending simultaneously to " << ChannelID(id, destination) << "; packet dropped." << endl;

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

InputCrossbar::InputCrossbar(sc_module_name name, const ComponentID& ID) :
    LokiComponent(name, ID),
    clock("clock"),
    creditClock("creditClock"),
    iData("iData", CORES_PER_TILE + 3), // All cores + insts + data + router
    oReady("oReady", CORE_INPUT_CHANNELS),
    oData("oData", CORE_INPUT_CHANNELS),
    iFlowControl("iFlowControl", CORE_INPUT_CHANNELS),
    iDataConsumed("iDataConsumed", CORE_INPUT_CHANNELS),
    oCredit("oCredit", 1),
    creditNet("credit", ID, CORE_INPUT_CHANNELS, 1, 1, Network::NONE, 1),
    constantHigh("constantHigh"),
    dataToBuffer("dataToBuffer", CORE_INPUT_CHANNELS),
    creditsToNetwork("creditsToNetwork", CORE_INPUT_CHANNELS),
    sendData("sendDataEvent", CORE_INPUT_CHANNELS),
    dataSource(CORE_INPUT_CHANNELS, INACTIVE) {

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
  creditNet.clock(creditClock);
  creditNet.oData[0](oCredit[0]);
  creditNet.iReady[0][0](constantHigh); // Can always send credits.
  constantHigh.write(true);

  // Create and wire up all flow control units.
  for (unsigned int i=0; i<oData.size(); i++) {
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
