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

	if (execState == STATE_INIT) {
		// First call

		execState = STATE_STANDBY;
		execCycle = 0;
		next_trigger(validDataIn[0].default_event());
	} else if (execState == STATE_STANDBY) {
		if (sc_core::sc_time_stamp().value() % 1000 != 0) {
			execState = STATE_STANDBY;
			next_trigger(clock.posedge_event());
		} else  if (execCycle + 750 <= sc_core::sc_time_stamp().value() && validDataIn[0].read()) {
			DataType data = dataIn[0].read();

			output = getDestination(data.channelID());
			assert(output < numOutputs);

			dataOut[output].write(data);
			validDataOut[output].write(true);

			// Wait until receipt of the data is acknowledged

			execState = STATE_OUTPUT_VALID;
			execCycle = sc_core::sc_time_stamp().value();
			next_trigger(ackDataOut[output].default_event());
		} else {
			execState = STATE_STANDBY;
			next_trigger(validDataIn[0].default_event() & clock.posedge_event());
		}
	} else if (execState == STATE_OUTPUT_VALID) {
	    validDataOut[output].write(false);

	    // Pulse an acknowledgement.
	    ackDataIn[0].write(true);

		execState = STATE_ACKNOWLEDGED;
		next_trigger(clock.posedge_event());
	} else if (execState == STATE_ACKNOWLEDGED) {
		ackDataIn[0].write(false);

		// Allow time for the valid signal to be deasserted.
		triggerSignal.write(triggerSignal.read() + 1);
		execState = STATE_STANDBY;
		next_trigger(triggerSignal.default_event());
	}
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

	execState = STATE_INIT;
	execCycle = 0;
	output = 0;

	triggerSignal.write(0);

	SC_METHOD(busProcess);

	SC_METHOD(computeSwitching);
	sensitive << dataIn[0];
	dont_initialize();

	end_module();
}
