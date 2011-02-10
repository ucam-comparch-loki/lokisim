//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Network Interface Implementation
//-------------------------------------------------------------------------------------------------
// Implements the interface logic between the on-chip network and the cache controller.
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
// File:       SharedL1CacheNetworkInterface.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 12/01/2011
//-------------------------------------------------------------------------------------------------

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/Word.h"
#include "../../Utility/BatchMode/BatchModeEventRecorder.h"
#include "SharedL1CacheNetworkInterface.h"

//-------------------------------------------------------------------------------------------------
// Processes
//-------------------------------------------------------------------------------------------------

// Called whenever iDataRx might have changed in the immediately preceding delta cycle and loads the data into the queue

void SharedL1CacheNetworkInterface::processInputDataChanged() {
	// Workaround for old event driven interface

	if(iDataRx.event()) {
		if (vInputDataChanged) {
			cerr << "Error: Observed multiple input data changes during clock cycle" << endl;
		} else {
			if (DEBUG) {
				cout << this->name();

				char formatMessage[1024];
				sprintf(formatMessage, ": Received word 0x%.16llX", iDataRx.read().toLong());

				cout << formatMessage << endl;
			}

			vInputDataChanged = true;
			vNewInputData = iDataRx.read();
		}
	}
}

// Called whenever iFlowControlTx might have changed in the immediately preceding delta cycle and forwards the value (workaround for limited SystemC port bindings)

void SharedL1CacheNetworkInterface::processFlowControlChanged() {
	oDataTxFree.write(iFlowControlTx.read());
}

// Called at the negative clock edge to modify the queue registers

void SharedL1CacheNetworkInterface::processQueueRegisters() {
	if (DEBUG) {
		if (vInputDataChanged) {
			cout << this->name();

			char formatMessage[1024];
			sprintf(formatMessage, ": Inserted word 0x%.16llX into queue", vNewInputData.toLong());

			cout << formatMessage << endl;
		}

		if (iDataRxAcknowledge.read()) {
			cout << this->name();

			char formatMessage[1024];
			sprintf(formatMessage, ": Removed word 0x%.16llX from queue", rQueueData[rQueueReadCursor.read()].read().toLong());

			cout << formatMessage << endl;
		}
	}

	if (vInputDataChanged && iDataRxAcknowledge.read()) {
		// Read and write concurrently

		if (rQueueCounter.read() == 0) {
			cerr << "Error: Read from empty input queue" << endl;
		} else {
			rQueueReadCursor.write((rQueueReadCursor.read() + 1) % cQueueDepth);
			oFlowControlRx.write(1);	// Hack for old credit interface
		}

		rQueueData[rQueueWriteCursor.read()].write(vNewInputData);
		rQueueWriteCursor.write((rQueueWriteCursor.read() + 1) % cQueueDepth);

		// Do not change counter

		vInputDataChanged = false;
	} else if (vInputDataChanged) {
		// Write only

		if (rQueueCounter.read() == cQueueDepth) {
			cerr << "Error: Write to full input queue" << endl;
		} else {
			rQueueData[rQueueWriteCursor.read()].write(vNewInputData);
			rQueueWriteCursor.write((rQueueWriteCursor.read() + 1) % cQueueDepth);
			rQueueCounter.write(rQueueCounter.read() + 1);
		}

		vInputDataChanged = false;
	} else if (iDataRxAcknowledge.read()) {
		// Read only

		if (rQueueCounter.read() == 0) {
			cerr << "Error: Read from empty input queue" << endl;
		} else {
			rQueueReadCursor.write((rQueueReadCursor.read() + 1) % cQueueDepth);
			rQueueCounter.write(rQueueCounter.read() - 1);
			oFlowControlRx.write(1);	// Hack for old credit interface
		}
	}
}

// Handles output connections of the queue

void SharedL1CacheNetworkInterface::processQueueCombinational() {
	oDataRx.write(rQueueData[rQueueReadCursor.read()].read());
	oDataRxAvailable.write(rQueueCounter.read() != 0);
}

// Called at the negative clock edge to send data to the network

void SharedL1CacheNetworkInterface::processSendData() {
	if (iDataTxEnable.read()) {
		if (!iFlowControlTx.read())
			cerr << "Error: Flow control violation while sending data" << endl;
		else
			oDataTx.write(iDataTx.read());
	}
}

//-------------------------------------------------------------------------------------------------
// Constructors / Destructors
//-------------------------------------------------------------------------------------------------

SharedL1CacheNetworkInterface::SharedL1CacheNetworkInterface(sc_module_name name, ComponentID id, BatchModeEventRecorder *eventRecorder, uint channel, uint queueDepth) :
	Component(name, id)
{
	// Instrumentation

	vEventRecorder = eventRecorder;

	if (vEventRecorder != NULL) {
		vEventRecorder->registerInstance(this, BatchModeEventRecorder::kInstanceSharedL1CacheNetworkInterface);
		vEventRecorder->setInstanceProperty(this, kPropertyChannelNumber, channel);
	}

	// Check and setup configuration parameters

	if (queueDepth == 0) {
		cerr << "Shared L1 cache network interface instantiated with zero queue depth" << endl;
		throw std::exception();
	}

	cQueueDepth = queueDepth;

	// Setup configuration dependent structures

	rQueueData = new sc_signal<Word>[cQueueDepth];

	// Setup utility data structures

	vInputDataChanged = false;

	// Connect signals

	//oDataTxFree.bind(iFlowControlTx);
	//iFlowControlTx.bind(oDataTxFree);

	// Initialise ports and

	oFlowControlRx.initialize(0);
	oIdle.initialize(true);

	oDataRxAvailable.initialize(false);
	oDataTxFree.initialize(false);

	// Initialise signals

	rQueueWriteCursor.write(0);
	rQueueReadCursor.write(0);
	rQueueCounter.write(0);

	// Register processes

	SC_METHOD(processInputDataChanged);
	sensitive << iDataRx;
	dont_initialize();

	SC_METHOD(processFlowControlChanged);
	sensitive << iFlowControlTx;
	dont_initialize();

	SC_METHOD(processQueueRegisters);
	sensitive << iClock.neg();
	dont_initialize();

	SC_METHOD(processQueueCombinational);
	sensitive << rQueueReadCursor << rQueueCounter;
	dont_initialize();

	SC_METHOD(processSendData);
	sensitive << iClock.neg();
	dont_initialize();

	// Indicate non-default component constructor

	end_module();
}

SharedL1CacheNetworkInterface::~SharedL1CacheNetworkInterface() {
	delete[] rQueueData;
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods inherited from TileComponent - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// The area of this component in square micrometres

double SharedL1CacheNetworkInterface::area() const {
	cerr << "Shared L1 cache area estimation not yet implemented" << endl;
	return 0.0;
}

// The energy consumed by this component in picojoules

double SharedL1CacheNetworkInterface::energy() const {
	cerr << "Shared L1 cache energy estimation not yet implemented" << endl;
	return 0.0;
}
