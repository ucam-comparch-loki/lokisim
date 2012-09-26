/*
 * MulticastBus.cpp
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "MulticastBus.h"
#include "../../Datatype/AddressedWord.h"

void MulticastBus::busLoop() {
  switch(state) {
    case WAITING_FOR_DATA : {
      if(!dataIn[0].valid()) {
        // It turns out that there wasn't actually more data: wait until some
        // arrives.
        next_trigger(dataIn[0].default_event());
      }
      else {
        // There definitely is data: send it.
        DataType data = dataIn[0].read();

        outputUsed = getDestinations(data.channelID());
        assert(outputUsed != 0);

        for(int i=0; i<8; i++) {  // 8 = number of bits in PortIndex... increase this?
          if((outputUsed >> i) & 1)
            dataOut[i].write(data);
        }

        next_trigger(receivedAllAcks);
        state = WAITING_FOR_ACK;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      // All acknowledgements have been received, so now it is time to
      // acknowledge the input data.
      dataIn[0].ack();

      next_trigger(dataIn[0].default_event());
      state = WAITING_FOR_DATA;

      break;
    }

  } // end switch
}

void MulticastBus::ackArrived(PortIndex port) {
    // If we're receiving an ack, we should have sent data on that port.
    assert((outputUsed >> port) & 1);
    outputUsed &= ~(1 << port);  // Clear the bit

    // If outputUsed is 0, it means there are no more outstanding
    // acknowledgements.
    if(outputUsed == 0) {
      receivedAllAcks.notify();
    }
}

PortIndex MulticastBus::getDestinations(const ChannelID& address) const {
  // In practice, we would probably only allow multicast addresses, but
  // allowing both options for the moment makes testing easier.

  if(level == COMPONENT && address.isMulticast()) {
    // The address is already correctly encoded.
    return getDestination(address);
  }
  else {
    // If it is not a multicast address, there is only one destination.
    // Shift a single bit to the correct position.
    return 1 << getDestination(address);
  }
}

MulticastBus::MulticastBus(const sc_module_name& name, const ComponentID& ID, int numOutputs,
                           HierarchyLevel level, int firstOutput) :
    Bus(name, ID, numOutputs, level, firstOutput) {

//  creditsIn      = new CreditInput[numOutputs];
//  creditsOut     = new CreditOutput[1];

  // Generate a method for each output port, to wait for acknowledgements and
  // notify the main process when all have been received.
  for(int i=0; i<numOutputs; i++)
    SPAWN_METHOD(dataOut[i].ack_event(), MulticastBus::ackArrived, i, false);
}

MulticastBus::~MulticastBus() {
//  delete[] creditsIn;
//  delete[] creditsOut;
}
