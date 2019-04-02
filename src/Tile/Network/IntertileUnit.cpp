/*
 * IntertileUnit.cpp
 *
 *  Created on: 29 Mar 2019
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "IntertileUnit.h"
#include "../../Utility/Assert.h"
#include "../Tile.h"

IntertileUnit::IntertileUnit(sc_module_name name, const tile_parameters_t& params) :
    LokiComponent(name),
    iData("iData"),
    oData("oData"),
    oCredit("oCredit"),
    iFlowControl("iFlowControl", params.numCores, params.core.numInputChannels),
    inBuffer("inBuffer", 4),  // To match Verilog
    outBuffer("outBuffer", 1) {

  nackChannel = ChannelID();

  for (uint core = 0; core < params.numCores; core++) {
    creditState.push_back(vector<credit_state_t>());

    for (uint buffer = 0; buffer < params.core.numInputChannels; buffer++) {
      credit_state_t state(ChannelID(tile().id, core, buffer), *this);
      creditState[core].push_back(state);
    }
  }

  iData(inBuffer);
  oData(outBuffer);

  SC_METHOD(dataArrived);
  sensitive << inBuffer.canReadEvent();
  dont_initialize();

  SC_METHOD(sendCredits);
  sensitive << newCreditEvent;
  dont_initialize();

}

void IntertileUnit::end_of_elaboration() {
  for (uint core = 0; core < iFlowControl.size(); core++) {
    for (uint buffer = 0; buffer < iFlowControl[core].size(); buffer++) {
      // Can't provide 2D position, so flatten.
      PortIndex port = addressToIndex(ChannelID(tile().id, core, buffer));
      SPAWN_METHOD(iFlowControl[core][buffer]->dataConsumedEvent(), IntertileUnit::dataConsumed1D, port, false);
    }
  }
}

void IntertileUnit::dataArrived() {
  loki_assert(inBuffer.canRead());

  // Can't accept more data if there is an outstanding nack, because we only
  // support one nack at a time.
  if (!nackChannel.isNullMapping())
    next_trigger(oCredit->canWriteEvent());
  // Wait until output buffer/register is available.
  else if (!outBuffer.canWrite())
    next_trigger(outBuffer.canWriteEvent());
  else {
    Flit<Word> data = inBuffer.read();
    loki_assert(data.channelID().component.tile == tile().id);

    if (data.getCoreMetadata().allocate)
      handlePortClaim(data);
    else
      outBuffer.write(data);
  }
}

void IntertileUnit::handlePortClaim(const Flit<Word> request) {
  loki_assert(request.getCoreMetadata().allocate);

  ChannelID destination = request.channelID();
  credit_state_t& state = creditState[destination.component.position][destination.channel];

  // If the connection is already set up, this is a disconnect request.
  if (request.getCoreMetadata().acquired) {
    // State needs to be preserved until the final credit has been sent, so
    // just set a flag for now.
    state.disconnectPending = true;
    state.addCredit();
  }
  // If the connection isn't already set up, try to set it up.
  else {
    int payload = request.payload().toInt();
    ComponentID component(payload & 0xFFFF, Encoding::softwareComponentID);
    ChannelIndex channel = (payload >> 16) & 0xFFFF;
    ChannelID source = ChannelID(component, channel);

    // Only accept if there is not already a connection to this channel.
    if (state.sourceAddress.isNullMapping())
      acceptPortClaim(state, source);
    else
      rejectPortClaim(state, source);

  }
}

void IntertileUnit::acceptPortClaim(credit_state_t& state, const ChannelID& source) {
  loki_assert(state.creditsPending == 0);
  state.useCredits = true;
  state.sourceAddress = source;
  state.addCredit();

  LOKI_LOG << this->name() << ": " << state.sinkAddress << " claimed by "
      << source << ", with" << (state.useCredits ? "" : "out") << " credits" << endl;
}

void IntertileUnit::rejectPortClaim(credit_state_t& state, const ChannelID& source) {
  // Can't have multiple pending nacks.
  loki_assert(nackChannel.isNullMapping());

  // Can't have a claim from the source we're already connected to.
  loki_assert(state.sourceAddress != source);

  nackChannel = source;
  newCreditEvent.notify(sc_core::SC_ZERO_TIME);

  LOKI_LOG << this->name() << ": " << state.sinkAddress << " rejected claim from " << source << endl;
}

void IntertileUnit::sendCredits() {
  // Wait until there is space to write a result.
  if (!oCredit->canWrite())
    next_trigger(oCredit->canWriteEvent());
  else if (creditsOutstanding.empty())
    next_trigger(creditsOutstanding.newRequestEvent());
  else {
    // Priority: respond to failed connection attempts. We can't have more
    // than one of these in progress simultaneously.
    if (!nackChannel.isNullMapping()) {
      sendNackFlit(nackChannel);
      nackChannel = ChannelID();
    }

    // Otherwise, select a buffer to send credits for. Not obvious what the
    // best way of selecting is. Using round robin for simplicity.
    //  * Prioritise channels with more outstanding credits?
    //  * Prioritise channels waiting to be released?
    //  * Round robin could iterate over all buffers of one core before
    //    considering the next core, which might not be fair.
    else {
      PortIndex buffer = arbiter.selectRequester(creditsOutstanding);

      // Assuming all cores are identical.
      uint component = buffer / iFlowControl.size();
      uint channel   = buffer % iFlowControl.size();
      credit_state_t& state = creditState[component][channel];
      sendCreditFlit(state);

      creditsOutstanding.remove(buffer);
    }

    // If there are already more credits waiting, don't need to wait for a new
    // credit to be required.
    if (!creditsOutstanding.empty())
      next_trigger(sc_core::SC_ZERO_TIME);
    // Default event: newCreditEvent
  }
}

void IntertileUnit::sendNackFlit(ChannelID destination) {
  loki_assert(!destination.isNullMapping());
  loki_assert(oCredit->canWrite());

  Flit<Word> nack(Word(0), destination, true);

  LOKI_LOG << this->name() << " sending nack to " << destination << " (id:" << nack.messageID() << ")" << endl;

  oCredit->write(nack);
}

void IntertileUnit::sendCreditFlit(credit_state_t& state) {
  loki_assert(state.useCredits);
  loki_assert(!state.sourceAddress.isNullMapping());
  loki_assert(state.creditsPending > 0);
  loki_assert(oCredit->canWrite());

  Flit<Word> flit(Word(state.creditsPending), state.sourceAddress, true);

  LOKI_LOG << this->name() << " sending " << state.creditsPending
      << " credit(s) to " << state.sourceAddress << " (id:" << flit.messageID()
      << ")" << endl;

  oCredit->write(flit);
  state.creditsPending = 0;

  // Tear down the connection, if necessary.
  if (state.disconnectPending) {
    LOKI_LOG << this->name() << " ending connection with " << state.sourceAddress << endl;

    state.sourceAddress = ChannelID();
    state.useCredits = false;
    state.disconnectPending = false;
  }
}

ChannelID IntertileUnit::indexToAddress(PortIndex index) const {
  // Assuming all connected components are identical.
  uint component = index / iFlowControl.size();
  uint channel   = index % iFlowControl.size();
  return ChannelID(tile().id, component, channel);
}

PortIndex IntertileUnit::addressToIndex(ChannelID address) const {
  // Assuming all connected components are identical.
  uint component = address.component.position;
  uint channel   = address.channel;
  return (component * iFlowControl.size()) + channel;
}

void IntertileUnit::dataConsumed(uint component, uint channel) {
  credit_state_t& state = creditState[component][channel];
  state.addCredit();
}

void IntertileUnit::dataConsumed1D(PortIndex input) {
  ChannelID address = indexToAddress(input);
  dataConsumed(address.component.position, address.channel);
}

Tile& IntertileUnit::tile() const {
  return static_cast<Tile&>(*(this->get_parent_object()));
}
