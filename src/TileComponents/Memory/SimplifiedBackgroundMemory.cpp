//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Background Memory Implementation
//-------------------------------------------------------------------------------------------------
// Implements a simplified background memory.
//
// Data is queued in input queues coming from the cache banks. Requests get serviced in round-robin
// order and require a fixed parameterisable number of clock cycles to complete. Memory operation
// is fully pipelined.
//-------------------------------------------------------------------------------------------------
// File:       SimplifiedBackgroundMemory.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 17/01/2011
//-------------------------------------------------------------------------------------------------

#include "../../Component.h"
#include "../../Utility/BatchMode/BatchModeEventRecorder.h"
#include "SimplifiedBackgroundMemory.h"

//-------------------------------------------------------------------------------------------------
// Simulation utility methods
//-------------------------------------------------------------------------------------------------

void SimplifiedBackgroundMemory::debugOutputMessage(const char* message, long long arg1 = 0, long long arg2 = 0, long long arg3 = 0) {
	if (!DEBUG)
		return;

	cout << this->name() << ": ";

	char formatMessage[1024];
	sprintf(formatMessage, message, arg1, arg2, arg3);

	cout << formatMessage << endl;
}

//-------------------------------------------------------------------------------------------------
// Processes
//-------------------------------------------------------------------------------------------------

// Memory logic sensitive to negative clock edge

void SimplifiedBackgroundMemory::processMemory() {
	// Update input queues first (allows zero cycle request serving in case the module is idle)

	for (uint bank = 0; bank < cMemoryBanks; bank++) {
		if (iReadEnable[bank].read() || iWriteEnable[bank].read()) {
			debugOutputMessage("Simplified background memory: Received request from bank %lld\n", bank);

			if (iReadEnable[bank].read() && iWriteEnable[bank].read()) {
				cerr << "Error: Read and Write Enable signals to simplified background memory asserted at the same time" << endl;
				continue;
			}

			if (vRequestQueueCounter[bank] == cQueueLength) {
				cerr << "Error: Request queue overflow in simplified background memory" << endl;
				continue;
			}

			vRequestQueueData[bank * cQueueLength + vRequestQueueWriteCursor[bank]].Address = iAddress[bank].read();
			vRequestQueueData[bank * cQueueLength + vRequestQueueWriteCursor[bank]].Data = iWriteData[bank].read();
			vRequestQueueData[bank * cQueueLength + vRequestQueueWriteCursor[bank]].WriteAccess = iWriteEnable[bank].read();

			vRequestQueueWriteCursor[bank] = (vRequestQueueWriteCursor[bank] + 1) % cQueueLength;
			vRequestQueueCounter[bank]++;
		}
	}

	// Service a single memory request (if available)

	vDelayedOutputBuffer[vDelayedOutputWriteCursor].Valid = false;

	for (uint counter = 0; counter < cMemoryBanks; counter++) {
		uint bank = vNextRequestIndex;
		vNextRequestIndex = (vNextRequestIndex + 1) % cMemoryBanks;

		if (vRequestQueueCounter[bank] > 0) {
			uint32_t address = vRequestQueueData[bank * cQueueLength + vRequestQueueReadCursor[bank]].Address;
			uint tableSlot = address >> 16;
			uint subAddress = address & 0xFFFFUL;

			if (vRequestQueueData[bank * cQueueLength + vRequestQueueReadCursor[bank]].WriteAccess) {
				// Write request

				if (vMemoryTable[tableSlot] == NULL) {
					vMemoryTable[tableSlot] = new uint64_t[65536];
					memset(vMemoryTable[tableSlot], 0x00, sizeof(uint64_t) * 65536);
				}

				vMemoryTable[tableSlot][subAddress] = vRequestQueueData[bank * cQueueLength + vRequestQueueReadCursor[bank]].Data;
			} else {
				// Read request

				vDelayedOutputBuffer[vDelayedOutputWriteCursor].BankNumber = bank;
				vDelayedOutputBuffer[vDelayedOutputWriteCursor].Data = (vMemoryTable[tableSlot] == NULL) ? 0 : vMemoryTable[tableSlot][subAddress];
				vDelayedOutputBuffer[vDelayedOutputWriteCursor].Valid = true;
			}

			vRequestQueueReadCursor[bank] = (vRequestQueueReadCursor[bank] + 1) % cQueueLength;
			vRequestQueueCounter[bank]--;

			break;
		}
	}

	// Send data from delayed output buffer (if valid)

	for (uint bank = 0; bank < cMemoryBanks; bank++) {
		oReadData[bank].write(0);
		oAcknowledge[bank].write(false);
	}

	if (vDelayedOutputBuffer[vDelayedOutputReadCursor].Valid) {
		debugOutputMessage("Simplified background memory: Sent data %.16llX to bank %lld\n", vDelayedOutputBuffer[vDelayedOutputReadCursor].Data, vDelayedOutputBuffer[vDelayedOutputReadCursor].BankNumber);

		oReadData[vDelayedOutputBuffer[vDelayedOutputReadCursor].BankNumber].write(vDelayedOutputBuffer[vDelayedOutputReadCursor].Data);
		oAcknowledge[vDelayedOutputBuffer[vDelayedOutputReadCursor].BankNumber].write(true);
	}

	// Update delayed output buffer cursors

	vDelayedOutputWriteCursor = (vDelayedOutputWriteCursor + 1) % (cAccessDelayCycles + 1);
	vDelayedOutputReadCursor = (vDelayedOutputReadCursor + 1) % (cAccessDelayCycles + 1);
}

//-------------------------------------------------------------------------------------------------
// Constructors / Destructors
//-------------------------------------------------------------------------------------------------

SimplifiedBackgroundMemory::SimplifiedBackgroundMemory(sc_module_name name, ComponentID id, BatchModeEventRecorder *eventRecorder, uint memoryBanks, uint queueLength, uint accessDelayCycles) :
	Component(name, id)
{
	// Instrumentation

	vEventRecorder = eventRecorder;

	if (vEventRecorder != NULL)
		vEventRecorder->registerInstance(this, BatchModeEventRecorder::kInstanceSimplifiedBackgroundMemory);

	// Initialise configuration

	cMemoryBanks = memoryBanks;

	if (cMemoryBanks == 0) {
		cerr << "Simplified background memory instantiated with zero memory banks" << endl;
		throw std::exception();
	}

	cQueueLength = queueLength;

	if (cQueueLength < 2) {
		cerr << "Simplified background memory instantiated with underprovisioned queue resources" << endl;
		throw std::exception();
	}

	cAccessDelayCycles = accessDelayCycles;

	// Initialise ports

	iAddress = new sc_in<uint32_t>[cMemoryBanks];
	iWriteData = new sc_in<uint64_t>[cMemoryBanks];
	iReadEnable = new sc_in<bool>[cMemoryBanks];
	iWriteEnable = new sc_in<bool>[cMemoryBanks];

	oReadData = new sc_out<uint64_t>[cMemoryBanks];
	oAcknowledge = new sc_out<bool>[cMemoryBanks];

	for (uint i = 0; i < cMemoryBanks; i++)
		oAcknowledge[i].initialize(false);

	// Initialise state

	vRequestQueueData = new MemoryRequestDescriptor[cMemoryBanks * cQueueLength];
	vRequestQueueWriteCursor = new uint[cMemoryBanks];
	vRequestQueueReadCursor = new uint[cMemoryBanks];
	vRequestQueueCounter = new uint[cMemoryBanks];

	vNextRequestIndex = 0;

	vDelayedOutputBuffer = new MemoryResponseDescriptor[cAccessDelayCycles + 1];
	vDelayedOutputWriteCursor = 0;
	vDelayedOutputReadCursor = 1;

	vMemoryTable = new uint64_t*[65536];

	memset(vRequestQueueWriteCursor, 0x00, sizeof(uint) * cMemoryBanks);
	memset(vRequestQueueReadCursor, 0x00, sizeof(uint) * cMemoryBanks);
	memset(vRequestQueueCounter, 0x00, sizeof(uint) * cMemoryBanks);
	memset(vDelayedOutputBuffer, 0x00, sizeof(MemoryRequestDescriptor) * (cAccessDelayCycles + 1));
	memset(vMemoryTable, 0x00, sizeof(uint64_t*) * 65536);

	// Register processes

	SC_METHOD(processMemory);
	sensitive << iClock.neg();
	dont_initialize();

	// Indicate non-default component constructor

	end_module();
}

SimplifiedBackgroundMemory::~SimplifiedBackgroundMemory() {
	delete[] iAddress;
	delete[] iWriteData;
	delete[] iReadEnable;
	delete[] iWriteEnable;

	delete[] oReadData;
	delete[] oAcknowledge;

	delete[] vRequestQueueData;
	delete[] vRequestQueueWriteCursor;
	delete[] vRequestQueueReadCursor;
	delete[] vRequestQueueCounter;

	delete[] vDelayedOutputBuffer;

	for (uint i = 0; i < 65536; i++)
		delete[] vMemoryTable[i];  // Checks for NULL pointers automatically

	delete[] vMemoryTable;
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// Initialise the memory contents

void SimplifiedBackgroundMemory::setWords(uint32_t address, const uint64_t *data, uint32_t count) {
	for (uint32_t index = 0; index < count; index++) {
		uint32_t currAddress = address + index * 4;
		uint tableSlot = currAddress >> 16;
		uint subAddress = currAddress & 0xFFFFUL;

		if (vMemoryTable[tableSlot] == NULL) {
			vMemoryTable[tableSlot] = new uint64_t[65536];
			memset(vMemoryTable[tableSlot], 0x00, sizeof(uint64_t) * 65536);
		}

		vMemoryTable[tableSlot][subAddress] = data[index];
	}
}

// Retrieve the memory contents

void SimplifiedBackgroundMemory::getWords(uint32_t address, uint64_t *data, uint32_t count) const {
	for (uint32_t index = 0; index < count; index++) {
		uint32_t currAddress = address + index * 4;
		uint tableSlot = currAddress >> 16;
		uint subAddress = currAddress & 0xFFFFUL;

		data[index] = (vMemoryTable[tableSlot] == NULL) ? 0 : vMemoryTable[tableSlot][subAddress];
	}
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods inherited from Component - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// The area of this component in square micrometres

double SimplifiedBackgroundMemory::area() const {
	cerr << "Shared L1 cache area estimation not yet implemented" << endl;
	return 0.0;
}

// The energy consumed by this component in picojoules

double SimplifiedBackgroundMemory::energy() const {
	cerr << "Shared L1 cache energy estimation not yet implemented" << endl;
	return 0.0;
}
