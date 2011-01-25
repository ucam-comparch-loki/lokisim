//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Crossbar Switch Implementation
//-------------------------------------------------------------------------------------------------
// Implements a fully combinational crossbar switch connecting the cache controllers with the
// memory banks.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheCrossbarSwitch.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 12/01/2011
//-------------------------------------------------------------------------------------------------

#include "../../Component.h"
#include "SharedL1CacheCrossbarSwitch.h"

//---------------------------------------------------------------------------------------------
// Event handlers / Processes
//---------------------------------------------------------------------------------------------

// Check all input ports for new data and update output ports

void SharedL1CacheCrossbarSwitch::processInputChanged() {
	for (uint channel = 0; channel < cChannels; channel++) {
		oReadData[channel].write(0);			// Should really be don't care - needed to break dependency chains
		oAcknowledge[channel].write(false);
	}

	for (uint bank = 0; bank < cMemoryBanks; bank++) {
		oAddress[bank].write(0);				// Should really be don't care - needed to break dependency chains
		oWriteData[bank].write(0);				// Should really be don't care - needed to break dependency chains
		oByteMask[bank].write(0);				// Should really be don't care - needed to break dependency chains
		oReadEnable[bank].write(false);
		oWriteEnable[bank].write(false);

		for (uint channel = 0; channel < cChannels; channel++) {
			// Extract number of memory bank

			uint bankAddress = (iAddress[channel].read() >> cCacheLineBits) & ((1UL << cBankSelectionBits) - 1);

			// Route signals

			if ((iReadEnable[channel].read() || iWriteEnable[channel].read()) && bankAddress == bank) {
				// Forward routing

				oAddress[bank].write(iAddress[channel].read());
				oWriteData[bank].write(iWriteData[channel].read());
				oByteMask[bank].write(iByteMask[channel].read());
				oReadEnable[bank].write(iReadEnable[channel].read());
				oWriteEnable[bank].write(iWriteEnable[channel].read());

				// Backward routing

				oReadData[channel].write(iReadData[bank].read());
				oAcknowledge[channel].write(iAcknowledge[bank].read());

				break;
			}
		}
	}
}

//---------------------------------------------------------------------------------------------
// Constructors / Destructors
//---------------------------------------------------------------------------------------------

SharedL1CacheCrossbarSwitch::SharedL1CacheCrossbarSwitch(sc_module_name name, ComponentID id, uint channels, uint memoryBanks, uint cacheLineSize) :
	Component(name, id)
{
	// Setup configuration parameters and dependent structures

	cChannels = channels;

	if (channels == 0) {
		cerr << "Shared L1 cache crossbar switch instantiated with no channels" << endl;
		throw std::exception();
	}

	uint temp = memoryBanks >> 1;
	uint bitCount = 0;
	while (temp != 0) {
		temp >>= 1;
		bitCount++;
	}

	cMemoryBanks = memoryBanks;
	cBankSelectionBits = bitCount;

	if (memoryBanks != 1UL << cBankSelectionBits) {
		cerr << "Shared L1 cache crossbar switch instantiated with memory bank count not being a power of two" << endl;
		throw std::exception();
	}

	temp = cacheLineSize >> 1;
	bitCount = 0;
	while (temp != 0) {
		temp >>= 1;
		bitCount++;
	}

	cCacheLineBits = bitCount;

	if (cacheLineSize != 1UL << cCacheLineBits) {
		cerr << "Shared L1 cache crossbar switch instantiated with cache line size not being a power of two" << endl;
		throw std::exception();
	}

	// Initialise network interface ports

	iAddress = new sc_in<uint32_t>[cChannels];
	iWriteData = new sc_in<uint32_t>[cChannels];
	iByteMask = new sc_in<uint8_t>[cChannels];
	iReadEnable = new sc_in<bool>[cChannels];
	iWriteEnable = new sc_in<bool>[cChannels];

	oReadData = new sc_out<uint32_t>[cChannels];
	oAcknowledge = new sc_out<bool>[cChannels];

	for (uint i = 0; i < channels; i++)
		oAcknowledge[i].initialize(false);

	// Initialise memory bank ports

	oAddress = new sc_out<uint32_t>[cMemoryBanks];
	oWriteData = new sc_out<uint32_t>[cMemoryBanks];
	oByteMask = new sc_out<uint8_t>[cMemoryBanks];
	oReadEnable = new sc_out<bool>[cMemoryBanks];
	oWriteEnable = new sc_out<bool>[cMemoryBanks];

	iReadData = new sc_in<uint32_t>[cMemoryBanks];
	iAcknowledge = new sc_in<bool>[cMemoryBanks];

	for (uint i = 0; i < cMemoryBanks; i++) {
		oReadEnable[i].initialize(false);
		oWriteEnable[i].initialize(false);
	}

	// Register processes

	SC_METHOD(processInputChanged);
	for (uint i = 0; i < channels; i++)
		sensitive << iAddress[i] << iWriteData[i] << iByteMask[i] << iReadEnable[i] << iWriteEnable[i] << iReadData[i] << iAcknowledge[i];
	dont_initialize();

	// Indicate non-default component constructor

	end_module();
}

SharedL1CacheCrossbarSwitch::~SharedL1CacheCrossbarSwitch() {
	delete[] iAddress;
	delete[] iWriteData;
	delete[] iByteMask;
	delete[] iReadEnable;
	delete[] iWriteEnable;

	delete[] oReadData;
	delete[] oAcknowledge;

	delete[] oAddress;
	delete[] oWriteData;
	delete[] oByteMask;
	delete[] oReadEnable;
	delete[] oWriteEnable;

	delete[] iReadData;
	delete[] iAcknowledge;
}

//---------------------------------------------------------------------------------------------
// Simulation utility methods inherited from Component - not part of simulated logic
//---------------------------------------------------------------------------------------------

// The area of this component in square micrometres

double SharedL1CacheCrossbarSwitch::area() const {
	cerr << "Shared L1 cache area estimation not yet implemented" << endl;
	return 0.0;
}

// The energy consumed by this component in picojoules

double SharedL1CacheCrossbarSwitch::energy() const {
	cerr << "Shared L1 cache energy estimation not yet implemented" << endl;
	return 0.0;
}
