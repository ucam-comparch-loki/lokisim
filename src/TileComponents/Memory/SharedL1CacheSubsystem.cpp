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
	bool sum = false;

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++) {
		sum |= sNetworkInterfaceIdle[i].read();
		sum |= sControllerIdle[i].read();
	}

	idle.write(sum);
}

//-------------------------------------------------------------------------------------------------
// Constructors / Destructors
//-------------------------------------------------------------------------------------------------

SharedL1CacheSubsystem::SharedL1CacheSubsystem(sc_module_name name, ComponentID id) :
	TileComponent(name, id),
	mCrossbarSwitch("shared_l1_cache_crossbar_switch", id, SHARED_L1_CACHE_CHANNELS, SHARED_L1_CACHE_BANKS, SHARED_L1_CACHE_LINE_SIZE),
	mBackgroundMemory("shared_l1_cache_background_memory", id, SHARED_L1_CACHE_BANKS, SHARED_L1_CACHE_MEMORY_QUEUE_DEPTH, SHARED_L1_CACHE_MEMORY_DELAY_CYCLES)
{
	// Construct network interfaces

	mNetworkInterfaces = new SharedL1CacheNetworkInterface*[SHARED_L1_CACHE_CHANNELS];

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++)
		mNetworkInterfaces[i] = new SharedL1CacheNetworkInterface("shared_l1_cache_network_interface", id, SHARED_L1_CACHE_INTERFACE_QUEUE_DEPTH);

	// Construct cache controllers

	mControllers = new SharedL1CacheController*[SHARED_L1_CACHE_CHANNELS];

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++)
		mControllers[i] = new SharedL1CacheController("shared_l1_cache_controller", id, (uint)i);

	// Construct memory banks

	mMemoryBanks = new SharedL1CacheBank*[SHARED_L1_CACHE_BANKS];

	for (uint i = 0; i < SHARED_L1_CACHE_BANKS; i++)
		mMemoryBanks[i] = new SharedL1CacheBank("shared_l1_cache_bank", id, (uint)i, SHARED_L1_CACHE_BANKS, SHARED_L1_CACHE_SETS_PER_BANK, SHARED_L1_CACHE_ASSOCIATIVITY, SHARED_L1_CACHE_LINE_SIZE, SHARED_L1_CACHE_SEQUENTIAL_SEARCH != 0, SHARED_L1_CACHE_RANDOM_REPLACEMENT != 0);

	// Initialise signals

	sNetworkInterfaceIdle = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];
	sControllerIdle = new sc_signal<bool>[SHARED_L1_CACHE_CHANNELS];

	// Network interface and cache controller port connections

	for (uint i = 0; i < SHARED_L1_CACHE_CHANNELS; i++) {
		sNetworkInterfaceIdle[i].write(true);
		sControllerIdle[i].write(true);

		mNetworkInterfaces[i]->iClock.bind(clock);

		mNetworkInterfaces[i]->iDataRx.bind(in[i]);
		mNetworkInterfaces[i]->oFlowControlRx.bind(flowControlOut[i]);

		mNetworkInterfaces[i]->oDataTx.bind(out[i]);
		mNetworkInterfaces[i]->iFlowControlTx.bind(flowControlIn[i]);

		mNetworkInterfaces[i]->oIdle.bind(sNetworkInterfaceIdle[i]);

		mControllers[i]->iClock.bind(clock);

		mControllers[i]->iDataRx.bind(mNetworkInterfaces[i]->oDataRx);
		mControllers[i]->iDataRxAvailable.bind(mNetworkInterfaces[i]->oDataRxAvailable);
		mNetworkInterfaces[i]->iDataRxAcknowledge.bind(mControllers[i]->oDataRxAcknowledge);

		mNetworkInterfaces[i]->iDataTx.bind(mControllers[i]->oDataTx);
		mNetworkInterfaces[i]->iDataTxEnable.bind(mControllers[i]->oDataTxEnable);
		mControllers[i]->iDataTxFree.bind(mNetworkInterfaces[i]->oDataTxFree);

		mControllers[i]->oIdle.bind(sControllerIdle[i]);

		mCrossbarSwitch.iAddress[i].bind(mControllers[i]->oAddress);
		mCrossbarSwitch.iWriteData[i].bind(mControllers[i]->oData);
		mCrossbarSwitch.iByteMask[i].bind(mControllers[i]->oByteMask);
		mCrossbarSwitch.iReadEnable[i].bind(mControllers[i]->oReadEnable);
		mCrossbarSwitch.iWriteEnable[i].bind(mControllers[i]->oWriteEnable);

		mControllers[i]->iData.bind(mCrossbarSwitch.oReadData[i]);
		mControllers[i]->iAcknowledge.bind(mCrossbarSwitch.oAcknowledge[i]);
	}

	// Memory bank and background memory port connections

	mBackgroundMemory.iClock.bind(clock);

	for (uint i = 0; i < SHARED_L1_CACHE_BANKS; i++) {
		mMemoryBanks[i]->iClock.bind(clock);

		mMemoryBanks[i]->iAddress.bind(mCrossbarSwitch.oAddress[i]);
		mMemoryBanks[i]->iWriteData.bind(mCrossbarSwitch.oWriteData[i]);
		mMemoryBanks[i]->iByteMask.bind(mCrossbarSwitch.oByteMask[i]);
		mMemoryBanks[i]->iReadEnable.bind(mCrossbarSwitch.oReadEnable[i]);
		mMemoryBanks[i]->iWriteEnable.bind(mCrossbarSwitch.oWriteEnable[i]);

		mCrossbarSwitch.iReadData[i].bind(mMemoryBanks[i]->oReadData);
		mCrossbarSwitch.iAcknowledge[i].bind(mMemoryBanks[i]->oAcknowledge);

		mBackgroundMemory.iAddress[i].bind(mMemoryBanks[i]->oAddress);
		mBackgroundMemory.iWriteData[i].bind(mMemoryBanks[i]->oWriteData);
		mBackgroundMemory.iReadEnable[i].bind(mMemoryBanks[i]->oReadEnable);
		mBackgroundMemory.iWriteEnable[i].bind(mMemoryBanks[i]->oWriteEnable);

		mMemoryBanks[i]->iReadData.bind(mBackgroundMemory.oReadData[i]);
		mMemoryBanks[i]->iAcknowledge.bind(mBackgroundMemory.oAcknowledge[i]);

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
