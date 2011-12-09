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
      if(clock.posedge()) {
        // Wait for a delta cycle, because the valid signal is deasserted on
        // the clock edge.
        next_trigger(sc_core::SC_ZERO_TIME);
      }
      else if(!validDataIn[0].read()) {
        // It turns out that there wasn't actually more data: the valid signal
        // was deasserted on the clock edge.
        next_trigger(validDataIn[0].posedge_event());
      }
      else {
        // There definitely is data: send it.
        DataType data = dataIn[0].read();

        outputUsed = getDestinations(data.channelID());
        assert(outputUsed != 0);

        for(int i=0; i<8; i++) {  // 8 = number of bits in PortIndex... increase this?
          if((outputUsed >> i) & 1) {
            dataOut[i].write(data);
//            validDataOut[i].write(true);
          }
        }

        next_trigger(receivedAllAcks);
        state = WAITING_FOR_ACK;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      // All acknowledgements have been received, so now it is time to
      // acknowledge the input data.
      ackDataIn[0].write(true);

      next_trigger(clock.posedge_event());
      state = SENT_ACK;

      break;
    }

    case SENT_ACK : {
      assert(ackDataIn[0].read());

      ackDataIn[0].write(false);

      state = WAITING_FOR_DATA;
      next_trigger(sc_core::SC_ZERO_TIME);

      break;
    }
  } // end switch
}

void MulticastBus::ackArrived(PortIndex port) {
  if(ackDataOut[port].event()) {
    // If we're receiving an ack, we should have sent data on that port.
    assert((outputUsed >> port) & 1);
    outputUsed &= ~(1 << port);  // Clear the bit

    validDataOut[port].write(false);

    // If outputUsed is 0, it means there are no more outstanding
    // acknowledgements.
    if(outputUsed == 0) {
      receivedAllAcks.notify();
    }

    next_trigger(dataOut[port].default_event());
  }
  else {
    // If no ack arrived, it means data is being sent on this port.
    // Since this method is responsible for taking down the validDataOut signal,
    // it must also be responsible for putting it up.
    assert((outputUsed >> port) & 1);

    validDataOut[port].write(true);

    // Wait until the acknowledgement arrives.
    next_trigger(ackDataOut[port].posedge_event());
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
                           HierarchyLevel level, Dimension size, int firstOutput) :
    Bus(name, ID, numOutputs, level, size, firstOutput) {

//  creditsIn      = new CreditInput[numOutputs];
//  validCreditIn  = new ReadyInput[numOutputs];
//  ackCreditIn    = new ReadyOutput[numOutputs];
//
//  creditsOut     = new CreditOutput[1];
//  validCreditOut = new ReadyOutput[1];
//  ackCreditOut   = new ReadyInput[1];

  // Generate a method for each output port, to wait for acknowledgements and
  // notify the main process when all have been received.
  for(int i=0; i<numOutputs; i++)
    SPAWN_METHOD(dataOut[i].value_changed(), MulticastBus::ackArrived, i, false);
}

MulticastBus::~MulticastBus() {
//  delete[] creditsIn;
//  delete[] validCreditIn;
//  delete[] ackCreditIn;
//
//  delete[] creditsOut;
//  delete[] validCreditOut;
//  delete[] ackCreditOut;
}
