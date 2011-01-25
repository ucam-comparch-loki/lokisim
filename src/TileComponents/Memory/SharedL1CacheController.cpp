//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Controller Implementation
//-------------------------------------------------------------------------------------------------
// Implements the cache controller.
//
// This module handles the translation of memory commands received through the network into
// memory access and the delivery of data being read to the network interface.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheController.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 12/01/2011
//-------------------------------------------------------------------------------------------------

#include "../../Component.h"
#include "../../Datatype/AddressedWord.h"
#include "../../Datatype/ChannelRequest.h"
#include "../../Datatype/Data.h"
#include "../../Datatype/Instruction.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Datatype/Word.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Parameters.h"
#include "SharedL1CacheController.h"

//-------------------------------------------------------------------------------------------------
// Processes
//-------------------------------------------------------------------------------------------------

// FSM register logic sensitive to negative clock edge

void SharedL1CacheController::processFSMRegisters() {
	rCurrentState.write(sNextState.read());
	rRemoteChannel.write(sRemoteChannel.read());
	rAddress.write(sAddress.read());
	rByteMask.write(sByteMask.read());
}

// Combinational FSM logic

void SharedL1CacheController::processFSMCombinational() {
	// Update instrumentation

	Instrumentation::idle(id, rCurrentState.read() == STATE_IDLE || rCurrentState.read() == STATE_CONNECTED_IDLE);

	// Convert memory request

	MemoryRequest vRequest = static_cast<MemoryRequest>(iDataRx.read());

	// Break dependency chains on port and signal values to make the following code more compact

	oDataRxAcknowledge.write(false);
	oDataTx.write(AddressedWord());		// Should really be don't care
	oDataTxEnable.write(false);

	oAddress.write(0);					// Should really be don't care
	oData.write(0);						// Should really be don't care
	oByteMask.write(0);					// Should really be don't care
	oReadEnable.write(false);
	oWriteEnable.write(false);

	// Implicitly generate state based clock enable signals by holding register values whenever they are not changed (saves both area and energy)

	sRemoteChannel.write(rRemoteChannel.read());
	sAddress.write(rAddress.read());
	sByteMask.write(rByteMask.read());
	sDataBuffer.write(rDataBuffer.read());
	sByteSelect.write(rByteSelect.read());

	// Steer port and signal values based on current FSM state and input port values

	switch (rCurrentState.read()) {
	case STATE_IDLE:
		// No connection established

		if (iDataRxAvailable.read() && iDataTxFree.read()) {
			// New request data is available and flow control permits sending message to client

			// Remove request from input queue

			oDataRxAcknowledge.write(true);

			// There is no connection set up at the moment - this must be a setup request, otherwise there is an error

			if (vRequest.isSetup()) {
				if (DEBUG)
					cout << this->name() << " set-up connection at channel " << (int)cChannel << endl;

				// Load response address into register

				sRemoteChannel.write(vRequest.address());

				// Send response word to client

				oDataTx.write(AddressedWord(Word(id * NUM_MEMORY_OUTPUTS + cChannel), vRequest.address(), true));
				oDataTxEnable.write(true);

				sNextState.write(STATE_CONNECTED_IDLE);
			} else {
				cerr << "Error: unexpected input at " << this->name() << " channel " << (int)cChannel << ": " << vRequest << endl;

				sNextState.write(STATE_IDLE);
			}
		} else {
			// No new request available or sending data not permitted - stay idle

			sNextState.write(STATE_IDLE);
		}

		break;

	case STATE_CONNECTED_IDLE:
		// Connection established and idle

		// Check whether there is an request ready and initiate it

		subProcessInitiateRequest(vRequest);

		break;

	case STATE_READ_WORD_PENDING:
		// Waiting for memory to read word

		if (iAcknowledge.read()) {
			// Memory access completed - data must be consumed and request signals cleared unless there is another request

			if (DEBUG)
				cout << "Read word " << iData.read() << " from memory " << id << ", address " << rAddress.read() << endl;

			if (iDataTxFree.read()) {
				// Sending back data is possible - do so immediately

				oDataTx.write(AddressedWord(Word(iData.read()), rRemoteChannel.read()));
				oDataTxEnable.write(true);

				// Check whether chaining another request without stall cycle is possible

				subProcessInitiateRequest(vRequest);
			} else {
				// Flow control does not permit sending back data - stall read operation (ports cleared implicitly)

				// Buffer data read in controller until it can be sent back to client

				sDataBuffer.write(iData.read());

				sNextState.write(STATE_READ_STALLED);
			}
		} else {
			// Memory access not yet completed - hold request signals and try again next cycle

			oAddress.write(rAddress.read());
			oReadEnable.write(true);

			sNextState.write(STATE_READ_WORD_PENDING);
		}

		break;

	case STATE_READ_BYTE_PENDING:
		// Waiting for memory to read word - byte extraction needed

		if (iAcknowledge.read()) {
			// Memory access completed - data must be consumed and request signals cleared unless there is another request

			if (DEBUG)
				cout << "Read byte " << ((iData.read() >> rByteSelect.read()) & 0xFFUL) << " from memory " << id << ", address " << rAddress.read() << endl;

			if (iDataTxFree.read()) {
				// Sending back data is possible - do so immediately

				// Extract an individual byte and zero extend

				oDataTx.write(AddressedWord(Word((iData.read() >> rByteSelect.read()) & 0xFFUL), rRemoteChannel.read()));
				oDataTxEnable.write(true);

				// Check whether chaining another request without stall cycle is possible

				subProcessInitiateRequest(vRequest);
			} else {
				// Flow control does not permit sending back data - stall read operation (ports cleared implicitly)

				// Extract an individual byte from data read, zero extend and buffer it in controller until it can be sent back to client

				sDataBuffer.write((iData.read() >> rByteSelect.read()) & 0xFFUL);

				sNextState.write(STATE_READ_STALLED);
			}
		} else {
			// Memory access not yet completed - hold request signals and try again next cycle

			oAddress.write(rAddress.read());
			oReadEnable.write(true);

			sNextState.write(STATE_READ_BYTE_PENDING);
		}

		break;

	case STATE_READ_IPK_PENDING:
		// Waiting for memory to read word - IPK streaming in progress

		if (iAcknowledge.read()) {
			// Memory access completed - data must be consumed and request signals cleared unless there is another request

			if (DEBUG)
				cout << "Read instruction " << iData.read() << " from memory " << id << ", address " << rAddress.read() << endl;

			if (iDataTxFree.read()) {
				// Sending back data is possible - do so immediately

				if (static_cast<Instruction>(Word(iData.read())).endOfPacket()) {
					// This is the last instruction in the packet

					AddressedWord outWord(Word(iData.read()), rRemoteChannel.read());
					oDataTx.write(outWord);
					oDataTxEnable.write(true);

					// Check whether chaining another request without stall cycle is possible

					subProcessInitiateRequest(vRequest);
				} else {
					// There are more instructions in the packet

					AddressedWord outWord(Word(iData.read()), rRemoteChannel.read());
					outWord.notEndOfPacket();
					oDataTx.write(outWord);
					oDataTxEnable.write(true);

					// Load new memory address into register

					sAddress.write(rAddress.read() + 4);

					// Directly forward read command to memory bank to save a clock cycle

					oAddress.write(rAddress.read() + 4);
					oReadEnable.write(true);

					sNextState.write(STATE_READ_IPK_PENDING);
				}
			} else {
				// Flow control does not permit sending back data - stall read operation (ports cleared implicitly)

				// Buffer data read in controller until it can be sent back to client

				sDataBuffer.write(iData.read());

				sNextState.write(STATE_READ_IPK_STALLED);
			}
		} else {
			// Memory access not yet completed - hold request signals and try again next cycle

			oAddress.write(rAddress.read());
			oReadEnable.write(true);

			sNextState.write(STATE_READ_IPK_PENDING);
		}

		break;

	case STATE_READ_STALLED:
		// Memory access completed but flow control does not permit sending back the data

		if (iDataTxFree.read()) {
			// Sending back data is possible - do so

			oDataTx.write(AddressedWord(Word(rDataBuffer.read()), rRemoteChannel.read()));
			oDataTxEnable.write(true);

			// Check whether chaining another request without stall cycle is possible

			subProcessInitiateRequest(vRequest);
		} else {
			// Flow control does not permit sending back data - try again next cycle

			sNextState.write(STATE_READ_STALLED);
		}

		break;

	case STATE_READ_IPK_STALLED:
		// IPK related memory access completed but flow control does not permit sending back the data

		if (iDataTxFree.read()) {
			// Sending back data is possible - do so

			if (static_cast<Instruction>(Word(rDataBuffer.read())).endOfPacket()) {
				// This is the last instruction in the packet

				AddressedWord outWord(Word(rDataBuffer.read()), rRemoteChannel.read());
				oDataTx.write(outWord);
				oDataTxEnable.write(true);

				// Check whether chaining another request without stall cycle is possible

				subProcessInitiateRequest(vRequest);
			} else {
				// There are more instructions in the packet

				AddressedWord outWord(Word(rDataBuffer.read()), rRemoteChannel.read());
				outWord.notEndOfPacket();
				oDataTx.write(outWord);
				oDataTxEnable.write(true);

				// Load new memory address into register

				sAddress.write(rAddress.read() + 4);

				// Directly forward read command to memory bank to save a clock cycle

				oAddress.write(rAddress.read() + 4);
				oReadEnable.write(true);

				sNextState.write(STATE_READ_IPK_PENDING);
			}
		} else {
			// Flow control does not permit sending back data - try again next cycle

			sNextState.write(STATE_READ_IPK_STALLED);
		}

		break;

	case STATE_WRITE_DATA:
		// Waiting for data to write (word or byte)

		if (iDataRxAvailable.read()) {
			// Data is available - initiate memory access

			// The request always can proceed - flow control dependent stall cycles are inserted after carrying out the read operation

			// Remove request from input queue

			oDataRxAcknowledge.write(true);

			// Buffer data to write

			sDataBuffer.write((uint64_t)(iDataRx.read().toLong()));

			// Directly forward write command and data to memory bank to save a clock cycle

			oAddress.write(rAddress.read());
			oData.write((uint64_t)(iDataRx.read().toLong()));
			oByteMask.write(rByteMask.read());
			oWriteEnable.write(true);

			sNextState.write(STATE_WRITE_PENDING);
		} else {
			// Data not yet available - wait for data to arrive

			sNextState.write(STATE_WRITE_DATA);
		}

		break;

	case STATE_WRITE_PENDING:
		// Waiting for memory to write data (word or byte)

		if (iAcknowledge.read()) {
			// Memory access completed - request signals must be cleared unless there is another request (ports cleared implicitly)

			if (DEBUG)
				cout << "Wrote " << rDataBuffer.read() << " with byte mask " << rByteMask.read() << " to memory " << id << ", address " << rAddress.read() << endl;

			// Check whether chaining another request without stall cycle is possible

			subProcessInitiateRequest(vRequest);
		} else {
			// Memory access not yet completed - hold request signals and try again next cycle

			oAddress.write(rAddress.read());
			oData.write(rDataBuffer.read());
			oByteMask.write(rByteMask.read());
			oWriteEnable.write(true);

			sNextState.write(STATE_WRITE_PENDING);
		}

		break;
	}
}

// Checks whether a request is ready at the input ports and initiates it, otherwise it returns to connected idle state

void SharedL1CacheController::subProcessInitiateRequest(MemoryRequest &vRequest) {
	if (iDataRxAvailable.read()) {
		// New request data is available

		if (vRequest.isSetup()) {
			// Lazily tear down connections when the next setup request arrives

			if (iDataTxFree.read()) {
				// Flow control permits sending message to client (otherwise try again next clock cycle)

				if (DEBUG)
					cout << "set-up connection at channel " << (int)cChannel << endl;

				// Remove request from input queue

				oDataRxAcknowledge.write(true);

				// Load response address into register

				sRemoteChannel.write(vRequest.address());

				// Send response word to client

				oDataTx.write(AddressedWord(Word(id * NUM_MEMORY_OUTPUTS + cChannel), vRequest.address(), true));
				oDataTxEnable.write(true);
			}

			sNextState.write(STATE_CONNECTED_IDLE);
		} else if (vRequest.isReadRequest()) {
			// This is either a word or byte read request or an IPK read request

			if (DEBUG)
				cout << "read from address " << vRequest.address() << endl;

			// The request always can proceed - flow control dependent stall cycles are inserted after carrying out the read operation

			// Remove request from input queue

			oDataRxAcknowledge.write(true);

			// Load memory address into register

			sAddress.write(vRequest.address());

			// Directly forward read command to memory bank to save a clock cycle

			oAddress.write(vRequest.address());
			oReadEnable.write(true);

			// Set next state based on type of operation

			if (vRequest.isIPKRequest()) {
				if ((vRequest.address() & 0x3) != 0)
					cerr << "WARNING: Unaligned write access detected" << endl;

				Instrumentation::memoryRead(vRequest.address(), true);

				sNextState.write(STATE_READ_IPK_PENDING);
			} else if (false) {									//TODO: Fix condition as soon as the interface supports byte reads
				// Load byte selector into register

				sByteSelect.write(vRequest.address() & 0x3);

				Instrumentation::memoryRead(vRequest.address(), false);

				sNextState.write(STATE_READ_BYTE_PENDING);
			} else {
				if ((vRequest.address() & 0x3) != 0)
					cerr << "WARNING: Unaligned write access detected" << endl;

				Instrumentation::memoryRead(vRequest.address(), false);

				sNextState.write(STATE_READ_WORD_PENDING);
			}
		} else {
			// This is a write request header

			if (DEBUG)
				cout << "store to address " << vRequest.address() << endl;

			Instrumentation::memoryWrite(vRequest.address());

			// The request always can proceed

			// Remove request from input queue

			oDataRxAcknowledge.write(true);

			// Load memory address into register

			sAddress.write(vRequest.address());

			// Load byte mask into register

			if (false) {				//TODO: Fix condition and byte mask as soon as the interface supports byte reads
				sByteMask.write(1UL << (vRequest.address() & 0x3));
			} else {
				if ((vRequest.address() & 0x3) != 0)
					cerr << "WARNING: Unaligned write access detected" << endl;

				sByteMask.write(0xF);
			}

			sNextState.write(STATE_WRITE_DATA);
		}
	} else {
		// No new request available - return to idle state

		sNextState.write(STATE_CONNECTED_IDLE);
	}
}

// Idle signal steering

void SharedL1CacheController::processIdleSignal() {
	oIdle.write(rCurrentState.read() == STATE_IDLE || rCurrentState.read() == STATE_CONNECTED_IDLE);
}

//-------------------------------------------------------------------------------------------------
// Constructors / Destructors
//-------------------------------------------------------------------------------------------------

SharedL1CacheController::SharedL1CacheController(sc_module_name name, ComponentID id, uint channel) :
	Component(name, id)
{
	// Setup configuration

	cChannel = channel;

	// Initialise ports

	oDataRxAcknowledge.initialize(false);
	oDataTxEnable.initialize(false);
	oIdle.initialize(true);
	oReadEnable.initialize(false);
	oWriteEnable.initialize(false);

	// Initialise signals

	rCurrentState.write(STATE_IDLE);

	// Register processes

	SC_METHOD(processFSMRegisters);
	sensitive << iClock.neg();
	dont_initialize();

	SC_METHOD(processFSMCombinational);
	sensitive << iDataRx << iDataRxAvailable << iDataTxFree << iData << iAcknowledge;
	sensitive << rCurrentState << rRemoteChannel << rAddress << rByteMask << rDataBuffer << rByteSelect;
	dont_initialize();

	SC_METHOD(processIdleSignal);
	sensitive << rCurrentState;
	dont_initialize();

	// Indicate non-default component constructor

	end_module();
}

SharedL1CacheController::~SharedL1CacheController() {
	// Nothing
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods inherited from Component - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// The area of this component in square micrometres

double SharedL1CacheController::area() const {
	cerr << "Shared L1 cache area estimation not yet implemented" << endl;
	return 0.0;
}

// The energy consumed by this component in picojoules

double SharedL1CacheController::energy() const {
	cerr << "Shared L1 cache energy estimation not yet implemented" << endl;
	return 0.0;
}
