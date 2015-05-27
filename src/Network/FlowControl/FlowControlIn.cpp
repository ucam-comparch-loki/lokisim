/*
 * FlowControlIn.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../../Datatype/MemoryRequest.h"

void FlowControlIn::dataLoop() {
  NetworkData data = iData.read();

  if (!data.channelID().multicast && (data.channelID() != channel)) {
    cerr << "Error: " << data << " arrived at channel " << channel << endl;
    assert(false);
  }

  if (data.getCoreMetadata().allocate)
    handlePortClaim();
  else
    oData.write(data.payload());

  iData.ack();
}

void FlowControlIn::handlePortClaim() {
  assert(iData.valid());

  NetworkData data = iData.read();
  assert(data.getCoreMetadata().allocate);

  // TODO: only accept the port claim when we have no credits left to send.

  // Set the return address so we can send flow control.
  returnAddress = data.payload().toInt();

  // Only use credits if the sender is on a different tile.
  useCredits = id.tile != returnAddress.component.tile;

  addCredit();

  if (DEBUG)
    cout << this->name() << " claimed by " << returnAddress << " [flow control " << (useCredits ? "enabled" : "disabled") << "]" << endl;

  // If this is a port claim from a memory, to a core's data input, this
  // message doubles as a synchronisation message to show that all memories are
  // now set up. We want to forward it to the buffer when possible.
  if (!useCredits &&
      (returnAddress.component.position >= CORES_PER_TILE) &&
      (data.channelID().channel >= 2)) {

    oData.write(data.payload());
  }

}

void FlowControlIn::addCredit() {
  if (useCredits) {
    numCredits++;
    newCredit.notify();
    assert(numCredits <= IN_CHANNEL_BUFFER_SIZE);
  }
}

void FlowControlIn::creditLoop() {
  switch (creditState) {
    case NO_CREDITS : {
      if (numCredits == 0) {
        next_trigger(newCredit);
        return;
      }
      
      assert(useCredits);

      // Information can only be sent onto the network at a positive clock edge.
      next_trigger(clock.posedge_event());
      creditState = WAITING_TO_SEND;
      break;
    }

    case WAITING_TO_SEND : {
      assert(numCredits > 0);
      assert(useCredits);

      // Only send the credit if there is a valid address to send to.
      if (!returnAddress.isNullMapping()) {
        sendCredit();

        // Wait for the credit to be acknowledged.
        next_trigger(oCredit.ack_event());
        creditState = WAITING_FOR_ACK;
      }
      else {
//        cerr << "Warning: trying to send credit from " << channel.getString()
//             << " when there is no connection" << endl;
        numCredits--;
        next_trigger(newCredit);
        creditState = NO_CREDITS;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      if (numCredits > 0) {
        next_trigger(clock.posedge_event());
        creditState = WAITING_TO_SEND;
      }
      else {
        next_trigger(newCredit);
        creditState = NO_CREDITS;
      }

      break;
    }
  } // end switch
}

void FlowControlIn::sendCredit() {
  NetworkCredit aw(Word(1), returnAddress);
  oCredit.write(aw);

  numCredits--;

  if (DEBUG)
    cout << this->name() << " sent credit to " << returnAddress << " (id:" << aw.messageID() << ")" << endl;
}

FlowControlIn::FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
    Component(name, ID),
    channel(channelManaged) {

  returnAddress = ChannelID();
  useCredits = true;
  numCredits = 0;

  creditState = NO_CREDITS;

  SC_METHOD(dataLoop);
  sensitive << iData;
  dont_initialize();

  SC_METHOD(creditLoop);

  SC_METHOD(addCredit);
  sensitive << iDataConsumed;
  dont_initialize();

}
