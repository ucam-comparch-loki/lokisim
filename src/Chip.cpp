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

  dataFromComponents     = new flag_signal<DataType>[numOutputs];
  dataToComponents       = new flag_signal<DataType>[numInputs];
  creditsFromComponents  = new flag_signal<CreditType>[numInputs];
  creditsToComponents    = new sc_buffer<CreditType>[numOutputs];

  networkReadyForData    = new sc_signal<ReadyType>[numOutputs];
  compsReadyForCredits   = new sc_signal<ReadyType>[numOutputs];
  compsReadyForData      = new sc_signal<ReadyType>[numInputs];
  networkReadyForCredits = new sc_signal<ReadyType>[numInputs];

  network.clock(clock);

  for(uint j=0; j<NUM_TILES; j++) {
    // Initialise the Clusters of this Tile
    for(uint i=0; i<CORES_PER_TILE; i++) {
      ComponentID clusterID = j*COMPONENTS_PER_TILE + i;
      Cluster* c = new Cluster("cluster", clusterID);
      c->idle(idleSig[clusterID]);
      contents.push_back(c);
    }

    // Initialise the memories of this Tile
    if (ENABLE_SHARED_L1_CACHE_SUBSYSTEM == 0) {
		for(uint i=CORES_PER_TILE; i<COMPONENTS_PER_TILE; i++) {
			ComponentID memoryID = j*COMPONENTS_PER_TILE + i;
			MemoryMat* m = new MemoryMat("memory", memoryID);
			m->idle(idleSig[memoryID]);
			contents.push_back(m);
		}
    } else {
		ComponentID memoryID = j * COMPONENTS_PER_TILE + CORES_PER_TILE;
		SharedL1CacheSubsystem* m = new SharedL1CacheSubsystem("memory", memoryID, eventRecorder);
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

      contents[i]->canReceiveData[j](compsReadyForData[index]);
      network.canSendData[index](compsReadyForData[index]);
      contents[i]->canSendCredit[j](networkReadyForCredits[index]);
      network.canReceiveCredit[index](networkReadyForCredits[index]);
    }

    for(int j=0; j<outputs; j++) {
      int index = TileComponent::outputPortID(i,j); // Position in network's array

      contents[i]->dataOut[j](dataFromComponents[index]);
      network.dataIn[index](dataFromComponents[index]);
      contents[i]->canSendData[j](networkReadyForData[index]);
      network.canReceiveData[index](networkReadyForData[index]);

      contents[i]->creditsIn[j](creditsToComponents[index]);
      network.creditsOut[index](creditsToComponents[index]);
      contents[i]->canReceiveCredit[j](compsReadyForCredits[index]);
      network.canSendCredit[index](compsReadyForCredits[index]);
    }

    contents[i]->clock(clock);

  }

}

Chip::~Chip() {
  delete[] idleSig;
  delete[] dataFromComponents;
  delete[] dataToComponents;
  delete[] creditsFromComponents;
  delete[] networkReadyForData;

  delete[] creditsToComponents;
  delete[] compsReadyForCredits;

  delete[] compsReadyForData;
  delete[] networkReadyForCredits;

  for(uint i=0; i<contents.size(); i++) delete contents[i];
}
