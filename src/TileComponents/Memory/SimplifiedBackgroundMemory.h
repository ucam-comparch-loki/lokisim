//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Background Memory Definition
//-------------------------------------------------------------------------------------------------
// Defines a simplified background memory.
//
// Data is queued in input queues coming from the cache banks. Requests get serviced in round-robin
// order and require a fixed parameterisable number of clock cycles to complete. Memory operation
// is fully pipelined.
//-------------------------------------------------------------------------------------------------
// File:       SimplifiedBackgroundMemory.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 17/01/2011
//-------------------------------------------------------------------------------------------------

#ifndef SIMPLIFIEDBACKGROUNDMEMORY_HPP_
#define SIMPLIFIEDBACKGROUNDMEMORY_HPP_

#include "../../Component.h"
#include "../../Utility/BatchMode/BatchModeEventRecorder.h"

class SimplifiedBackgroundMemory : public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint					cMemoryBanks;			// Number of memory banks
	uint					cQueueLength;			// Number of input queue entries for each memory bank
	uint					cAccessDelayCycles;		// Number of clock cycles all responses are delayed

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	// Clock

	sc_in<bool>				iClock;					// Clock

	// Ports to cache banks

	sc_in<uint32_t>			*iAddress;				// Addresses input from cache controllers
	sc_in<uint64_t>			*iWriteData;			// Data words input from cache controllers
	sc_in<bool>				*iReadEnable;			// Read enable signals input from cache controllers
	sc_in<bool>				*iWriteEnable;			// Write enable signals input from cache controllers

	sc_out<uint64_t>		*oReadData;				// Data words output to cache controllers
	sc_out<bool>			*oAcknowledge;			// Acknowledgement signals output to cache controllers

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	struct MemoryRequestDescriptor {
	public:
		uint32_t Address;							// Address (word aligned)
		uint64_t Data;								// Data (full words only)
		bool WriteAccess;							// Indicates that this is a write access (read access otherwise)
	};

	struct MemoryResponseDescriptor {
	public:
		uint BankNumber;							// Number of memory bank to send data to
		uint64_t Data;								// Data (full words only)
		bool Valid;									// Indicates that this is valid data to send
	};

	//---------------------------------------------------------------------------------------------
	// State
	//---------------------------------------------------------------------------------------------

private:

	// This is just a simulation model - state is maintained in variables

	MemoryRequestDescriptor *vRequestQueueData;
	uint *vRequestQueueWriteCursor;
	uint *vRequestQueueReadCursor;
	uint *vRequestQueueCounter;

	uint vNextRequestIndex;

	MemoryResponseDescriptor *vDelayedOutputBuffer;
	uint vDelayedOutputWriteCursor;
	uint vDelayedOutputReadCursor;

	uint64_t **vMemoryTable;

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	BatchModeEventRecorder *vEventRecorder;

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods
	//---------------------------------------------------------------------------------------------

private:

	void debugOutputMessage(const char* message, long long arg1, long long arg2, long long arg3);

	//---------------------------------------------------------------------------------------------
	// Processes
	//---------------------------------------------------------------------------------------------

private:

	// Memory logic sensitive to negative clock edge

	void processMemory();

	//---------------------------------------------------------------------------------------------
	// Constructors / Destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(SimplifiedBackgroundMemory);
	SimplifiedBackgroundMemory(sc_module_name name, ComponentID id, BatchModeEventRecorder *eventRecorder, uint memoryBanks, uint queueLength, uint accessDelayCycles);
	virtual ~SimplifiedBackgroundMemory();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// Initialise the memory contents

	virtual void setWords(uint32_t address, const uint64_t *data, uint32_t count);

	// Retrieve the memory contents

	virtual void getWords(uint32_t address, uint64_t *data, uint32_t count) const;

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// The area of this component in square micrometres

	virtual double area() const;

	// The energy consumed by this component in picojoules

	virtual double energy() const;
};

#endif /* SIMPLIFIEDBACKGROUNDMEMORY_HPP_ */
