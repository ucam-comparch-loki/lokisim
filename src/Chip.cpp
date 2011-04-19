/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#include "Chip.h"
#include "TileComponents/Cluster.h"
#include "TileComponents/MemoryMat.h"
#include "TileComponents/Memory/SharedL1CacheSubsystem.h"

double Chip::area() const {
  // Update this if allowing heterogeneity.
  return network.area() +
         contents[0]->area() * CORES_PER_TILE +
         contents[CORES_PER_TILE]->area() * MEMS_PER_TILE;
}

double Chip::energy() const {
  // Update this if allowing heterogeneity.
  return network.energy() +
         contents[0]->energy() * CORES_PER_TILE +
         contents[CORES_PER_TILE]->energy() * MEMS_PER_TILE;
}

bool Chip::isIdle() const {
  return idleVal;
}

void Chip::storeData(vector<Word>& data, ComponentID component, MemoryAddr location) {
  assert(component < contents.size());
  contents[component]->storeData(data, location);
}

void Chip::print(ComponentID component, MemoryAddr start, MemoryAddr end) const {
  if(start<end) contents[component]->print(start, end);
  else          contents[component]->print(end, start);
}

Word Chip::readWord(ComponentID component, MemoryAddr addr) const {
  return contents[component]->readWord(addr);
}

Word Chip::readByte(ComponentID component, MemoryAddr addr) const {
  return contents[component]->readByte(addr);
}

void Chip::writeWord(ComponentID component, MemoryAddr addr, Word data) const {
  return contents[component]->writeWord(addr, data);
}

void Chip::writeByte(ComponentID component, MemoryAddr addr, Word data) const {
  return contents[component]->writeByte(addr, data);
}

int Chip::readRegister(ComponentID component, RegisterIndex reg) const {
  return contents[component]->readRegDebug(reg);
}

MemoryAddr Chip::getInstIndex(ComponentID component) const {
  return contents[component]->getInstIndex();
}

bool Chip::readPredicate(ComponentID component) const {
  return contents[component]->readPredReg();
}

void Chip::updateIdle() {
  idleVal = true;

  for(uint i=0; i<NUM_COMPONENTS; i++) {
    idleVal &= idleSig[i].read();
  }

  // Also check network to make sure data/instructions aren't in transit

  idle.write(idleVal);
}

Chip::Chip(sc_module_name name, ComponentID ID, BatchModeEventRecorder *eventRecorder) :
    Component(name),
    network("network") {

  idleSig = new sc_signal<bool>[NUM_COMPONENTS];

  SC_METHOD(updateIdle);
  for(uint i=0; i<NUM_COMPONENTS; i++) sensitive << idleSig[i];
  // do initialise

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

  for(uint j=0; j<NUM_TILES; j++) {
    std::stringstream namebuilder;
    std::string name;

    // Initialise the Clusters of this Tile
    for(uint i=0; i<CORES_PER_TILE; i++) {
      ComponentID clusterID = j*COMPONENTS_PER_TILE + i;

      namebuilder << "core_" << clusterID;
      namebuilder >> name;
      namebuilder.clear();

      Cluster* c = new Cluster(name.c_str(), clusterID);
      c->idle(idleSig[clusterID]);
      contents.push_back(c);
    }

    // Initialise the memories of this Tile
    if (ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) {
		for(uint i=CORES_PER_TILE; i<COMPONENTS_PER_TILE; i++) {
			ComponentID memoryID = j*COMPONENTS_PER_TILE + i;

      namebuilder << "memory_" << memoryID;
      namebuilder >> name;
      namebuilder.clear();

			MemoryMat* m = new MemoryMat(name.c_str(), memoryID);
			m->idle(idleSig[memoryID]);
			contents.push_back(m);
		}
    } else {
      ComponentID memoryID = j * COMPONENTS_PER_TILE + CORES_PER_TILE;

      namebuilder << "memory_" << memoryID;
      namebuilder >> name;
      namebuilder.clear();

      SharedL1CacheSubsystem* m =
          new SharedL1CacheSubsystem(name.c_str(), memoryID, eventRecorder);
      m->idle(idleSig[memoryID]);
      contents.push_back(m);
    }
  }

  // Connect the clusters and memories to the local interconnect
  for(uint i=0; i<NUM_COMPONENTS; i++) {
    bool isCore = (i%COMPONENTS_PER_TILE) < CORES_PER_TILE;
    int inputs = isCore ? CORE_INPUT_PORTS : MEMORY_INPUT_PORTS;
    int outputs = isCore ? CORE_OUTPUT_PORTS : MEMORY_OUTPUT_PORTS;

    for(int j=0; j<inputs; j++) {
      int index = TileComponent::inputPortID(i,j);  // Position in network's array

      contents[i]->dataIn[j](dataToComponents[index]);
      network.dataOut[index](dataToComponents[index]);
      contents[i]->creditsOut[j](creditsFromComponents[index]);
      network.creditsIn[index](creditsFromComponents[index]);

      contents[i]->ackDataIn[j](ackDataToComps[index]);
      network.ackDataOut[index](ackDataToComps[index]);
      contents[i]->ackCreditOut[j](ackCreditFromComps[index]);
      network.ackCreditIn[index](ackCreditFromComps[index]);

      contents[i]->validDataIn[j](validDataToComps[index]);
      network.validDataOut[index](validDataToComps[index]);
      contents[i]->validCreditOut[j](validCreditFromComps[index]);
      network.validCreditIn[index](validCreditFromComps[index]);
    }

    for(int j=0; j<outputs; j++) {
      int index = TileComponent::outputPortID(i,j); // Position in network's array

      contents[i]->dataOut[j](dataFromComponents[index]);
      network.dataIn[index](dataFromComponents[index]);
      contents[i]->ackDataOut[j](ackDataFromComps[index]);
      network.ackDataIn[index](ackDataFromComps[index]);

      contents[i]->creditsIn[j](creditsToComponents[index]);
      network.creditsOut[index](creditsToComponents[index]);
      contents[i]->ackCreditIn[j](ackCreditToComps[index]);
      network.ackCreditOut[index](ackCreditToComps[index]);

      contents[i]->validDataOut[j](validDataFromComps[index]);
      network.validDataIn[index](validDataFromComps[index]);
      contents[i]->validCreditIn[j](validCreditToComps[index]);
      network.validCreditOut[index](validCreditToComps[index]);
    }

    contents[i]->clock(clock);

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

  for(uint i=0; i<contents.size(); i++) delete contents[i];
}
