//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Subsystem Wrapper Implementation
//-------------------------------------------------------------------------------------------------
// Implements a wrapper containing all modules comprising the Shared L1 Cache Subsystem.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheSubsystem.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 28/01/2011
//-------------------------------------------------------------------------------------------------

#include "../../Utility/Parameters.h"
#include "../../Utility/BatchMode/BatchModeEventRecorder.h"
#include "../TileComponent.h"
#include "SharedL1CacheBank.h"
#include "SharedL1CacheCrossbarSwitch.h"
#include "SharedL1CacheController.h"
#include "SharedL1CacheNetworkInterface.h"
#include "SimplifiedBackgroundMemory.h"
#include "SharedL1CacheSubsystem.h"

//-------------------------------------------------------------------------------------------------
// Processes
//-------------------------------------------------------------------------------------------------

void SharedL1CacheSubsystem::processUpdateIdle() {
	bool product = true;

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++) {
		product &= sNetworkInterfaceIdle[i].read();
		product &= sControllerIdle[i].read();
	}

	idle.write(product);

	Instrumentation::idle(id, product);
}

//-------------------------------------------------------------------------------------------------
// Constructors / Destructors
//-------------------------------------------------------------------------------------------------

SharedL1CacheSubsystem::SharedL1CacheSubsystem(sc_module_name name, ComponentID id, BatchModeEventRecorder *eventRecorder) :
	TileComponent(name, id),
	mCrossbarSwitch("shared_l1_cache_crossbar_switch", id, eventRecorder, SHARED_L1_CACHE_CHANNELS, SHARED_L1_CACHE_BANKS, SHARED_L1_CACHE_LINE_SIZE),
	mBackgroundMemory("shared_l1_cache_background_memory", id, eventRecorder, SHARED_L1_CACHE_BANKS, SHARED_L1_CACHE_MEMORY_QUEUE_DEPTH, SHARED_L1_CACHE_MEMORY_DELAY_CYCLES)
{
	// Instrumentation

	vEventRecorder = eventRecorder;

	if (vEventRecorder != NULL)
		vEventRecorder->registerInstance(this, BatchModeEventRecorder::kInstanceSharedL1CacheSubsystem);

	// Initialise top-level ports of cache subsystem

	flowControlOut = new sc_out<int>[SHARED_L1_CACHE_CHANNELS];
	in             = new sc_in<Word>[SHARED_L1_CACHE_CHANNELS];

	flowControlIn  = new sc_in<bool>[SHARED_L1_CACHE_CHANNELS];
	out            = new sc_out<AddressedWord>[SHARED_L1_CACHE_CHANNELS];

	// Construct network interfaces

	mNetworkInterfaces = new SharedL1CacheNetworkInterface*[SHARED_L1_CACHE_CHANNELS];

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++)
		mNetworkInterfaces[i] = new SharedL1CacheNetworkInterface("shared_l1_cache_network_interface", id, eventRecorder, (uint)i, SHARED_L1_CACHE_INTERFACE_QUEUE_DEPTH);

	// Construct cache controllers

	mControllers = new SharedL1CacheController*[SHARED_L1_CACHE_CHANNELS];

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++)
		mControllers[i] = new SharedL1CacheController("shared_l1_cache_controller", id, eventRecorder, (uint)i);

	// Construct memory banks

	mMemoryBanks = new SharedL1CacheBank*[SHARED_L1_CACHE_BANKS];

	for (uint i = 0; i < SHARED_L1_CACHE_BANKS; i++)
		mMemoryBanks[i] = new SharedL1CacheBank("shared_l1_cache_bank", id, eventRecorder, (uint)i, SHARED_L1_CACHE_BANKS, SHARED_L1_CACHE_SETS_PER_BANK, SHARED_L1_CACHE_ASSOCIATIVITY, SHARED_L1_CACHE_LINE_SIZE, SHARED_L1_CACHE_SEQUENTIAL_SEARCH != 0, SHARED_L1_CACHE_RANDOM_REPLACEMENT != 0);

	// Initialise signals

	sNetworkInterfaceIdle = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];
	sControllerIdle = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];

	// Connections between network interfaces and controllers

	sN2CDataRx = new sc_signal<Word>[SHARED_L1_CACHE_CHANNELS];
	sN2CDataRxAvailable = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];
	sC2NDataRxAcknowledge = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];

	sC2NDataTx = new sc_signal<AddressedWord>[SHARED_L1_CACHE_CHANNELS];
	sC2NDataTxEnable = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];
	sN2CDataTxFree = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];

	// Connections between controllers and crossbar switch

	sC2SAddress = new sc_signal<uint32_t>[SHARED_L1_CACHE_CHANNELS];
	sC2SData = new sc_signal<uint64_t>[SHARED_L1_CACHE_CHANNELS];
	sC2SByteMask = new sc_signal<uint8_t>[SHARED_L1_CACHE_CHANNELS];
	sC2SReadEnable = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];
	sC2SWriteEnable = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];

	sS2CData = new sc_signal<uint64_t>[SHARED_L1_CACHE_CHANNELS];
	sS2CAcknowledge = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];

	// Connections between crossbar switch and memory banks

	sS2MAddress = new sc_signal<uint32_t>[SHARED_L1_CACHE_BANKS];
	sS2MWriteData = new sc_signal<uint64_t>[SHARED_L1_CACHE_BANKS];
	sS2MByteMask = new sc_signal<uint8_t>[SHARED_L1_CACHE_BANKS];
	sS2MReadEnable = new sc_signal<bool>[SHARED_L1_CACHE_BANKS];
	sS2MWriteEnable = new sc_signal<bool>[SHARED_L1_CACHE_BANKS];

	sM2SReadData = new sc_signal<uint64_t>[SHARED_L1_CACHE_BANKS];
	sM2SAcknowledge = new sc_signal<bool>[SHARED_L1_CACHE_BANKS];

	// Connections between memory banks and background memory

	sM2BAddress = new sc_signal<uint32_t>[SHARED_L1_CACHE_BANKS];
	sM2BWriteData = new sc_signal<uint64_t>[SHARED_L1_CACHE_BANKS];
	sM2BReadEnable = new sc_signal<bool>[SHARED_L1_CACHE_BANKS];
	sM2BWriteEnable = new sc_signal<bool>[SHARED_L1_CACHE_BANKS];

	sB2MReadData = new sc_signal<uint64_t>[SHARED_L1_CACHE_BANKS];
	sB2MAcknowledge = new sc_signal<bool>[SHARED_L1_CACHE_BANKS];

	// Network interface and cache controller port connections

	// Crossbar switch frontend

	mCrossbarSwitch.iClock.bind(clock);

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++) {
		// Initialize idle signal collectors

		sNetworkInterfaceIdle[i].write(true);
		sControllerIdle[i].write(true);

		// Network interface front end

		mNetworkInterfaces[i]->iClock.bind(clock);

		mNetworkInterfaces[i]->iDataRx.bind(in[i]);
		mNetworkInterfaces[i]->oFlowControlRx.bind(flowControlOut[i]);

		mNetworkInterfaces[i]->oDataTx.bind(out[i]);
		mNetworkInterfaces[i]->iFlowControlTx.bind(flowControlIn[i]);

		mNetworkInterfaces[i]->oIdle.bind(sNetworkInterfaceIdle[i]);

		// Controller front end

		mControllers[i]->iClock.bind(clock);
		mControllers[i]->oIdle.bind(sControllerIdle[i]);

		// Connections between network interfaces and controllers

		mNetworkInterfaces[i]->oDataRx.bind(sN2CDataRx[i]);						mControllers[i]->iDataRx.bind(sN2CDataRx[i]);
		mNetworkInterfaces[i]->oDataRxAvailable.bind(sN2CDataRxAvailable[i]);	mControllers[i]->iDataRxAvailable.bind(sN2CDataRxAvailable[i]);
		mControllers[i]->oDataRxAcknowledge.bind(sC2NDataRxAcknowledge[i]);		mNetworkInterfaces[i]->iDataRxAcknowledge.bind(sC2NDataRxAcknowledge[i]);

		mControllers[i]->oDataTx.bind(sC2NDataTx[i]);							mNetworkInterfaces[i]->iDataTx.bind(sC2NDataTx[i]);
		mControllers[i]->oDataTxEnable.bind(sC2NDataTxEnable[i]);				mNetworkInterfaces[i]->iDataTxEnable.bind(sC2NDataTxEnable[i]);
		mNetworkInterfaces[i]->oDataTxFree.bind(sN2CDataTxFree[i]);				mControllers[i]->iDataTxFree.bind(sN2CDataTxFree[i]);

		// Connections between controllers and crossbar switch

		mControllers[i]->oAddress.bind(sC2SAddress[i]);							mCrossbarSwitch.iAddress[i].bind(sC2SAddress[i]);
		mControllers[i]->oData.bind(sC2SData[i]);								mCrossbarSwitch.iWriteData[i].bind(sC2SData[i]);
		mControllers[i]->oByteMask.bind(sC2SByteMask[i]);						mCrossbarSwitch.iByteMask[i].bind(sC2SByteMask[i]);
		mControllers[i]->oReadEnable.bind(sC2SReadEnable[i]);					mCrossbarSwitch.iReadEnable[i].bind(sC2SReadEnable[i]);
		mControllers[i]->oWriteEnable.bind(sC2SWriteEnable[i]);					mCrossbarSwitch.iWriteEnable[i].bind(sC2SWriteEnable[i]);

		mCrossbarSwitch.oReadData[i].bind(sS2CData[i]);							mControllers[i]->iData.bind(sS2CData[i]);
		mCrossbarSwitch.oAcknowledge[i].bind(sS2CAcknowledge[i]);				mControllers[i]->iAcknowledge.bind(sS2CAcknowledge[i]);
	}

	// Memory bank and background memory port connections

	// Background memory front end

	mBackgroundMemory.iClock.bind(clock);

	for (uint i = 0; i < SHARED_L1_CACHE_BANKS; i++) {
		// Memory bank front end

		mMemoryBanks[i]->iClock.bind(clock);

		// Connections between crossbar switch and memory banks

		mCrossbarSwitch.oAddress[i].bind(sS2MAddress[i]);						mMemoryBanks[i]->iAddress.bind(sS2MAddress[i]);
		mCrossbarSwitch.oWriteData[i].bind(sS2MWriteData[i]);					mMemoryBanks[i]->iWriteData.bind(sS2MWriteData[i]);
		mCrossbarSwitch.oByteMask[i].bind(sS2MByteMask[i]);						mMemoryBanks[i]->iByteMask.bind(sS2MByteMask[i]);
		mCrossbarSwitch.oReadEnable[i].bind(sS2MReadEnable[i]);					mMemoryBanks[i]->iReadEnable.bind(sS2MReadEnable[i]);
		mCrossbarSwitch.oWriteEnable[i].bind(sS2MWriteEnable[i]);				mMemoryBanks[i]->iWriteEnable.bind(sS2MWriteEnable[i]);

		mMemoryBanks[i]->oReadData.bind(sM2SReadData[i]);						mCrossbarSwitch.iReadData[i].bind(sM2SReadData[i]);
		mMemoryBanks[i]->oAcknowledge.bind(sM2SAcknowledge[i]);					mCrossbarSwitch.iAcknowledge[i].bind(sM2SAcknowledge[i]);

		// Connections between memory banks and background memory

		mMemoryBanks[i]->oAddress.bind(sM2BAddress[i]);							mBackgroundMemory.iAddress[i].bind(sM2BAddress[i]);
		mMemoryBanks[i]->oWriteData.bind(sM2BWriteData[i]);						mBackgroundMemory.iWriteData[i].bind(sM2BWriteData[i]);
		mMemoryBanks[i]->oReadEnable.bind(sM2BReadEnable[i]);					mBackgroundMemory.iReadEnable[i].bind(sM2BReadEnable[i]);
		mMemoryBanks[i]->oWriteEnable.bind(sM2BWriteEnable[i]);					mBackgroundMemory.iWriteEnable[i].bind(sM2BWriteEnable[i]);

		mBackgroundMemory.oReadData[i].bind(sB2MReadData[i]);					mMemoryBanks[i]->iReadData.bind(sB2MReadData[i]);
		mBackgroundMemory.oAcknowledge[i].bind(sB2MAcknowledge[i]);				mMemoryBanks[i]->iAcknowledge.bind(sB2MAcknowledge[i]);
	}

	// Register processes

	SC_METHOD(processUpdateIdle);
	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++)
		sensitive << sNetworkInterfaceIdle[i] << sControllerIdle[i];
	dont_initialize();

	// Indicate non-default component constructor

	end_module();
}

SharedL1CacheSubsystem::~SharedL1CacheSubsystem() {
	// Destruct network interfaces

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++)
		delete mNetworkInterfaces[i];

	delete[] mNetworkInterfaces;

	// Destruct cache controllers

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++)
		delete mControllers[i];

	delete[] mControllers;

	// Destruct memory banks

	for (uint i = 0; i < SHARED_L1_CACHE_BANKS; i++)
		delete mMemoryBanks[i];

	delete[] mMemoryBanks;

	// Destruct signals

	delete[] sNetworkInterfaceIdle;
	delete[] sControllerIdle;

	// Connections between network interfaces and controllers

	delete[] sN2CDataRx;
	delete[] sN2CDataRxAvailable;
	delete[] sC2NDataRxAcknowledge;

	delete[] sC2NDataTx;
	delete[] sC2NDataTxEnable;
	delete[] sN2CDataTxFree;

	// Connections between controllers and crossbar switch

	delete[] sC2SAddress;
	delete[] sC2SData;
	delete[] sC2SByteMask;
	delete[] sC2SReadEnable;
	delete[] sC2SWriteEnable;

	delete[] sS2CData;
	delete[] sS2CAcknowledge;

	// Connections between crossbar switch and memory banks

	delete[] sS2MAddress;
	delete[] sS2MWriteData;
	delete[] sS2MByteMask;
	delete[] sS2MReadEnable;
	delete[] sS2MWriteEnable;

	delete[] sM2SReadData;
	delete[] sM2SAcknowledge;

	// Connections between memory banks and background memory

	delete[] sM2BAddress;
	delete[] sM2BWriteData;
	delete[] sM2BReadEnable;
	delete[] sM2BWriteEnable;

	delete[] sB2MReadData;
	delete[] sB2MAcknowledge;
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods inherited from TileComponent - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// Initialise the contents of this memory to the Words in the given vector

void SharedL1CacheSubsystem::storeData(const std::vector<Word>& data, MemoryAddr location = 0) {
	size_t count = data.size();
	uint64_t *plainWords = new uint64_t[count];
	const Word *inputWords = data.data();

	for (size_t i = 0; i < count; i++)
		plainWords[i] = (uint64_t)inputWords[i].toLong();

	mBackgroundMemory.setWords(location, plainWords, count);

	delete[] plainWords;
}

// Print the contents of this memory

void SharedL1CacheSubsystem::print(MemoryAddr start = 0, MemoryAddr end = MEMORY_SIZE) const {
	for (uint32_t address = start; address < end; address += 4) {
		uint64_t data;
		bool wordCached = false;

		for (uint i = 0; i < SHARED_L1_CACHE_BANKS; i++) {
			if (mMemoryBanks[i]->getWord(address, data)) {
				wordCached = true;
				break;
			}
		}

		if (!wordCached)
			mBackgroundMemory.getWords(address, &data, 1);

		printf("%.8X:  0x%.8llX\n", address, (unsigned long long)data);
	}
}

// Return the value at the given address

const Word SharedL1CacheSubsystem::getMemVal(MemoryAddr addr) const {
	uint32_t address = addr;
	uint64_t data;

	for (uint i = 0; i < SHARED_L1_CACHE_BANKS; i++)
		if (mMemoryBanks[i]->getWord(address, data))
			return Word(data);

	mBackgroundMemory.getWords(address, &data, 1);
	return Word(data);
}

//-------------------------------------------------------------------------------------------------
// Simulation utility methods inherited from Component - not part of simulated logic
//-------------------------------------------------------------------------------------------------

// The area of this component in square micrometres

double SharedL1CacheSubsystem::area() const {
	cerr << "Shared L1 cache area estimation not yet implemented" << endl;
	return 0.0;
}

// The energy consumed by this component in picojoules

double SharedL1CacheSubsystem::energy() const {
	cerr << "Shared L1 cache energy estimation not yet implemented" << endl;
	return 0.0;
}
