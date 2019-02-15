/*
 * MulticastBus.cpp
 *
 *  Created on: 8 Mar 2011
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "MulticastBus.h"
#include "../../Utility/Assert.h"

using sc_core::sc_module_name;

void MulticastBus::busLoop() {
  switch (state) {
    case WAITING_FOR_DATA : {
      if (!iData.valid()) {
        // It turns out that there wasn't actually more data: wait until some
        // arrives.
        next_trigger(iData.default_event());
      }
      else {
        // There definitely is data: send it.
        NetworkData data = iData.read();
        assert(data.channelID().multicast);

        outputUsed = data.channelID().coremask;
        loki_assert(outputUsed != 0);

        for (int i=0; i<8; i++) {  // 8 = number of bits in PortIndex... increase this?
          if ((outputUsed >> i) & 1)
            oData[i].write(data);
        }

        next_trigger(receivedAllAcks);
        state = WAITING_FOR_ACK;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      // All acknowledgements have been received, so now it is time to
      // acknowledge the input data.
      iData.ack();

      next_trigger(iData.default_event());
      state = WAITING_FOR_DATA;

      break;
    }

  } // end switch
}

void MulticastBus::ackArrived(PortIndex port) {
    // If we're receiving an ack, we should have sent data on that port.
    loki_assert((outputUsed >> port) & 1);
    outputUsed &= ~(1 << port);  // Clear the bit

    // If outputUsed is 0, it means there are no more outstanding
    // acknowledgements.
    if (outputUsed == 0) {
      receivedAllAcks.notify();
    }
}

MulticastBus::MulticastBus(const sc_module_name& name, int numOutputs) :
    Bus(name, numOutputs) {

  // Generate a method for each output port, to wait for acknowledgements and
  // notify the main process when all have been received.
  for (int i=0; i<numOutputs; i++)
    SPAWN_METHOD(oData[i].ack_finder(), MulticastBus::ackArrived, i, false);
}
