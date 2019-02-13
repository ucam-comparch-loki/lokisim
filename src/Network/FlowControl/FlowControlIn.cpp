/*
 * FlowControlIn.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Utility/Assert.h"
#include "../../Utility/Instrumentation/Latency.h"

void FlowControlIn::dataLoop() {
  // Don't accept any new data if we are waiting to reject a claim for this
  // port. We can't handle simultaneous rejections.
  if (iData.valid() && nackChannel.isNullMapping() && !disconnectPending) {

    NetworkData data = iData.read();

    if (!data.channelID().multicast && (data.channelID() != sinkChannel)) {
      LOKI_ERROR << data << " arrived at channel " << sinkChannel << endl;
      loki_assert(false);
    }

    if (data.getCoreMetadata().allocate)
      handlePortClaim(data);
    else {
      oData.write(data.payload());
      Instrumentation::Latency::coreReceivedResult(sinkChannel.component, data);
    }

    iData.ack();
  }
}

void FlowControlIn::handlePortClaim(NetworkData request) {
  loki_assert(iData.valid());
  loki_assert(!disconnectPending);

  loki_assert(request.getCoreMetadata().allocate);


  // If the connection is already set up, this is a disconnect request.
  if (request.getCoreMetadata().acquired) {
    // State needs to be preserved until the final credit has been sent, so
    // just set a flag for now.
    disconnectPending = true;
    addCredit();
  }
  // If the connection isn't already set up, try to set it up.
  else {
    int payload = request.payload().toInt();
    ComponentID component(payload & 0xFFFF, Encoding::softwareComponentID);
    ChannelIndex channel = (payload >> 16) & 0xFFFF;
    ChannelID source = ChannelID(component, channel);

    // Only accept if there is not already a connection to this channel.
    if (sourceChannel.isNullMapping()) {
      assert(numCredits == 0);
      acceptPortClaim(source);
    }
    else {
      rejectPortClaim(source);
    }

  }

}

void FlowControlIn::acceptPortClaim(ChannelID source) {
  useCredits = true;
  sourceChannel = source;

  addCredit();

  LOKI_LOG << this->name() << " claimed by " << sourceChannel << endl;
}

void FlowControlIn::rejectPortClaim(ChannelID source) {
  // Can't have multiple pending nacks.
  loki_assert(nackChannel.isNullMapping());
  nackChannel = source;

  assert(nackChannel != sourceChannel);

  newCredit.notify();

  LOKI_LOG << this->name() << " rejected claim from " << nackChannel << endl;
}

void FlowControlIn::addCredit() {
  if (useCredits) {
    numCredits++;
    newCredit.notify();
  }
}

void FlowControlIn::creditLoop() {
  switch (creditState) {
    case IDLE : {
      // Top priority: send nack for connection setup requests.
      // These can't be queued up.
      if (!nackChannel.isNullMapping()) {
        prepareNack();
        next_trigger(clock.posedge_event());
        creditState = CREDIT_SEND;
      }
      // Send credits if there are any.
      else if (numCredits > 0) {
        prepareCredit();
        // Information can only be sent onto the network at a positive clock edge.
        next_trigger(clock.posedge_event());
        creditState = CREDIT_SEND;
      }
      // Wait until something happens.
      else {
        next_trigger(newCredit);
      }
      
      break;
    }

    case CREDIT_SEND : {
      sendCredit();

      // Wait for the credit to be acknowledged.
      next_trigger(oCredit.ack_event());
      creditState = ACKNOWLEDGE;

      break;
    }

    case ACKNOWLEDGE : {
      next_trigger(sc_core::SC_ZERO_TIME);
      creditState = IDLE;

      break;
    }
  } // end switch
}

void FlowControlIn::prepareCredit() {
  loki_assert(useCredits);
  assert(!sourceChannel.isNullMapping());
  assert(toSend.channelID().isNullMapping());
  assert(numCredits > 0);

  toSend = NetworkCredit(Word(numCredits), sourceChannel, true);

  numCredits = 0;

  LOKI_LOG << this->name() << " sending credit to " << sourceChannel << " (id:" << toSend.messageID() << ")" << endl;

  // Tear down the connection, if necessary.
  if (disconnectPending) {
    LOKI_LOG << this->name() << " ending connection with " << sourceChannel << endl;

    sourceChannel = ChannelID();
    useCredits = false;
    disconnectPending = false;

    unblockInput.notify();
  }
}

void FlowControlIn::prepareNack() {
  assert(!nackChannel.isNullMapping());
  assert(toSend.channelID().isNullMapping());

  toSend = NetworkCredit(Word(0), nackChannel, true);

  LOKI_LOG << this->name() << " sending nack to " << nackChannel << " (id:" << toSend.messageID() << ")" << endl;

  nackChannel = ChannelID();
  unblockInput.notify();
}

void FlowControlIn::sendCredit() {
  assert(!toSend.channelID().isNullMapping());
  oCredit.write(toSend);
  toSend = NetworkCredit();
}

FlowControlIn::FlowControlIn(sc_module_name name, const ChannelID& channelManaged) :
    LokiComponent(name),
    clock("clock"),
    iData("iData"),
    oData("oData"),
    oCredit("oCredit"),
    iDataConsumed("iDataConsumed"),
    sinkChannel(channelManaged) {

  sourceChannel = ChannelID();
  nackChannel = ChannelID();
  useCredits = false;
  numCredits = 0;

  disconnectPending = false;

  creditState = IDLE;

  SC_METHOD(dataLoop);
  sensitive << iData << unblockInput;
  dont_initialize();

  SC_METHOD(creditLoop);

  SC_METHOD(addCredit);
  sensitive << iDataConsumed;
  dont_initialize();

}
