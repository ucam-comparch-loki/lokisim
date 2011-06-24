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

        outputUsed = getDestination(data.channelID());
        assert(outputUsed < numOutputs);

        // TODO: assert that no more than one word is sent per cycle.
        dataOut[outputUsed].write(data);
        validDataOut[outputUsed].write(true);

        next_trigger(ackDataOut[outputUsed].default_event());
        state = WAITING_FOR_ACK;
      }

      break;
    }

    case WAITING_FOR_ACK : {
      // The data has been consumed, so is no longer valid.
      validDataOut[outputUsed].write(false);

      // Acknowledge the input data.
      ackDataIn[0].write(true);

      next_trigger(clock.posedge_event());
      state = SENT_ACK;

      break;
    }

    case SENT_ACK : {
      assert(ackDataIn[0].read());

      ackDataIn[0].write(false);

      if(!validDataIn[0].read()) {
        next_trigger(validDataIn[0].posedge_event());
        state = WAITING_FOR_DATA;
      }
      else {
        // The valid signal is still high at the start of the next clock cycle.
        // This means there may be more data to consume.
        next_trigger(sc_core::SC_ZERO_TIME);
        state = WAITING_FOR_DATA;
      }

      break;
    }
  } // end switch
}

void Bus::computeSwitching() {
	DataType newData = dataIn[0].read();
	unsigned int dataDiff = newData.payload().toInt() ^ lastData.read().payload().toInt();
	unsigned int channelDiff = newData.channelID().getData() ^ lastData.read().channelID().getData();

	int bitsSwitched = __builtin_popcount(dataDiff) + __builtin_popcount(channelDiff);

	// Is the distance that the switching occurred over:
	//  1. The width of the network (length of the bus), or
	//  2. Width + height?
	Instrumentation::networkActivity(id, 0, 0, size.first, bitsSwitched);

	lastData.write(newData);
}

Bus::Bus(sc_module_name name, const ComponentID& ID, int numOutputPorts, HierarchyLevel level, Dimension size) :
    Network(name, ID, 1, numOutputPorts, level, size)
{
	lastData.write(DataType());

	state = WAITING_FOR_DATA;

	SC_METHOD(busLoop);
	sensitive << validDataIn[0].pos();
	dont_initialize();

	SC_METHOD(computeSwitching);
	sensitive << dataIn[0];
	dont_initialize();
}
