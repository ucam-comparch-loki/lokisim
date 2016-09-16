/*
 * FlowControlIn.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Utility/Assert.h"

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
      handlePortClaim();
    else
      oData.write(data.payload());

    iData.ack();

  }
}

void FlowControlIn::handlePortClaim() {
  loki_assert(iData.valid());
  loki_assert(!disconnectPending);

  NetworkData data = iData.read();
  loki_assert(data.getCoreMetadata().allocate);

  // If the connection is already set up, this is a disconnect request.
  if (data.getCoreMetadata().acquired) {
    // State needs to be preserved until the final credit has been sent, so
    // just set a flag for now.
    disconnectPending = true;
    addCredit();
  }
  // If the connection isn't already set up, try to set it up.
  else {
    int payload = data.payload().toInt();
    ComponentID component(payload & 0xFFFF);
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
        next_trigger(clock.posedge_event());
        creditState = NACK_SEND;
      }
      // Send credits if there are any.
      else if (numCredits > 0) {
        loki_assert(useCredits);

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
      loki_assert(numCredits > 0);
      loki_assert(useCredits);

      // Only send the credit if there is a valid address to send to.
      if (!sourceChannel.isNullMapping()) {
        sendCredit();

        // Wait for the credit to be acknowledged.
        next_trigger(oCredit.ack_event());
        creditState = ACKNOWLEDGE;
      }
      else {
        numCredits = 0;
        next_trigger(newCredit);
        creditState = IDLE;
      }

      break;
    }

    case NACK_SEND : {
      sendNack();

      // Wait for the nack to be acknowledged.
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

void FlowControlIn::sendCredit() {
  assert(!sourceChannel.isNullMapping());

  NetworkCredit aw(Word(numCredits), sourceChannel);
  oCredit.write(aw);

  numCredits = 0;

  LOKI_LOG << this->name() << " sent credit to " << sourceChannel << " (id:" << aw.messageID() << ")" << endl;

  // Tear down the connection, if necessary.
  if (disconnectPending) {
    LOKI_LOG << this->name() << " ending connection with " << sourceChannel << endl;

    sourceChannel = ChannelID();
    useCredits = false;
    disconnectPending = false;

    unblockInput.notify();
  }
}

void FlowControlIn::sendNack() {
  assert(!nackChannel.isNullMapping());

  NetworkCredit aw(Word(0), nackChannel);
  oCredit.write(aw);

  LOKI_LOG << this->name() << " sent nack to " << nackChannel << " (id:" << aw.messageID() << ")" << endl;

  nackChannel = ChannelID();
  unblockInput.notify();
}

FlowControlIn::FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
    Component(name, ID),
    sinkChannel(channelManaged) {

  sourceChannel = ChannelID();
  nackChannel = ChannelID();
  useCredits = true;
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
