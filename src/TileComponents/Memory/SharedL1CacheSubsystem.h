//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Shared L1 Cache Subsystem Wrapper Definition
//-------------------------------------------------------------------------------------------------
// Defines a wrapper containing all modules comprising the Shared L1 Cache Subsystem.
//-------------------------------------------------------------------------------------------------
// File:       SharedL1CacheSubsystem.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 28/01/2011
//-------------------------------------------------------------------------------------------------

#ifndef SHAREDL1CACHESUBSYSTEM_HPP_
#define SHAREDL1CACHESUBSYSTEM_HPP_

#include "../../Utility/Parameters.h"
#include "../TileComponent.h"
#include "SharedL1CacheBank.h"
#include "SharedL1CacheCrossbarSwitch.h"
#include "SharedL1CacheController.h"
#include "SharedL1CacheNetworkInterface.h"
#include "SimplifiedBackgroundMemory.h"

class SharedL1CacheSubsystem : public TileComponent {
	//---------------------------------------------------------------------------------------------
	// Ports
	//---------------------------------------------------------------------------------------------

public:

	// All ports are inherited from TileComponent

	//sc_in<bool>			clock;					// Clock
	//sc_in<Word>			*in;					// All inputs to the component. There should be NUM_CLUSTER_INPUTS of them.
	//sc_out<AddressedWord>	*out;					// All outputs of the component. There should be NUM_CLUSTER_OUTPUTS of them.
	//sc_in<bool>			*flowControlIn;			// A flow control signal for each output (NUM_CLUSTER_OUTPUTS).
	//sc_out<int> 			*flowControlOut;		// A flow control signal for each input (NUM_CLUSTER_INPUTS). Each one tells how much space is remaining in a particular input buffer.
	//sc_out<bool>			idle;					// Signal that this component is not currently doing any work.

	//---------------------------------------------------------------------------------------------
	// Subcomponents
	//---------------------------------------------------------------------------------------------

private:

	SharedL1CacheNetworkInterface **mNetworkInterfaces;
	SharedL1CacheController **mControllers;
	SharedL1CacheCrossbarSwitch mCrossbarSwitch;
	SharedL1CacheBank **mMemoryBanks;
	SimplifiedBackgroundMemory mBackgroundMemory;

	//---------------------------------------------------------------------------------------------
	// Signals
	//---------------------------------------------------------------------------------------------

private:

	sc_signal<bool>			*sNetworkInterfaceIdle;
	sc_signal<bool>			*sControllerIdle;

	//---------------------------------------------------------------------------------------------
	// Processes
	//---------------------------------------------------------------------------------------------

private:

	void processUpdateIdle();

	//---------------------------------------------------------------------------------------------
	// Constructors / Destructors
	//---------------------------------------------------------------------------------------------

public:

	SC_HAS_PROCESS(SharedL1CacheSubsystem);
	SharedL1CacheSubsystem(sc_module_name name, ComponentID id);
	virtual ~SharedL1CacheSubsystem();

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from TileComponent - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// Initialise the contents of this memory to the Words in the given vector

	virtual void storeData(const std::vector<Word>& data, MemoryAddr location);

	// Print the contents of this memory

	virtual void print(MemoryAddr start, MemoryAddr end) const;

	// Return the value at the given address

	virtual const Word getMemVal(MemoryAddr addr) const;

	//---------------------------------------------------------------------------------------------
	// Simulation utility methods inherited from Component - not part of simulated logic
	//---------------------------------------------------------------------------------------------

public:

	// The area of this component in square micrometres

	virtual double area() const;

	// The energy consumed by this component in picojoules

	virtual double energy() const;
};

#endif /* SHAREDL1CACHESUBSYSTEM_HPP_ */
