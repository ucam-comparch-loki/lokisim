//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Simplified On-Chip Scratchpad Definition
//-------------------------------------------------------------------------------------------------
// Second version of simplified background memory model.
//
// Models a simplified on-chip scratchpad with an unlimited number of ports.
// The scratchpad is connected through an abstract on-chip network and
// serves as the background memory for the cache mode(s) of the memory banks.
//
// The number of input and output ports is configurable. The ports possess
// queues for incoming and outgoing data.
//
// Colliding write requests are executed in ascending port order.
//
// Communication protocol:
//
// 1. Send start address and operation
// 2. Send words in case of write command
//-------------------------------------------------------------------------------------------------
// File:       SimplifiedOnChipScratchpad.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 08/04/2011
//-------------------------------------------------------------------------------------------------

#ifndef SIMPLIFIEDONCHIPSCRATCHPAD_H
#define SIMPLIFIEDONCHIPSCRATCHPAD_H

#include "../../Component.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Network/BufferArray.h"

class SimplifiedOnChipScratchpad: public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint						cDelayCycles;			// Number of clock cycles requests are delayed
	uint						cBanks;					// Number of memory banks

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	sc_in<bool>					iClock;					// Clock

	sc_out<bool>				oIdle;					// Signal that this component is not currently doing any work

	sc_in<bool>					*iDataStrobe;			// Indicate that corresponding input data word is valid
	sc_in<MemoryRequest>		*iData;					// Memory request words input from cache controllers

	sc_out<bool>				*oDataStrobe;			// Indicate that corresponding output data word is valid
	sc_out<Word>				*oData;					// Data words output to cache controllers

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	enum PortState {
		STATE_IDLE,
		STATE_READING,
		STATE_WRITING
	};

	struct PortData {
	public:
		PortState State;
		uint32_t WordsLeft;
		uint32_t Address;
	};

	struct InputWord {
	public:
		uint64_t EarliestExecutionCycle;
		MemoryRequest Request;
	};

	//---------------------------------------------------------------------------------------------
	// Local state
	//---------------------------------------------------------------------------------------------

private:

	uint64_t mCycleCounter;								// Cycle counter used for delay control

	uint32_t *mData;									// Data words stored in the scratchpad

	uint mPortCount;									// Number of ports
	PortData *mPortData;								// State information about all ports
	BufferArray<InputWord> mInputQueues;				// Input queues for all ports

	//---------------------------------------------------------------------------------------------
	// Internal functions
	//---------------------------------------------------------------------------------------------

private:

	void tryStartRequest(uint port);					// Helper function starting / chaining new request

	void mainLoop();									// Main loop thread - running at every positive clock edge

	//---------------------------------------------------------------------------------------------
	// Constructors and destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(SimplifiedOnChipScratchpad);
	SimplifiedOnChipScratchpad(sc_module_name name, const ComponentID& ID, uint portCount);
	~SimplifiedOnChipScratchpad();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	//void flushQueues();

	void storeData(vector<Word>& data, MemoryAddr location);
	const void* getData();

	void print(MemoryAddr start, MemoryAddr end);
	Word readWord(MemoryAddr addr);
	Word readByte(MemoryAddr addr);
	void writeWord(MemoryAddr addr, Word data);
	void writeByte(MemoryAddr addr, Word data);

	virtual double area() const;						// The area of this component in square micrometres
	virtual double energy() const;						// The energy consumed by this component in picojoules
};

#endif /* SIMPLIFIEDONCHIPSCRATCHPAD_H */
