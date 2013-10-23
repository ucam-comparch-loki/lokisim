/*
 * Bus.cpp
 *
 *  Created on: 31 Mar 2011
 *      Author: db434
 */

#include "Bus.h"
#include "../../Utility/Instrumentation.h"

void Bus::busLoop() {
  switch(state) {
    case WAITING_FOR_DATA : {
      if(!dataIn[0].valid()) {
        // It turns out that there wasn't actually more data: wait until some
        // arrives.
        next_trigger(dataIn[0].default_event());
      }
      else {
        unsigned long cycle = Instrumentation::currentCycle();
        assert(lastWriteTime != cycle);
        lastWriteTime = cycle;

        // There definitely is data: send it.
        DataType data = dataIn[0].read();

        outputUsed = getDestination(data.channelID());
        assert(outputUsed < numOutputPorts());

        dataOut[outputUsed].write(data);

        cout << this->name() << " sent " << data << endl;

        next_trigger(dataOut[outputUsed].ack_event());
        state = WAITING_FOR_ACK;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      // Acknowledge the input data.
      dataIn[0].ack();

      // Wait until the next clock edge to deassert the acknowledgement.
      next_trigger(dataIn[0].default_event());
      state = WAITING_FOR_DATA;

      break;
    }

  } // end switch
}

Bus::Bus(const sc_module_name& name, const ComponentID& ID, int numOutputPorts, HierarchyLevel level, int firstOutput) :
    Network(name, ID, 1, numOutputPorts, level, firstOutput)
{
  lastWriteTime = -1;

  state = WAITING_FOR_DATA;

  SC_METHOD(busLoop);
  // do initialise
}
