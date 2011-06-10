/*
 * FlowControlIn.cpp
 *
 *  Created on: 8 Nov 2010
 *      Author: db434
 */

#include "FlowControlIn.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../TileComponents/TileComponent.h"

void FlowControlIn::dataLoop() {
  switch(dataState) {
    case WAITING_FOR_DATA: {
      if(clock.posedge()) {
        // Wait for a delta cycle, because the valid signal is deasserted on
        // the clock edge.
        next_trigger(tinyWait.default_event());
        tinyWait.write(!tinyWait.read());
      }
      else if(!validDataIn.read())
        next_trigger(validDataIn.posedge_event());
      else
        handleNewData();

      break;
    }

    case WAITING_FOR_SPACE: {
      assert(bufferHasSpace.read());

      dataOut.write(dataIn.read().payload());
      sendAck();

      break;
    }

    case SENT_ACK: {
      assert(ackDataIn.read());

      ackDataIn.write(false);

      if(!validDataIn.read()) {
        next_trigger(validDataIn.posedge_event());
        dataState = WAITING_FOR_DATA;
      }
      else {
        // The valid signal is still high at the start of the next clock cycle.
        // This means there may be more data to consume.
        next_trigger(tinyWait.default_event());
        tinyWait.write(!tinyWait.read());
        dataState = WAITING_FOR_DATA;
      }

      break;
    }
  } // end switch
}

void FlowControlIn::handleNewData() {
  if(dataIn.read().portClaim()) {
    handlePortClaim();
  }
  else if (!bufferHasSpace.read()) {
    next_trigger(bufferHasSpace.posedge_event());
    dataState = WAITING_FOR_SPACE;
  }
  else {
    dataOut.write(dataIn.read().payload());
    sendAck();
  }
}

void FlowControlIn::handlePortClaim() {
  assert(validDataIn.read());
  assert(dataIn.read().portClaim());

  // TODO: only accept the port claim when we have no credits left to send.

  // Set the return address so we can send flow control.
  returnAddress = dataIn.read().payload().toInt();
  useCredits = dataIn.read().useCredits();

  if(DEBUG)
    cout << "Channel " << channel << " was claimed by " << returnAddress << " [flow control " << (useCredits ? "enabled" : "disabled") << "]" << endl;

  // If this is a data input to a core, this message doubles as a
  // synchronisation message to show that all memories are now set up. We want
  // to forward it to the buffer when possible.
  if (!useCredits && dataIn.read().channelID().getChannel() >= 2) {
    // Wait until there is space in the buffer, if necessary
    if (!bufferHasSpace.read()) {
      next_trigger(bufferHasSpace.posedge_event());
      dataState = WAITING_FOR_SPACE;
      return;
    }
    else {
      dataOut.write(dataIn.read().payload());
      sendAck();
    }
  }
  else {
    // Acknowledge the port claim message.
    sendAck();
  }

}

void FlowControlIn::addCredit() {
  if (useCredits) {
    numCredits++;
    newCredit.notify();
    assert(numCredits <= CHANNEL_END_BUFFER_SIZE);
  }
}

void FlowControlIn::sendAck() {
  ackDataIn.write(true);
  addCredit();

  next_trigger(clock.posedge_event());
  dataState = SENT_ACK;
}

void FlowControlIn::creditLoop() {
  switch(creditState) {
    case NO_CREDITS : {
      // Should have just received a credit.
      assert(numCredits > 0);
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
      if(!returnAddress.isNullMapping()) {
        sendCredit();

        // Wait for the credit to be acknowledged.
        next_trigger(ackCreditOut.posedge_event());
        creditState = WAITING_FOR_ACK;
      }
      else {
        cerr << "Warning: trying to send credit from " << channel.getString()
             << " when there is no connection" << endl;
        numCredits--;
        next_trigger(newCredit);
        creditState = NO_CREDITS;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      assert(ackCreditOut.read());

      // Deassert the valid signal when the acknowledgement arrives.
      validCreditOut.write(false);

      if(numCredits > 0) {
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
  AddressedWord aw(Word(1), returnAddress);
  creditsOut.write(aw);
  validCreditOut.write(true);

  numCredits--;

  if (DEBUG)
    cout << "Sent credit from " << channel << " to " << returnAddress << endl;
}

FlowControlIn::FlowControlIn(sc_module_name name, const ComponentID& ID, const ChannelID& channelManaged) :
    Component(name, ID)
{
	channel = channelManaged;
	returnAddress = -1;
	useCredits = true;
	numCredits = 0;

	dataState   = WAITING_FOR_DATA;
	creditState = NO_CREDITS;

	SC_METHOD(dataLoop);
	sensitive << validDataIn.pos();
	dont_initialize();

	SC_METHOD(creditLoop);
	sensitive << newCredit;
	dont_initialize();
}
