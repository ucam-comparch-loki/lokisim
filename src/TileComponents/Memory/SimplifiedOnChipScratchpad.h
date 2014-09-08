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
#include "../../Datatype/Packets/Packet.h"
#include "../../Network/BufferArray.h"


class SimplifiedOnChipScratchpad: public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	cycle_count_t		cDelayCycles;		// Number of clock cycles requests are delayed
	uint						cBanks;					// Number of memory banks

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	ClockInput					iClock;					// Clock

	RequestInput        iRequest;       // Requests from MemoryBanks for data.
	ReadyOutput         oReadyRequest;

	ResponseInput       iResponse;      // Flushed cache lines from MemoryBanks.
	ReadyOutput         oReadyResponse;

	RequestOutput       oRequest;       // Requests to MemoryBanks.

	ResponseOutput      oResponse;      // Cache lines to MemoryBanks.

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
		uint32_t MemoryAddress;
		ChannelID NetworkAddress;
	};

	struct InputWord {
	public:
		cycle_count_t  EarliestExecutionCycle;
		NetworkRequest Request;
	};

	//---------------------------------------------------------------------------------------------
	// Local state
	//---------------------------------------------------------------------------------------------

private:

	enum Input {
	  INPUT_REQUEST,
	  INPUT_RESPONSE
	};
	Input                   priorityInput;      // Arbitrate fairly between input ports

	cycle_count_t           mCycleCounter;			// Cycle counter used for delay control

	uint32_t               *mData;							// Data words stored in the scratchpad

	Packet<Word>           *mInputPacket;       // Data being received.
	Packet<Word>           *mOutputPacket;      // Data being sent.
	bool                    finishedPacket;     // Finished receiving a packet.

	uint                    mPortCount;					// Number of ports
	PortData               *mPortData;					// State information about all ports
	BufferArray<InputWord>  mInputQueues;				// Input queues for all ports

	// Mainly for debug, mark the read-only sections of the address space.
	std::vector<MemoryAddr> readOnlyBase,
	                        readOnlyLimit;

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

	ChannelID networkAddress() const;

	void storeData(vector<Word>& data, MemoryAddr location, bool readOnly);
	const void* getData();

	bool readOnly(MemoryAddr addr) const;     // Check whether a memory location is read-only

	void print(MemoryAddr start, MemoryAddr end);
	Word readWord(MemoryAddr addr);
	Word readByte(MemoryAddr addr);
	void writeWord(MemoryAddr addr, Word data);
	void writeByte(MemoryAddr addr, Word data);
};

#endif /* SIMPLIFIEDONCHIPSCRATCHPAD_H */
