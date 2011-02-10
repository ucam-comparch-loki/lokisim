//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Controller Definition
//-------------------------------------------------------------------------------------------------
// Defines the cache controller.
//
// This module handles the translation of memory commands received through the network into
// memory access and the delivery of data being read to the network interface.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheController.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 12/01/2011
//-------------------------------------------------------------------------------------------------

#ifndef SHAREDL1CACHECONTROLLER_HPP_
#define SHAREDL1CACHECONTROLLER_HPP_

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Datatype/Word.h"
#include "../../Utility/BatchMode/BatchModeEventRecorder.h"

class SharedL1CacheController : public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint					cChannel;				// Channel index

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	// Clock

	sc_in<bool>				iClock;					// Clock

	// Network interface ports

	sc_in<Word>				iDataRx;				// Data from queue forwarded to cache controller
	sc_in<bool>				iDataRxAvailable;		// Indicates whether input data is available
	sc_out<bool>			oDataRxAcknowledge;		// Indicates that the data word got consumed by the cache controller

	sc_out<AddressedWord>	oDataTx;				// Data to send to network
	sc_out<bool>			oDataTxEnable;			// Output enable signal
	sc_in<bool>				iDataTxFree;			// Indicates whether output data can be sent

	sc_out<bool>			oIdle;					// Indicates whether the controller is idle

	// Memory ports

	sc_out<uint32_t>		oAddress;				// Memory address
	sc_out<uint64_t>		oData;					// Data word to be written
	sc_out<uint8_t>			oByteMask;				// Byte mask of data to be written
	sc_out<bool>			oReadEnable;			// Read enable signal
	sc_out<bool>			oWriteEnable;			// Write enable signal

	sc_in<uint64_t>			iData;					// Data word read from memory
	sc_in<bool>				iAcknowledge;			// Acknowledgement signal from memory

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	enum FSMState {
		STATE_IDLE,									// No connection established
		STATE_CONNECTED_IDLE,						// Connection established and idle
		STATE_READ_WORD_PENDING,					// Waiting for memory to read word
		STATE_READ_BYTE_PENDING,					// Waiting for memory to read word - byte extraction needed
		STATE_READ_IPK_PENDING,						// Waiting for memory to read word - IPK streaming in progress
		STATE_READ_STALLED,							// Memory access completed but flow control does not permit sending back the data
		STATE_READ_IPK_STALLED,						// IPK related memory access completed but flow control does not permit sending back the data
		STATE_WRITE_WORD_DATA,						// Waiting for data word to write
		STATE_WRITE_BYTE_DATA,						// Waiting for data byte to write
		STATE_WRITE_PENDING							// Waiting for memory to write data (word or byte)
	};

	//---------------------------------------------------------------------------------------------
	// Signals
	//---------------------------------------------------------------------------------------------

private:

	sc_signal<FSMState>		rCurrentState;			// Current state of the controller FSM
	sc_signal<uint32_t>		rRemoteChannel;			// Remote channel of connection established
	sc_signal<uint32_t>		rAddress;				// Current address
	sc_signal<uint8_t>		rByteMask;				// Current byte mask
	sc_signal<uint64_t>		rDataBuffer;			// Delayed data buffer
	sc_signal<uint8_t>		rByteSelect;			// Current byte selector

	sc_signal<FSMState>		sNextState;				// Next state of the controller FSM
	sc_signal<uint32_t>		sRemoteChannel;			// New remote channel of connection established
	sc_signal<uint32_t>		sAddress;				// New address
	sc_signal<uint8_t>		sByteMask;				// New byte mask
	sc_signal<uint64_t>		sDataBuffer;			// New delayed data buffer
	sc_signal<uint8_t>		sByteSelect;			// New byte selector

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

private:

	BatchModeEventRecorder	*vEventRecorder;

	static const int		kPropertyChannelNumber				= 1050;

	static const int		kEvent_STATE_IDLE					= 1;
	static const int		kEvent_STATE_CONNECTED_IDLE			= 2;
	static const int		kEvent_STATE_READ_WORD_PENDING		= 3;
	static const int		kEvent_STATE_READ_BYTE_PENDING		= 4;
	static const int		kEvent_STATE_READ_IPK_PENDING		= 5;
	static const int		kEvent_STATE_READ_STALLED			= 6;
	static const int		kEvent_STATE_READ_IPK_STALLED		= 7;
	static const int		kEvent_STATE_WRITE_WORD_DATA		= 8;
	static const int		kEvent_STATE_WRITE_BYTE_DATA		= 9;
	static const int		kEvent_STATE_WRITE_PENDING			= 10;

	static const int		kEventConnectionSetup				= 20;
	static const int		kEventConnectionSetupStalled		= 21;
	static const int		kEventReadWordStart					= 22;
	static const int		kEventReadByteStart					= 23;
	static const int		kEventReadIPKStart					= 24;
	static const int		kEventWriteWordStart				= 25;
	static const int		kEventWriteByteStart				= 26;

	static const int		kEventChainedOffset					= 10;

	static const int		kEventConnectionSetupChained		= kEventConnectionSetup + kEventChainedOffset;
	static const int		kEventConnectionSetupChainedStalled	= kEventConnectionSetupStalled + kEventChainedOffset;
	static const int		kEventReadWordChained				= kEventReadWordStart + kEventChainedOffset;
	static const int		kEventReadByteChained				= kEventReadByteStart + kEventChainedOffset;
	static const int		kEventReadIPKChained				= kEventReadIPKStart + kEventChainedOffset;
	static const int		kEventWriteWordChained				= kEventWriteWordStart + kEventChainedOffset;
	static const int		kEventWriteByteChained				= kEventWriteByteStart + kEventChainedOffset;

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods
	//---------------------------------------------------------------------------------------------

private:

	void debugOutputMessage(const char* message, long long arg1, long long arg2, long long arg3);

	//---------------------------------------------------------------------------------------------
	// Processes
	//---------------------------------------------------------------------------------------------

private:

	// FSM register logic sensitive to negative clock edge

	void processFSMRegisters();

	// Combinational FSM logic

	void processFSMCombinational();
	void subProcessInitiateRequest(MemoryRequest &vRequest, bool chained);

	// Idle signal steering

	void processIdleSignal();

	//---------------------------------------------------------------------------------------------
	// Constructors / Destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(SharedL1CacheController);
	SharedL1CacheController(sc_module_name name, ComponentID id, BatchModeEventRecorder *eventRecorder, uint channel);
	virtual ~SharedL1CacheController();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// The area of this component in square micrometres

	virtual double area() const;

	// The energy consumed by this component in picojoules

	virtual double energy() const;
};

#endif /* SHAREDL1CACHECONTROLLER_HPP_ */
