/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Chip.h"
#include "TileComponents/Cluster.h"
#include "TileComponents/Memory/MemoryBank.h"
#include "TileComponents/Memory/SimplifiedOnChipScratchpad.h"

double Chip::area() const {
	return network.area() +
		clusters[0]->area() * CORES_PER_TILE +
		memories[0]->area() * MEMS_PER_TILE;
}

double Chip::energy() const {
	return network.energy() +
		clusters[0]->energy() * CORES_PER_TILE +
		memories[0]->energy() * MEMS_PER_TILE;
}

bool Chip::isIdle() const {
	return idleVal;
}

void Chip::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
	clusters[component.getGlobalCoreNumber()]->storeData(instructions);
}

void Chip::storeData(vector<Word>& data, const ComponentID& component, MemoryAddr location) {
  if(component.isCore()) {
    clusters[component.getGlobalCoreNumber()]->storeData(data, location);
  }
  else if(component.isMemory()) {
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
	if (component.isMemory())
		return memories[component.getGlobalMemoryNumber()]->readWord(addr);
	else
		return backgroundMemory.readWord(addr);
}

Word Chip::readByte(const ComponentID& component, MemoryAddr addr) {
	if (component.isMemory())
		return memories[component.getGlobalMemoryNumber()]->readByte(addr);
	else
		return backgroundMemory.readByte(addr);
}

void Chip::writeWord(const ComponentID& component, MemoryAddr addr, Word data) {
	if (component.isMemory())
		return memories[component.getGlobalMemoryNumber()]->writeWord(addr, data);
	else
		return backgroundMemory.writeWord(addr, data);
}

void Chip::writeByte(const ComponentID& component, MemoryAddr addr, Word data) {
	if (component.isMemory())
		return memories[component.getGlobalMemoryNumber()]->writeByte(addr, data);
	else
		return backgroundMemory.writeByte(addr, data);
}

int Chip::readRegister(const ComponentID& component, RegisterIndex reg) const {
	return clusters[component.getGlobalCoreNumber()]->readRegDebug(reg);
}

MemoryAddr Chip::getInstIndex(const ComponentID& component) const {
	return clusters[component.getGlobalCoreNumber()]->getInstIndex();
}

bool Chip::readPredicate(const ComponentID& component) const {
	return clusters[component.getGlobalCoreNumber()]->readPredReg();
}

void Chip::updateIdle() {
	idleVal = true;

	for(uint i=0; i<NUM_COMPONENTS; i++)
		idleVal &= idleSig[i].read();

	// Also check network to make sure data/instructions aren't in transit

	idle.write(idleVal);
}

Chip::Chip(sc_module_name name, const ComponentID& ID) :
	Component(name),
	backgroundMemory("background_memory", NUM_COMPONENTS, NUM_MEMORIES),
	network("network")
{
	// Initialise idle signal vector

	idleSig = new sc_signal<bool>[NUM_COMPONENTS + 1];

	SC_METHOD(updateIdle);
	for (uint i = 0; i < NUM_COMPONENTS; i++)
		sensitive << idleSig[i];

	// Initialise network signals

	int numOutputs = TOTAL_OUTPUT_PORTS;
	int numInputs  = TOTAL_INPUT_PORTS;

	dataFromComponents    = new sc_buffer<DataType>[numOutputs];
	dataToComponents      = new flag_signal<DataType>[numInputs];
	creditsFromComponents = new sc_buffer<CreditType>[numInputs];
	creditsToComponents   = new sc_buffer<CreditType>[numOutputs];

	ackDataFromComps      = new sc_signal<ReadyType>[numOutputs];
	ackCreditToComps      = new sc_signal<ReadyType>[numOutputs];
	ackDataToComps        = new sc_signal<ReadyType>[numInputs];
	ackCreditFromComps    = new sc_signal<ReadyType>[numInputs];

	validDataFromComps    = new sc_signal<ReadyType>[numOutputs];
	validDataToComps      = new sc_signal<ReadyType>[numInputs];
	validCreditFromComps  = new sc_signal<ReadyType>[numInputs];
	validCreditToComps    = new sc_signal<ReadyType>[numOutputs];

	network.clock(clock);

	// Initialise clusters

	for (uint j = 0; j < NUM_TILES; j++) {
		std::stringstream namebuilder;
		std::string name;

		// Initialise the clusters of this tile

		for (uint i = 0; i < CORES_PER_TILE; i++) {
			ComponentID clusterID(j, i);

			namebuilder << "core_" << clusterID.getNameString();
			namebuilder >> name;
			namebuilder.clear();

			Cluster* c = new Cluster(name.c_str(), clusterID);
			c->clock(clock);
			c->idle(idleSig[clusterID.getGlobalComponentNumber()]);
			clusters.push_back(c);
		}
	}

	// Initialise memories

	strobeToBackgroundMemory = new sc_signal<bool>[NUM_MEMORIES];
	dataToBackgroundMemory = new sc_signal<Word>[NUM_MEMORIES];
	strobeFromBackgroundMemory = new sc_signal<bool>[NUM_MEMORIES];
	dataFromBackgroundMemory = new sc_signal<Word>[NUM_MEMORIES];

	ringStrobe = new sc_signal<bool>[NUM_MEMORIES];
	ringRequest = new sc_signal<MemoryBank::RingNetworkRequest>[NUM_MEMORIES];
	ringAcknowledge = new sc_signal<bool>[NUM_MEMORIES];

	backgroundMemory.iClock(clock);
	backgroundMemory.oIdle(idleSig[NUM_COMPONENTS]);

	for (uint j = 0; j < NUM_TILES; j++) {
		std::stringstream namebuilder;
		std::string name;

		// Initialise the memories of this tile

		for (uint i = 0; i < MEMS_PER_TILE; i++) {
			ComponentID memoryID(j, CORES_PER_TILE + i);

			namebuilder << "memory_" << memoryID.getNameString();
			namebuilder >> name;
			namebuilder.clear();

			MemoryBank* m = new MemoryBank(name.c_str(), memoryID, i);

			m->iClock(clock);
			m->oIdle(idleSig[memoryID.getGlobalComponentNumber()]);

			int portIndex = j * MEMS_PER_TILE + i;

			m->oBMDataStrobe(strobeToBackgroundMemory[portIndex]);
			backgroundMemory.iDataStrobe[portIndex](strobeToBackgroundMemory[portIndex]);
			m->oBMData(dataToBackgroundMemory[portIndex]);
			backgroundMemory.iData[portIndex](dataToBackgroundMemory[portIndex]);

			m->iBMDataStrobe((strobeFromBackgroundMemory[portIndex]));
			backgroundMemory.oDataStrobe[portIndex](strobeFromBackgroundMemory[portIndex]);
			m->iBMData((dataFromBackgroundMemory[portIndex]));
			backgroundMemory.oData[portIndex](dataFromBackgroundMemory[portIndex]);

			memories.push_back(m);
		}

		// Connect the memory ring network of this tile

		for (uint i = 0; i < MEMS_PER_TILE; i++) {
			int currIndex = j * MEMS_PER_TILE + i;
			int nextIndex = j * MEMS_PER_TILE + ((i + 1) % MEMS_PER_TILE);

			memories[currIndex]->oRingAcknowledge(ringAcknowledge[currIndex]);
			memories[nextIndex]->iRingAcknowledge(ringAcknowledge[currIndex]);
			memories[currIndex]->oRingStrobe(ringStrobe[currIndex]);
			memories[nextIndex]->iRingStrobe(ringStrobe[currIndex]);
			memories[currIndex]->oRingRequest(ringRequest[currIndex]);
			memories[nextIndex]->iRingRequest(ringRequest[currIndex]);
		}
	}

	// Connect clusters and memories to the local interconnect

	uint clusterIndex = 0;
	uint memoryIndex = 0;
	uint inputPortCounter = 0;
	uint outputPortCounter = 0;

	for (uint i = 0; i < NUM_COMPONENTS; i++) {
		if ((i % COMPONENTS_PER_TILE) < CORES_PER_TILE) {
			// This is a core

			for (uint j = 0; j < CORE_INPUT_PORTS; j++) {
				uint index = inputPortCounter++;  // Position in network's array

				clusters[clusterIndex]->dataIn[j](dataToComponents[index]);
				network.dataOut[index](dataToComponents[index]);
				clusters[clusterIndex]->validDataIn[j](validDataToComps[index]);
				network.validDataOut[index](validDataToComps[index]);
				clusters[clusterIndex]->ackDataIn[j](ackDataToComps[index]);
				network.ackDataOut[index](ackDataToComps[index]);

				clusters[clusterIndex]->creditsOut[j](creditsFromComponents[index]);
				network.creditsIn[index](creditsFromComponents[index]);
				clusters[clusterIndex]->validCreditOut[j](validCreditFromComps[index]);
				network.validCreditIn[index](validCreditFromComps[index]);
				clusters[clusterIndex]->ackCreditOut[j](ackCreditFromComps[index]);
				network.ackCreditIn[index](ackCreditFromComps[index]);
			}

			for (uint j = 0; j < CORE_OUTPUT_PORTS; j++) {
				uint index = outputPortCounter++;  // Position in network's array

				clusters[clusterIndex]->dataOut[j](dataFromComponents[index]);
				network.dataIn[index](dataFromComponents[index]);
				clusters[clusterIndex]->validDataOut[j](validDataFromComps[index]);
				network.validDataIn[index](validDataFromComps[index]);
				clusters[clusterIndex]->ackDataOut[j](ackDataFromComps[index]);
				network.ackDataIn[index](ackDataFromComps[index]);

				clusters[clusterIndex]->creditsIn[j](creditsToComponents[index]);
				network.creditsOut[index](creditsToComponents[index]);
				clusters[clusterIndex]->validCreditIn[j](validCreditToComps[index]);
				network.validCreditOut[index](validCreditToComps[index]);
				clusters[clusterIndex]->ackCreditIn[j](ackCreditToComps[index]);
				network.ackCreditOut[index](ackCreditToComps[index]);
			}

			clusterIndex++;
		} else {
			uint indexInput = inputPortCounter++;  // Position in network's array

			memories[memoryIndex]->iDataIn(dataToComponents[indexInput]);
			network.dataOut[indexInput](dataToComponents[indexInput]);
			memories[memoryIndex]->iDataInValid(validDataToComps[indexInput]);
			network.validDataOut[indexInput](validDataToComps[indexInput]);
			memories[memoryIndex]->oDataInAcknowledge(ackDataToComps[indexInput]);
			network.ackDataOut[indexInput](ackDataToComps[indexInput]);

			network.creditsIn[indexInput](creditsFromComponents[indexInput]);
			network.ackCreditIn[indexInput](ackCreditFromComps[indexInput]);
			ackCreditFromComps[indexInput].write(false);
			network.validCreditIn[indexInput](validCreditFromComps[indexInput]);
			validCreditFromComps[indexInput].write(false);

			uint indexOutput = outputPortCounter++;  // Position in network's array

			memories[memoryIndex]->oDataOut(dataFromComponents[indexOutput]);
			network.dataIn[indexOutput](dataFromComponents[indexOutput]);
			memories[memoryIndex]->oDataOutValid(validDataFromComps[indexOutput]);
			network.validDataIn[indexOutput](validDataFromComps[indexOutput]);
			memories[memoryIndex]->iDataOutAcknowledge(ackDataFromComps[indexOutput]);
			network.ackDataIn[indexOutput](ackDataFromComps[indexOutput]);

			network.creditsOut[indexOutput](creditsToComponents[indexOutput]);
			network.validCreditOut[indexOutput](validCreditToComps[indexOutput]);
			network.ackCreditOut[indexOutput](ackCreditToComps[indexOutput]);

			memoryIndex++;
		}
	}
}

Chip::~Chip() {
	delete[] idleSig;
	delete[] dataFromComponents;
	delete[] dataToComponents;
	delete[] creditsFromComponents;
	delete[] creditsToComponents;

	delete[] ackDataFromComps;
	delete[] ackCreditToComps;
	delete[] ackDataToComps;
	delete[] ackCreditFromComps;

	delete[] validDataFromComps;
	delete[] validDataToComps;
	delete[] validCreditFromComps;
	delete[] validCreditToComps;

	delete[] strobeToBackgroundMemory;
	delete[] dataToBackgroundMemory;
	delete[] strobeFromBackgroundMemory;
	delete[] dataFromBackgroundMemory;

	delete[] ringStrobe;
	delete[] ringRequest;
	delete[] ringAcknowledge;

	for(uint i = 0; i < clusters.size(); i++)
		delete clusters[i];

	for(uint i = 0; i < memories.size(); i++)
		delete memories[i];
}
