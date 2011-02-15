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
#include "../../Datatype/Instruction.h"
#include "../../Datatype/MemoryRequest.h"
#include "../../Datatype/Word.h"
#include "../../Utility/Instrumentation.h"
#include "../../Utility/Parameters.h"
#include "../../Utility/BatchMode/BatchModeEventRecorder.h"
#include "../TileComponent.h"
#include "SharedL1CacheController.h"

//-------------------------------------------------------------------------------------------------
// Simulation utility methods
//-------------------------------------------------------------------------------------------------

void SharedL1CacheController::debugOutputMessage(const char* message, long long arg1 = 0, long long arg2 = 0, long long arg3 = 0) {
	if (!DEBUG)
		return;

	cout << this->name();

	switch (rCurrentState.read()) {
	case STATE_IDLE:				cout << " [IDLE]: "; break;
	case STATE_CONNECTED_IDLE:		cout << " [CONNECTED_IDLE]: "; break;
	case STATE_READ_WORD_PENDING:	cout << " [READ_WORD_PENDING]: "; break;
	case STATE_READ_BYTE_PENDING:	cout << " [READ_BYTE_PENDING]: "; break;
	case STATE_READ_IPK_PENDING:	cout << " [READ_IPK_PENDING]: "; break;
	case STATE_READ_STALLED:		cout << " [READ_STALLED]: "; break;
	case STATE_READ_IPK_STALLED:	cout << " [READ_IPK_STALLED]: "; break;
	case STATE_WRITE_WORD_DATA:		cout << " [WRITE_WORD_DATA]: "; break;
	case STATE_WRITE_BYTE_DATA:		cout << " [WRITE_BYTE_DATA]: "; break;
	case STATE_WRITE_PENDING:		cout << " [WRITE_PENDING]: "; break;
	}

	char formatMessage[1024];
	sprintf(formatMessage, message, arg1, arg2, arg3);

	cout << formatMessage << endl;
}

//-------------------------------------------------------------------------------------------------
// Processes
//-------------------------------------------------------------------------------------------------

// FSM register logic sensitive to negative clock edge

void SharedL1CacheController::processFSMRegisters() {
	debugOutputMessage("Updating registers");

	if (vEventRecorder != NULL)
		vEventRecorder->commitInstanceEvents(this);

	rCurrentState.write(sNextState.read());
	rRemoteChannel.write(sRemoteChannel.read());
	rAddress.write(sAddress.read());
	rByteMask.write(sByteMask.read());
	rDataBuffer.write(sDataBuffer.read());
	rByteSelect.write(sByteSelect.read());
}

// Combinational FSM logic

void SharedL1CacheController::processFSMCombinational() {
	// Update instrumentation

	//Instrumentation::idle(id, rCurrentState.read() == STATE_IDLE || rCurrentState.read() == STATE_CONNECTED_IDLE);

	if (vEventRecorder != NULL)
		vEventRecorder->resetInstanceEvents(this);

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

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_IDLE);

		if (iDataRxAvailable.read() && iDataTxFree.read()) {
			// New request data is available and flow control permits sending message to client

			// Remove request from input queue

			oDataRxAcknowledge.write(true);

			// There is no connection set up at the moment - this must be a setup request, otherwise there is an error

			if (vRequest.isSetup()) {
				debugOutputMessage("Set-up connection at channel %lld", cChannel);

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventConnectionSetup);

				// Load response address into register

				sRemoteChannel.write(vRequest.address());

				// Send response word to client

				int portID = TileComponent::outputPortID(id, cChannel);
				oDataTx.write(AddressedWord(Word(portID), vRequest.address(), true));
				oDataTxEnable.write(true);

				sNextState.write(STATE_CONNECTED_IDLE);
			} else {
				cerr << "Error: unexpected input at " << this->name() << " channel " << (int)cChannel << ": " << vRequest << endl;

				sNextState.write(STATE_IDLE);
			}
		} else {
			// No new request available or sending data not permitted - stay idle

			if (vEventRecorder != NULL && iDataRxAvailable.read())
				vEventRecorder->recordInstanceEvent(this, kEventConnectionSetupStalled);

			sNextState.write(STATE_IDLE);
		}

		break;

	case STATE_CONNECTED_IDLE:
		// Connection established and idle

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_CONNECTED_IDLE);

		// Check whether there is an request ready and initiate it

		subProcessInitiateRequest(vRequest, false);

		break;

	case STATE_READ_WORD_PENDING:
		// Waiting for memory to read word

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_READ_WORD_PENDING);

		if (iAcknowledge.read()) {
			// Memory access completed - data must be consumed and request signals cleared unless there is another request

			if (iDataTxFree.read()) {
				debugOutputMessage("Pending word read acknowledged and forwarded - read 0x%.8llX from address %lld", iData.read(), rAddress.read());

				// Sending back data is possible - do so immediately

				oDataTx.write(AddressedWord(Word(iData.read()), rRemoteChannel.read()));
				oDataTxEnable.write(true);

				// Check whether chaining another request without stall cycle is possible

				subProcessInitiateRequest(vRequest, true);
			} else {
				debugOutputMessage("Pending word read acknowledged and stalled due to flow control - read 0x%.8llX from address %lld", iData.read(), rAddress.read());

				// Flow control does not permit sending back data - stall read operation (ports cleared implicitly)

				// Buffer data read in controller until it can be sent back to client

				sDataBuffer.write(iData.read());

				sNextState.write(STATE_READ_STALLED);
			}
		} else {
			debugOutputMessage("Pending word read in progress from address %lld", rAddress.read());

			// Memory access not yet completed - hold request signals and try again next cycle

			oAddress.write(rAddress.read());
			oReadEnable.write(true);

			sNextState.write(STATE_READ_WORD_PENDING);
		}

		break;

	case STATE_READ_BYTE_PENDING:
		// Waiting for memory to read word - byte extraction needed

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_READ_BYTE_PENDING);

		if (iAcknowledge.read()) {
			// Memory access completed - data must be consumed and request signals cleared unless there is another request

			if (iDataTxFree.read()) {
				debugOutputMessage("Pending byte read acknowledged and forwarded - read 0x%.2llX from address %lld", ((iData.read() >> (rByteSelect.read() * 8)) & 0xFFUL), rAddress.read());

				// Sending back data is possible - do so immediately

				// Extract an individual byte and zero extend

				oDataTx.write(AddressedWord(Word((iData.read() >> (rByteSelect.read() * 8)) & 0xFFUL), rRemoteChannel.read()));
				oDataTxEnable.write(true);

				// Check whether chaining another request without stall cycle is possible

				subProcessInitiateRequest(vRequest, true);
			} else {
				debugOutputMessage("Pending byte read acknowledged and stalled due to flow control - read 0x%.2llX from address %lld", ((iData.read() >> (rByteSelect.read() * 8)) & 0xFFUL), rAddress.read());

				// Flow control does not permit sending back data - stall read operation (ports cleared implicitly)

				// Extract an individual byte from data read, zero extend and buffer it in controller until it can be sent back to client

				sDataBuffer.write((iData.read() >> (rByteSelect.read() * 8)) & 0xFFUL);

				sNextState.write(STATE_READ_STALLED);
			}
		} else {
			debugOutputMessage("Pending byte read in progress from address %lld", rAddress.read());

			// Memory access not yet completed - hold request signals and try again next cycle

			oAddress.write(rAddress.read());
			oReadEnable.write(true);

			sNextState.write(STATE_READ_BYTE_PENDING);
		}

		break;

	case STATE_READ_IPK_PENDING:
		// Waiting for memory to read word - IPK streaming in progress

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_READ_IPK_PENDING);

		if (iAcknowledge.read()) {
			// Memory access completed - data must be consumed and request signals cleared unless there is another request

			if (iDataTxFree.read()) {
				// Sending back data is possible - do so immediately

				if (static_cast<Instruction>(Word(iData.read())).endOfPacket()) {
					debugOutputMessage("Pending instruction read acknowledged and forwarded - read 0x%.16llX from address %lld (last instruction)", iData.read(), rAddress.read());

					// This is the last instruction in the packet

					AddressedWord outWord(Word(iData.read()), rRemoteChannel.read());
					oDataTx.write(outWord);
					oDataTxEnable.write(true);

					// Check whether chaining another request without stall cycle is possible

					subProcessInitiateRequest(vRequest, true);
				} else {
					debugOutputMessage("Pending instruction read acknowledged and forwarded - read 0x%.16llX from address %lld (packet continues)", iData.read(), rAddress.read());

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
				debugOutputMessage("Pending instruction read acknowledged and stalled due to flow control - read 0x%.16llX from address %lld", iData.read(), rAddress.read());

				// Flow control does not permit sending back data - stall read operation (ports cleared implicitly)

				// Buffer data read in controller until it can be sent back to client

				sDataBuffer.write(iData.read());

				sNextState.write(STATE_READ_IPK_STALLED);
			}
		} else {
			debugOutputMessage("Pending instruction read in progress from address %lld", rAddress.read());

			// Memory access not yet completed - hold request signals and try again next cycle

			oAddress.write(rAddress.read());
			oReadEnable.write(true);

			sNextState.write(STATE_READ_IPK_PENDING);
		}

		break;

	case STATE_READ_STALLED:
		// Memory access completed but flow control does not permit sending back the data

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_READ_STALLED);

		if (iDataTxFree.read()) {
			debugOutputMessage("Stalled data read completed - sent 0x%.8llX", rDataBuffer.read());

			// Sending back data is possible - do so

			oDataTx.write(AddressedWord(Word(rDataBuffer.read()), rRemoteChannel.read()));
			oDataTxEnable.write(true);

			// Check whether chaining another request without stall cycle is possible

			subProcessInitiateRequest(vRequest, true);
		} else {
			debugOutputMessage("Stalled data read pending");

			// Flow control does not permit sending back data - try again next cycle

			sNextState.write(STATE_READ_STALLED);
		}

		break;

	case STATE_READ_IPK_STALLED:
		// IPK related memory access completed but flow control does not permit sending back the data

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_READ_IPK_STALLED);

		if (iDataTxFree.read()) {
			// Sending back data is possible - do so

			if (static_cast<Instruction>(Word(rDataBuffer.read())).endOfPacket()) {
				debugOutputMessage("Stalled instruction read completed - sent 0x%.16llX (last instruction)", rDataBuffer.read());

				// This is the last instruction in the packet

				AddressedWord outWord(Word(rDataBuffer.read()), rRemoteChannel.read());
				oDataTx.write(outWord);
				oDataTxEnable.write(true);

				// Check whether chaining another request without stall cycle is possible

				subProcessInitiateRequest(vRequest, true);
			} else {
				debugOutputMessage("Stalled instruction read completed - sent 0x%.16llX (packet continues)", rDataBuffer.read());

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
			debugOutputMessage("Stalled instruction read pending");

			// Flow control does not permit sending back data - try again next cycle

			sNextState.write(STATE_READ_IPK_STALLED);
		}

		break;

	case STATE_WRITE_WORD_DATA:
		// Waiting for data word to write

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_WRITE_WORD_DATA);

		if (iDataRxAvailable.read()) {
			debugOutputMessage("Data word to write available - writing 0x%.8llX (byte mask 0x%.1llX) to address %lld", (uint64_t)(iDataRx.read().toLong()), rByteMask.read(), rAddress.read());

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
			debugOutputMessage("Waiting for data word to write");

			// Data not yet available - wait for data to arrive

			sNextState.write(STATE_WRITE_WORD_DATA);
		}

		break;

	case STATE_WRITE_BYTE_DATA:
		// Waiting for data byte to write

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_WRITE_BYTE_DATA);

		if (iDataRxAvailable.read()) {
			debugOutputMessage("Data byte to write available - writing 0x%.8llX (byte mask 0x%.1llX) to address %lld", (uint64_t)(iDataRx.read().toLong()), rByteMask.read(), rAddress.read());

			// Data is available - initiate memory access

			// The request always can proceed - flow control dependent stall cycles are inserted after carrying out the read operation

			// Remove request from input queue

			oDataRxAcknowledge.write(true);

			// Buffer data to write

			sDataBuffer.write((uint64_t)(iDataRx.read().toLong()) << (rByteSelect.read() * 8));

			// Directly forward write command and data to memory bank to save a clock cycle

			oAddress.write(rAddress.read());
			oData.write((uint64_t)(iDataRx.read().toLong()) << (rByteSelect.read() * 8));
			oByteMask.write(rByteMask.read());
			oWriteEnable.write(true);

			sNextState.write(STATE_WRITE_PENDING);
		} else {
			debugOutputMessage("Waiting for data byte to write");

			// Data not yet available - wait for data to arrive

			sNextState.write(STATE_WRITE_BYTE_DATA);
		}

		break;

	case STATE_WRITE_PENDING:
		// Waiting for memory to write data (word or byte)

		if (vEventRecorder != NULL)
			vEventRecorder->recordInstanceEvent(this, kEvent_STATE_WRITE_PENDING);

		if (iAcknowledge.read()) {
			// Memory access completed - request signals must be cleared unless there is another request (ports cleared implicitly)

			debugOutputMessage("Write completed - wrote 0x%.8llX (byte mask 0x%.1llX) to address %lld", rDataBuffer.read(), rByteMask.read(), rAddress.read());

			// Check whether chaining another request without stall cycle is possible

			subProcessInitiateRequest(vRequest, true);
		} else {
			debugOutputMessage("Write in progress");

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

void SharedL1CacheController::subProcessInitiateRequest(MemoryRequest &vRequest, bool chained) {
	if (iDataRxAvailable.read()) {
		// New request data is available

		if (vRequest.isSetup()) {
			// Lazily tear down connections when the next setup request arrives

			if (iDataTxFree.read()) {
				// Flow control permits sending message to client (otherwise try again next clock cycle)

				debugOutputMessage("Set-up replacement connection at channel %lld", cChannel);

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventConnectionSetup + (chained ? kEventChainedOffset : 0));

				// Remove request from input queue

				oDataRxAcknowledge.write(true);

				// Load response address into register

				sRemoteChannel.write(vRequest.address());

				// Send response word to client

				int portID = TileComponent::outputPortID(id, cChannel);
				oDataTx.write(AddressedWord(Word(portID), vRequest.address(), true));
				oDataTxEnable.write(true);
			} else {
				debugOutputMessage("Set-up of replacement connection at channel %lld delayed by flow control", cChannel);

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventConnectionSetupStalled + (chained ? kEventChainedOffset : 0));
			}

			sNextState.write(STATE_CONNECTED_IDLE);
		} else if (vRequest.isReadRequest()) {
			// This is either a word or byte read request or an IPK read request

			// The request always can proceed - flow control dependent stall cycles are inserted after carrying out the read operation

			// Remove request from input queue

			oDataRxAcknowledge.write(true);

			// Set next state based on type of operation

			if (vRequest.isIPKRequest()) {
				debugOutputMessage("IPK load from address %lld", vRequest.address());

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventReadIPKStart + (chained ? kEventChainedOffset : 0));

				if ((vRequest.address() & 0x3) != 0)
					cerr << "WARNING: Unaligned write access detected" << endl;

				// Load memory address into register

				sAddress.write(vRequest.address());

				// Directly forward read command to memory bank to save a clock cycle

				oAddress.write(vRequest.address());
				oReadEnable.write(true);

				Instrumentation::l1Read(vRequest.address(), true);

				sNextState.write(STATE_READ_IPK_PENDING);
			} else if (vRequest.byteAccess()) {
				debugOutputMessage("Byte load from address %lld", vRequest.address());

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventReadByteStart + (chained ? kEventChainedOffset : 0));

				// Load memory address into register

				sAddress.write(vRequest.address() & 0xFFFFFFFC);

				// Directly forward read command to memory bank to save a clock cycle

				oAddress.write(vRequest.address() & 0xFFFFFFFC);
				oReadEnable.write(true);

				// Load byte selector into register

				sByteSelect.write(vRequest.address() & 0x3);

				Instrumentation::l1Read(vRequest.address(), false);

				sNextState.write(STATE_READ_BYTE_PENDING);
			} else {
				debugOutputMessage("Word load from address %lld", vRequest.address());

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventReadWordStart + (chained ? kEventChainedOffset : 0));

				if ((vRequest.address() & 0x3) != 0)
					cerr << "WARNING: Unaligned write access detected" << endl;

				// Load memory address into register

				sAddress.write(vRequest.address());

				// Directly forward read command to memory bank to save a clock cycle

				oAddress.write(vRequest.address());
				oReadEnable.write(true);

				Instrumentation::l1Read(vRequest.address(), false);

				sNextState.write(STATE_READ_WORD_PENDING);
			}
		} else {
			// This is a write request header

			Instrumentation::l1Write(vRequest.address());

			// The request always can proceed - remove request from input queue

			oDataRxAcknowledge.write(true);

			// Load memory address and byte mask into registers

			if (vRequest.byteAccess()) {
				debugOutputMessage("Byte store to address %lld", vRequest.address());

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventWriteByteStart + (chained ? kEventChainedOffset : 0));

				sAddress.write(vRequest.address() & 0xFFFFFFFC);
				sByteMask.write(1UL << (vRequest.address() & 0x3));

				// Load byte selector into register

				sByteSelect.write(vRequest.address() & 0x3);

				sNextState.write(STATE_WRITE_BYTE_DATA);
			} else {
				debugOutputMessage("Word store to address %lld", vRequest.address());

				if (vEventRecorder != NULL)
					vEventRecorder->recordInstanceEvent(this, kEventWriteWordStart + (chained ? kEventChainedOffset : 0));

				if ((vRequest.address() & 0x3) != 0)
					cerr << "WARNING: Unaligned write access detected" << endl;

				sAddress.write(vRequest.address());
				sByteMask.write(0xF);

				sNextState.write(STATE_WRITE_WORD_DATA);
			}
		}
	} else {
		// No new request available - return to idle state

		debugOutputMessage("No pending request available - returning to idle state");

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

SharedL1CacheController::SharedL1CacheController(sc_module_name name, ComponentID id, BatchModeEventRecorder *eventRecorder, uint channel) :
	Component(name, id)
{
	// Instrumentation

	vEventRecorder = eventRecorder;

	if (vEventRecorder != NULL) {
		vEventRecorder->registerInstance(this, BatchModeEventRecorder::kInstanceSharedL1CacheController);
		vEventRecorder->setInstanceProperty(this, kPropertyChannelNumber, channel);
	}

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
