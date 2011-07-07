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

    // If all of the components are now idle, the chip is now idle.
    if(isIdle()) idlenessChanged.notify();
  }
  else {
    // If all of the components were idle, and now one is busy, the chip is
    // no longer idle.
    if(isIdle()) idlenessChanged.notify();

    idleComponents--;
    assert(idleComponents >= 0);
    next_trigger(idleSig[component].posedge_event());
  }
}

bool Chip::isIdle() const {
  return idleComponents == NUM_COMPONENTS;
}

void Chip::updateIdle() {
  idle.write(isIdle());
}

void Chip::makeSignals() {
  int numOutputs = TOTAL_OUTPUT_PORTS;
  int numInputs  = TOTAL_INPUT_PORTS;

  idleSig               = new sc_signal<bool>[NUM_COMPONENTS + 1];

  dataFromComponents    = new sc_buffer<DataType>[numOutputs];
  dataToComponents      = new flag_signal<DataType>[numInputs];
  creditsFromComponents = new sc_buffer<CreditType>[NUM_CORES];
  creditsToComponents   = new sc_buffer<CreditType>[numOutputs];

  ackDataFromComps      = new sc_signal<ReadyType>[numOutputs];
  ackCreditToComps      = new sc_signal<ReadyType>[numOutputs];
  ackDataToComps        = new sc_signal<ReadyType>[numInputs];
  ackCreditFromComps    = new sc_signal<ReadyType>[NUM_CORES];

  validDataFromComps    = new sc_signal<ReadyType>[numOutputs];
  validDataToComps      = new sc_signal<ReadyType>[numInputs];
  validCreditFromComps  = new sc_signal<ReadyType>[NUM_CORES];
  validCreditToComps    = new sc_signal<ReadyType>[numOutputs];

	strobeToBackgroundMemory   = new sc_signal<bool>[NUM_MEMORIES];
	dataToBackgroundMemory 	   = new sc_signal<MemoryRequest>[NUM_MEMORIES];
	strobeFromBackgroundMemory = new sc_signal<bool>[NUM_MEMORIES];
	dataFromBackgroundMemory 	 = new sc_signal<Word>[NUM_MEMORIES];

	ringStrobe 								 = new sc_signal<bool>[NUM_MEMORIES];
	ringRequest 							 = new sc_signal<MemoryBank::RingNetworkRequest>[NUM_MEMORIES];
	ringAcknowledge 					 = new sc_signal<bool>[NUM_MEMORIES];
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

      Cluster* c = new Cluster(name.c_str(), clusterID);
      
      clusters.push_back(c);
    }

    // Initialise the memories of this Tile
    for (uint i = 0; i < MEMS_PER_TILE; i++) {
			ComponentID memoryID(j, CORES_PER_TILE + i);

			namebuilder << "memory_" << memoryID.getNameString();
			namebuilder >> name;
			namebuilder.clear();

			MemoryBank* m = new MemoryBank(name.c_str(), memoryID, i);

			memories.push_back(m);
		}
  }
}

void Chip::wireUp() {
  network.clock(clock);

	backgroundMemory.iClock(clock);
	backgroundMemory.oIdle(idleSig[NUM_COMPONENTS]);

// Connect clusters and memories to the local interconnect

	uint clusterIndex = 0;
	uint memoryIndex = 0;
	uint dataInputCounter = 0;
	uint dataOutputCounter = 0;
  uint creditInputCounter = 0;
  uint creditOutputCounter = 0;

	for (uint i = 0; i < NUM_COMPONENTS; i++) {
		if ((i % COMPONENTS_PER_TILE) < CORES_PER_TILE) {
			// This is a core
      clusters[clusterIndex]->idle(idleSig[clusterIndex+memoryIndex]);
      clusters[clusterIndex]->clock(clock);
      clusters[clusterIndex]->fastClock(fastClock);
      clusters[clusterIndex]->slowClock(slowClock);

			for (uint j = 0; j < CORE_INPUT_PORTS; j++) {
				uint index = dataInputCounter++;  // Position in network's array

				clusters[clusterIndex]->dataIn[j](dataToComponents[index]);
				network.dataOut[index](dataToComponents[index]);
				clusters[clusterIndex]->validDataIn[j](validDataToComps[index]);
				network.validDataOut[index](validDataToComps[index]);
				clusters[clusterIndex]->ackDataIn[j](ackDataToComps[index]);
				network.ackDataOut[index](ackDataToComps[index]);
			}

      uint index = creditOutputCounter++;

      clusters[clusterIndex]->creditsOut[0](creditsFromComponents[index]);
      network.creditsIn[index](creditsFromComponents[index]);
      clusters[clusterIndex]->validCreditOut[0](validCreditFromComps[index]);
      network.validCreditIn[index](validCreditFromComps[index]);
      clusters[clusterIndex]->ackCreditOut[0](ackCreditFromComps[index]);
      network.ackCreditIn[index](ackCreditFromComps[index]);

			for (uint j = 0; j < CORE_OUTPUT_PORTS; j++) {
				uint index = dataOutputCounter++;  // Position in network's array

				clusters[clusterIndex]->dataOut[j](dataFromComponents[index]);
				network.dataIn[index](dataFromComponents[index]);
				clusters[clusterIndex]->validDataOut[j](validDataFromComps[index]);
				network.validDataIn[index](validDataFromComps[index]);
				clusters[clusterIndex]->ackDataOut[j](ackDataFromComps[index]);
				network.ackDataIn[index](ackDataFromComps[index]);

				index = creditInputCounter++;

				clusters[clusterIndex]->creditsIn[j](creditsToComponents[index]);
				network.creditsOut[index](creditsToComponents[index]);
				clusters[clusterIndex]->validCreditIn[j](validCreditToComps[index]);
				network.validCreditOut[index](validCreditToComps[index]);
				clusters[clusterIndex]->ackCreditIn[j](ackCreditToComps[index]);
				network.ackCreditOut[index](ackCreditToComps[index]);
			}

			clusterIndex++;
		} else {
			uint indexInput = dataInputCounter++;  // Position in network's array

			memories[memoryIndex]->iDataIn(dataToComponents[indexInput]);
			network.dataOut[indexInput](dataToComponents[indexInput]);
			memories[memoryIndex]->iDataInValid(validDataToComps[indexInput]);
			network.validDataOut[indexInput](validDataToComps[indexInput]);
			memories[memoryIndex]->oDataInAcknowledge(ackDataToComps[indexInput]);
			network.ackDataOut[indexInput](ackDataToComps[indexInput]);

			uint indexOutput = dataOutputCounter++;  // Position in network's array

			memories[memoryIndex]->oDataOut(dataFromComponents[indexOutput]);
			network.dataIn[indexOutput](dataFromComponents[indexOutput]);
			memories[memoryIndex]->oDataOutValid(validDataFromComps[indexOutput]);
			network.validDataIn[indexOutput](validDataFromComps[indexOutput]);
			memories[memoryIndex]->iDataOutAcknowledge(ackDataFromComps[indexOutput]);
			network.ackDataIn[indexOutput](ackDataFromComps[indexOutput]);

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
	fastClock("fast_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.25),
  slowClock("slow_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.75)
{
  idleComponents = 0;

  makeSignals();
  makeComponents();
  wireUp();

	SC_METHOD(updateIdle);
	sensitive << idlenessChanged;

  // Generate a method to watch each component's idle signal, so we can
	// determine when all components are idle. For large numbers of components,
	// this is cheaper than checking all of them whenever one changes.
  for(unsigned int i=0; i<NUM_COMPONENTS; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.set_sensitivity(&(idleSig[i])); // Sensitive to this signal

    // Create the method.
    sc_spawn(sc_bind(&Chip::watchIdle, this, i), 0, &options);
  }
}

Chip::~Chip() {
	delete[] idleSig;
	delete[] dataFromComponents;  			delete[] dataToComponents;
	delete[] creditsFromComponents; 		delete[] creditsToComponents;

	delete[] ackDataFromComps;  				delete[] ackCreditToComps;
	delete[] ackDataToComps;  					delete[] ackCreditFromComps;

	delete[] validDataFromComps;  			delete[] validDataToComps;
	delete[] validCreditFromComps;  		delete[] validCreditToComps;

	delete[] strobeToBackgroundMemory;	delete[] strobeFromBackgroundMemory;
	delete[] dataToBackgroundMemory;		delete[] dataFromBackgroundMemory;

	delete[] ringStrobe;
	delete[] ringRequest;
	delete[] ringAcknowledge;

	for(uint i = 0; i < clusters.size(); i++)	delete clusters[i];
	for(uint i = 0; i < memories.size(); i++)	delete memories[i];
}
