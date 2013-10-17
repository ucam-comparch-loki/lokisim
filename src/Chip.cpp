/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Chip.h"
#include "TileComponents/Cluster.h"
#include "TileComponents/Memory/MemoryBank.h"
#include "TileComponents/Memory/SimplifiedOnChipScratchpad.h"

void Chip::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
	clusters[component.getGlobalCoreNumber()]->storeData(instructions);
}

void Chip::storeData(vector<Word>& data, const ComponentID& component, MemoryAddr location) {
  if(component.isCore()) {
    assert(component.getGlobalCoreNumber() < clusters.size());
    clusters[component.getGlobalCoreNumber()]->storeData(data, location);
  }
  else if(component.isMemory()) {
    assert(component.getGlobalMemoryNumber() < memories.size());
    memories[component.getGlobalMemoryNumber()]->storeData(data,location);
  }
  else backgroundMemory.storeData(data, location);
}

const void* Chip::getMemoryData() {
	// Synchronize data first

	for (uint i = 0; i < memories.size(); i++)
		memories[i]->synchronizeData();

	// Return synchronized background memory data array

	return backgroundMemory.getData();
}

void Chip::print(const ComponentID& component, MemoryAddr start, MemoryAddr end) {
	if (component.isMemory())
		memories[component.getGlobalMemoryNumber()]->print(start, end);
	else
		backgroundMemory.print(start, end);
}

Word Chip::readWord(const ComponentID& component, MemoryAddr addr) {
	if (component.isMemory()) {
	  assert(component.getGlobalMemoryNumber() < memories.size());
	  return memories[component.getGlobalMemoryNumber()]->readWord(addr);
	}
	else
		return backgroundMemory.readWord(addr);
}

Word Chip::readByte(const ComponentID& component, MemoryAddr addr) {
	if (component.isMemory()) {
    assert(component.getGlobalMemoryNumber() < memories.size());
	  return memories[component.getGlobalMemoryNumber()]->readByte(addr);
	}
	else
		return backgroundMemory.readByte(addr);
}

void Chip::writeWord(const ComponentID& component, MemoryAddr addr, Word data) {
	if (component.isMemory()) {
    assert(component.getGlobalMemoryNumber() < memories.size());
	  return memories[component.getGlobalMemoryNumber()]->writeWord(addr, data);
	}
	else
		return backgroundMemory.writeWord(addr, data);
}

void Chip::writeByte(const ComponentID& component, MemoryAddr addr, Word data) {
	if (component.isMemory()) {
    assert(component.getGlobalMemoryNumber() < memories.size());
	  return memories[component.getGlobalMemoryNumber()]->writeByte(addr, data);
	}
	else
		return backgroundMemory.writeByte(addr, data);
}

int Chip::readRegister(const ComponentID& component, RegisterIndex reg) const {
  assert(component.getGlobalCoreNumber() < clusters.size());

	return clusters[component.getGlobalCoreNumber()]->readRegDebug(reg);
}

MemoryAddr Chip::getInstIndex(const ComponentID& component) const {
  assert(component.getGlobalCoreNumber() < clusters.size());

	return clusters[component.getGlobalCoreNumber()]->getInstIndex();
}

bool Chip::readPredicate(const ComponentID& component) const {
  assert(component.getGlobalCoreNumber() < clusters.size());

	return clusters[component.getGlobalCoreNumber()]->readPredReg();
}

void Chip::watchIdle(int component) {
  if(idleSig[component].read()) {
    idleComponents++;
    assert(idleComponents <= NUM_COMPONENTS);
    next_trigger(idleSig[component].negedge_event());
  }
  else {
    idleComponents--;
    assert(idleComponents >= 0);
    next_trigger(idleSig[component].posedge_event());
  }
}

bool Chip::isIdle() const {
  return idleComponents == NUM_COMPONENTS;
}

void Chip::makeSignals() {
  int numOutputs = TOTAL_OUTPUT_PORTS;
  int numInputs  = TOTAL_INPUT_PORTS;

  idleSig.init(NUM_COMPONENTS + 1);

  dataFromComponents.init(numOutputs);
  dataToComponents.init(numInputs);
  creditsFromComponents.init(NUM_CORES);
  creditsToComponents.init(numOutputs);

  readyFromComps.init(NUM_CORES * CORE_INPUT_CHANNELS + NUM_MEMORIES);

	strobeToBackgroundMemory.init(NUM_MEMORIES);
	dataToBackgroundMemory.init(NUM_MEMORIES);
	strobeFromBackgroundMemory.init(NUM_MEMORIES);
	dataFromBackgroundMemory.init(NUM_MEMORIES);

	ringStrobe.init(NUM_MEMORIES);
	ringRequest.init(NUM_MEMORIES);
	ringAcknowledge.init(NUM_MEMORIES);
}

void Chip::makeComponents() {
  for(uint j=0; j<NUM_TILES; j++) {
    std::stringstream namebuilder;
    std::string name;

    // Initialise the Clusters of this Tile
    for(uint i=0; i<CORES_PER_TILE; i++) {
      ComponentID clusterID(j, i);

      namebuilder << "core_" << clusterID.getNameString();
      namebuilder >> name;
      namebuilder.clear();

      Cluster* c = new Cluster(name.c_str(), clusterID, network.getLocalNetwork(clusterID));
      
      clusters.push_back(c);
    }

    // Initialise the memories of this Tile
    for (uint i = 0; i < MEMS_PER_TILE; i++) {
			ComponentID memoryID(j, CORES_PER_TILE + i);

			namebuilder << "memory_" << memoryID.getNameString();
			namebuilder >> name;
			namebuilder.clear();

			MemoryBank* m = new MemoryBank(name.c_str(), memoryID, i);
			m->setLocalNetwork(network.getLocalNetwork(memoryID));

			memories.push_back(m);
		}
  }
}

void Chip::wireUp() {
  network.clock(clock);
  network.fastClock(fastClock);
  network.slowClock(slowClock);

	backgroundMemory.iClock(clock);
	backgroundMemory.oIdle(idleSig[NUM_COMPONENTS]);

// Connect clusters and memories to the local interconnect

	uint clusterIndex = 0;
	uint memoryIndex = 0;
	uint dataInputCounter = 0;
	uint dataOutputCounter = 0;
  uint creditInputCounter = 0;
  uint creditOutputCounter = 0;
  uint readyInputCounter = 0;

	for (uint i = 0; i < NUM_COMPONENTS; i++) {
		if ((i % COMPONENTS_PER_TILE) < CORES_PER_TILE) {
			// This is a core
      clusters[clusterIndex]->idle(idleSig[clusterIndex+memoryIndex]);
      clusters[clusterIndex]->clock(clock);
      clusters[clusterIndex]->fastClock(fastClock);

			for (uint j = 0; j < CORE_INPUT_PORTS; j++) {
				uint index = dataInputCounter++;  // Position in network's array

				clusters[clusterIndex]->dataIn[j](dataToComponents[index]);
				network.dataOut[index](dataToComponents[index]);
			}

			for (uint j = 0; j < CORE_INPUT_CHANNELS; j++) {
			  uint index = readyInputCounter++;  // Position in network's array

        clusters[clusterIndex]->readyOut[j](readyFromComps[index]);
        network.readyDataOut[index](readyFromComps[index]);
			}

      uint index = creditOutputCounter++;

      clusters[clusterIndex]->creditsOut[0](creditsFromComponents[index]);
      network.creditsIn[index](creditsFromComponents[index]);

			for (uint j = 0; j < CORE_OUTPUT_PORTS; j++) {
				uint index = dataOutputCounter++;  // Position in network's array

				clusters[clusterIndex]->dataOut[j](dataFromComponents[index]);
				network.dataIn[index](dataFromComponents[index]);

				index = creditInputCounter++;

				clusters[clusterIndex]->creditsIn[j](creditsToComponents[index]);
				network.creditsOut[index](creditsToComponents[index]);
			}

			clusterIndex++;
		} else {
			uint indexInput = dataInputCounter++;  // Position in network's array
			memories[memoryIndex]->iDataIn(dataToComponents[indexInput]);
			network.dataOut[indexInput](dataToComponents[indexInput]);

			uint indexReady = readyInputCounter++;  // Position in network's array
			memories[memoryIndex]->oReadyForData(readyFromComps[indexReady]);
	    network.readyDataOut[indexReady](readyFromComps[indexReady]);

			uint indexOutput = dataOutputCounter++;  // Position in network's array
			memories[memoryIndex]->oDataOut(dataFromComponents[indexOutput]);
			network.dataIn[indexOutput](dataFromComponents[indexOutput]);

			memoryIndex++;
		}
	}
  
  for(uint j=0; j<NUM_TILES; j++) {
 	  for (uint i = 0; i < MEMS_PER_TILE; i++) {
			int currIndex = j * MEMS_PER_TILE + i;
			int prevIndex = j * MEMS_PER_TILE + ((i + MEMS_PER_TILE - 1) % MEMS_PER_TILE);
			int nextIndex = j * MEMS_PER_TILE + ((i + 1) % MEMS_PER_TILE);
		
			MemoryBank* m = memories[currIndex];

			m->iClock(clock);
			m->oIdle(idleSig[m->id.getGlobalComponentNumber()]);

			int portIndex = j * MEMS_PER_TILE + i;

			m->oBMDataStrobe(strobeToBackgroundMemory[portIndex]);
			backgroundMemory.iDataStrobe[portIndex](strobeToBackgroundMemory[portIndex]);
			m->oBMData(dataToBackgroundMemory[portIndex]);
			backgroundMemory.iData[portIndex](dataToBackgroundMemory[portIndex]);

			m->iBMDataStrobe((strobeFromBackgroundMemory[portIndex]));
			backgroundMemory.oDataStrobe[portIndex](strobeFromBackgroundMemory[portIndex]);
			m->iBMData((dataFromBackgroundMemory[portIndex]));
			backgroundMemory.oData[portIndex](dataFromBackgroundMemory[portIndex]);
				
			// Connect the memory ring network of this tile
	
			memories[nextIndex]->oRingAcknowledge(ringAcknowledge[currIndex]);
			memories[nextIndex]->iRingStrobe(ringStrobe[currIndex]);
			memories[nextIndex]->iRingRequest(ringRequest[currIndex]);

			memories[currIndex]->iRingAcknowledge(ringAcknowledge[currIndex]);
			memories[currIndex]->oRingStrobe(ringStrobe[currIndex]);
			memories[currIndex]->oRingRequest(ringRequest[currIndex]);

			memories[currIndex]->setAdjacentMemories(memories[prevIndex], memories[nextIndex], &backgroundMemory);
		}
	}
}

Chip::Chip(const sc_module_name& name, const ComponentID& ID) :
    Component(name),
    backgroundMemory("background_memory", NUM_COMPONENTS, NUM_MEMORIES),
    network("network"),
    clock("clock", 1, SC_NS, 0.5),
    fastClock("fast_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.25),
    slowClock("slow_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.75) {
  
  idleComponents = 0;

  makeSignals();
  makeComponents();
  wireUp();

  // Generate a method to watch each component's idle signal, so we can
	// determine when all components are idle. For large numbers of components,
	// this is cheaper than checking all of them whenever one changes.
  for(unsigned int i=0; i<NUM_COMPONENTS; i++)
    SPAWN_METHOD(idleSig[i], Chip::watchIdle, i, true);

}

Chip::~Chip() {
	for(uint i = 0; i < clusters.size(); i++)	delete clusters[i];
	for(uint i = 0; i < memories.size(); i++)	delete memories[i];
}
