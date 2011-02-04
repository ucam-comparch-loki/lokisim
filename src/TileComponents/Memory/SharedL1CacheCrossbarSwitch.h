//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Crossbar Switch Definition
//-------------------------------------------------------------------------------------------------
// Defines a semi-combinational crossbar switch connecting the cache controllers with the
// memory banks.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheCrossbarSwitch.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 12/01/2011
//-------------------------------------------------------------------------------------------------

#ifndef SHAREDL1CACHECROSSBARSWITCH_HPP_
#define SHAREDL1CACHECROSSBARSWITCH_HPP_

#include "../../Component.h"
#include "../../Utility/BatchMode/BatchModeEventRecorder.h"

class SharedL1CacheCrossbarSwitch : public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint					cChannels;				// Number of channels
	uint					cMemoryBanks;			// Number of memory banks

	uint					cAddressTagBits;		// Number of high order bits used to generate cache tags
	uint					cBankSelectionBits;		// Number of intermediate bits used to select memory bank
	uint					cCacheLineBits;			// Number of low order bits used to select position in cache line

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	// Clock

	sc_in<bool>				iClock;					// Clock

	// Ports to cache controller

	sc_in<uint32_t>			*iAddress;				// Addresses input from cache controllers (one per channel)
	sc_in<uint64_t>			*iWriteData;			// Data words input from cache controllers (one per channel)
	sc_in<uint8_t>			*iByteMask;				// Byte masks input from cache controllers (one per channel)
	sc_in<bool>				*iReadEnable;			// Read enable signals input from cache controllers (one per channel)
	sc_in<bool>				*iWriteEnable;			// Write enable signals input from cache controllers (one per channel)

	sc_out<uint64_t>		*oReadData;				// Data words output to cache controllers (one per channel)
	sc_out<bool>			*oAcknowledge;			// Acknowledgement signals output to cache controllers (one per channel)

	// Ports to memory banks

	sc_out<uint32_t>		*oAddress;				// Addresses output to memory banks (one per memory bank)
	sc_out<uint64_t>		*oWriteData;			// Data words output to memory banks (one per memory bank)
	sc_out<uint8_t>			*oByteMask;				// Byte masks output to memory banks (one per memory bank)
	sc_out<bool>			*oReadEnable;			// Read enable signals output to memory banks (one per memory bank)
	sc_out<bool>			*oWriteEnable;			// Write enable signals output to memory banks (one per memory bank)

	sc_in<uint64_t>			*iReadData;				// Data words input from memory banks (one per memory bank)
	sc_in<bool>				*iAcknowledge;			// Acknowledgement signals input from memory banks (one per memory bank)

	//---------------------------------------------------------------------------------------------
	// Signals
	//---------------------------------------------------------------------------------------------

private:

	sc_signal<bool>			*rBankForwardConnection;		// Indicates that a memory bank is connected to a particular cache controller channel
	sc_signal<uint>			*rBankForwardChannel;			// Cache controller channel connected to memory bank for forward path

	sc_signal<bool>			*rChannelBackwardConnection;	// Indicates that a cache controller channel is connected to a particular memory bank
	sc_signal<uint>			*rChannelBackwardBank;			// Memory bank connected to cache controller channel for backward path

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	BatchModeEventRecorder	*vEventRecorder;

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods
	//---------------------------------------------------------------------------------------------

private:

	void debugOutputMessage(const char* message, long long arg1, long long arg2, long long arg3);

	//---------------------------------------------------------------------------------------------
	// Processes
	//---------------------------------------------------------------------------------------------

private:

	// Check all input ports for new data and update output ports

	void processInputChanged();

	// Updates the connection state of the switch - sensitive to negative clock edge

	void processUpdateState();

	//---------------------------------------------------------------------------------------------
	// Constructors / Destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(SharedL1CacheCrossbarSwitch);
	SharedL1CacheCrossbarSwitch(sc_module_name name, ComponentID id, BatchModeEventRecorder *eventRecorder, uint channels, uint memoryBanks, uint cacheLineSize);
	virtual ~SharedL1CacheCrossbarSwitch();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// The area of this component in square micrometres

	virtual double area() const;

	// The energy consumed by this component in picojoules

	virtual double energy() const;
};

#endif /* SHAREDL1CACHECROSSBARSWITCH_HPP_ */
