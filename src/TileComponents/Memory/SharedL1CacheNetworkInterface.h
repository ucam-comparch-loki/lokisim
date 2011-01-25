//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Network Interface Definition
//-------------------------------------------------------------------------------------------------
// Defines the interface logic between the on-chip network and the cache controller.
//
// Inputs from the network are buffered in an input queue and therefore implicitly registered.
//
// Protocol for communicating with a memory (currently identical to the old system):
//  1. Claim a port in the usual way (invisible to the programmer/compiler).
//     This is currently done automatically whenever setchmap is executed.
//  2. Send a MemoryRequest to that port, specifying the channel ID to send
//     results to, and saying that the MemoryRequest is to set up a connection:
//     MemoryRequest(channel id, MemoryRequest::SETUP);
//  3. Use the load and store instructions to send addresses/data to the
//     memory.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheNetworkInterface.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 12/01/2011
//-------------------------------------------------------------------------------------------------

#ifndef SHAREDL1CACHENETWORKINTERFACE_H_
#define SHAREDL1CACHENETWORKINTERFACE_H_

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Word.h"

class SharedL1CacheNetworkInterface : public Component {
	//---------------------------------------------------------------------------------------------
	// Configuration parameters
	//---------------------------------------------------------------------------------------------

private:

	uint					cQueueDepth;			// Maximum number of words in input queue

	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	// Clock

	sc_in<bool>				iClock;					// Clock

	// Network ports

	sc_in<Word>				iDataRx;				// Data input from the network
	sc_out<int>				oFlowControlRx;			// Flow control signal indicating remaining space in input queue

	sc_out<AddressedWord>	oDataTx;				// Data output to the network
	sc_in<bool>				iFlowControlTx;			// Flow control signal indicating that a word can be sent through the network

	sc_out<bool>			oIdle;					// Idleness signal

	// Cache controller ports

	sc_out<Word>			oDataRx;				// Data from queue forwarded to cache controller
	sc_out<bool>			oDataRxAvailable;		// Indicates whether input data is available
	sc_in<bool>				iDataRxAcknowledge;		// Indicates that the data word got consumed by the cache controller

	sc_in<AddressedWord>	iDataTx;				// Data to send to network
	sc_in<bool>				iDataTxEnable;			// Output enable signal
	sc_out<bool>			oDataTxFree;			// Indicates whether output data can be sent

	//---------------------------------------------------------------------------------------------
	// Utility definitions
	//---------------------------------------------------------------------------------------------

	bool					vInputDataChanged;		// Workaround for old event driven interface
	Word					vNewInputData;			// Workaround for old event driven interface

	//---------------------------------------------------------------------------------------------
	// Signals
	//---------------------------------------------------------------------------------------------

private:

	sc_signal<Word>			*rQueueData;			// Data stored in input queue
	sc_signal<uint>			rQueueWriteCursor;		// Input queue write cursor
	sc_signal<uint>			rQueueReadCursor;		// Input queue read cursor
	sc_signal<uint>			rQueueCounter;			// Number of valid elements in queue

	//---------------------------------------------------------------------------------------------
	// Processes
	//---------------------------------------------------------------------------------------------

private:

	// Called whenever iDataRx might have changed in the immediately preceding delta cycle and loads the data into the queue

	void processInputDataChanged();

	// Called at the negative clock edge to modify the queue registers

	void processQueueRegisters();

	// Handles output connections of the queue

	void processQueueCombinational();

	// Called at the negative clock edge to send data to the network

	void processSendData();

	//---------------------------------------------------------------------------------------------
	// Constructors / Destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(SharedL1CacheNetworkInterface);
	SharedL1CacheNetworkInterface(sc_module_name name, ComponentID id, uint queueDepth);
	virtual ~SharedL1CacheNetworkInterface();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// The area of this component in square micrometres

	virtual double area() const;

	// The energy consumed by this component in picojoules

	virtual double energy() const;
};

#endif /* SHAREDL1CACHENETWORKINTERFACE_H_ */
