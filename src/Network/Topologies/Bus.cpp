/*
 * Bus.cpp
 *
 *  Created on: 31 Mar 2011
 *      Author: db434
 */

#include "Bus.h"

void Bus::busLoop() {
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

        outputUsed = getDestination(data.channelID());
        assert(outputUsed < numOutputPorts());

        dataOut[outputUsed].write(data);

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

void Bus::computeSwitching() {
  unsigned long cycle = Instrumentation::currentCycle();
  assert(lastWriteTime != cycle);
  lastWriteTime = cycle;

  DataType newData = dataIn[0].read();
  unsigned int dataDiff = newData.payload().toInt() ^ lastData.payload().toInt();
  unsigned int channelDiff = newData.channelID().getData() ^ lastData.channelID().getData();

  int bitsSwitched = __builtin_popcount(dataDiff) + __builtin_popcount(channelDiff);

  // Is the distance that the switching occurred over:
  //  1. The width of the network (length of the bus), or
  //  2. Width + height?
  Instrumentation::networkActivity(id, 0, 0, size.first, bitsSwitched);

  lastData = newData;
}

Bus::Bus(const sc_module_name& name, const ComponentID& ID, int numOutputPorts, HierarchyLevel level, Dimension size, int firstOutput) :
    Network(name, ID, 1, numOutputPorts, level, size, firstOutput)
{
  lastData = DataType();
  lastWriteTime = -1;

  state = WAITING_FOR_DATA;

  SC_METHOD(busLoop);
  // do initialise

  SC_METHOD(computeSwitching);
  sensitive << dataIn[0];
  dont_initialize();
}
