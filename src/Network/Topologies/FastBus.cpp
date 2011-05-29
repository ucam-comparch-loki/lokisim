/*
 * FastBus.cpp
 *
 *  Created on: 29 May 2011
 *      Author: afjk2
 */

#include "FastBus.h"

void FastBus::busProcess() {
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This bus implementation is broken - but it is broken in a way the rest of the system depends on
	// Please do not change anything in the following lines without thorough testing of the on-chip network
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (execCycle != cycleCounter) {
		execCycle = cycleCounter;
		execState = 0;
	}

	if (execState == 0 && clock.posedge()) {
		ackDataIn[0].write(false);

		// Allow time for the valid signal to be deasserted

		triggerSignal.write(triggerSignal.read() + 1);
		execState = 1;
	} else if (execState == 1 && triggerSignal.event()) {
	    if (validDataIn[0].read()) {
	        DataType data = dataIn[0].read();

	        output = getDestination(data.channelID());
	        assert(output < numOutputs);

	        dataOut[output].write(data);
	        validDataOut[output].write(true);

	        // Wait until receipt of the data is acknowledged

	        execState = 2;
	    } else {
	    	execState = -1;
	    }
	} else if (execState == 2 && ackDataOut[output].event()) {
	    validDataOut[output].write(false);

	    // Pulse an acknowledgement.
	    ackDataIn[0].write(true);

	    execState = -1;
	}
}

void FastBus::clockProcess() {
	cycleCounter++;
}

void FastBus::computeSwitching() {
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

FastBus::FastBus(sc_module_name name, const ComponentID& ID, int numOutputPorts, HierarchyLevel level, Dimension size) :
	Network(name, ID, 1, numOutputPorts, level, size)
{
	lastData.write(DataType());

	cycleCounter = 0;

	execCycle = 0;
	execState = 0;
	output = 0;

	triggerSignal.write(0);

	SC_METHOD(busProcess);
	sensitive << clock.pos() << triggerSignal;
	for (int i = 0; i < numOutputPorts; i++)
		sensitive << ackDataOut[i];
	dont_initialize();

	SC_METHOD(clockProcess);
	sensitive << clock.pos();
	dont_initialize();

	SC_METHOD(computeSwitching);
	sensitive << dataIn[0];
	dont_initialize();

	end_module();
}
