/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Chip.h"
#include "Network/Global/DataNetwork.h"
#include "Network/Global/CreditNetwork.h"
#include "Network/Global/RequestNetwork.h"
#include "Network/Global/ResponseNetwork.h"
#include "TileComponents/Memory/MissHandlingLogic.h"
#include "Utility/Instrumentation/Stalls.h"
#include "Utility/StartUp/DataBlock.h"

using Instrumentation::Stalls;


void Chip::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
	cores[component.getGlobalCoreNumber()]->storeData(instructions);
}

void Chip::storeData(const DataBlock& data) {
  if (data.component().isCore()) {
    assert(data.component().getGlobalCoreNumber() < cores.size());
    cores[data.component().getGlobalCoreNumber()]->storeData(data.payload(), data.position());
  }
  else if (data.component().isMemory()) {
    assert(data.component().getGlobalMemoryNumber() < memories.size());
    memories[data.component().getGlobalMemoryNumber()]->storeData(data.payload(),data.position());
  }
  else backgroundMemory.storeData(data.payload(), data.position(), data.readOnly());
}

const void* Chip::getMemoryData() {
	// Synchronize data first

	//backgroundMemory.flushQueues();

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
  assert(component.getGlobalCoreNumber() < cores.size());

	return cores[component.getGlobalCoreNumber()]->readRegDebug(reg);
}

MemoryAddr Chip::getInstIndex(const ComponentID& component) const {
  assert(component.getGlobalCoreNumber() < cores.size());

	return cores[component.getGlobalCoreNumber()]->getInstIndex();
}

bool Chip::readPredicate(const ComponentID& component) const {
  assert(component.getGlobalCoreNumber() < cores.size());

	return cores[component.getGlobalCoreNumber()]->readPredReg();
}

bool Chip::isIdle() const {
  return Stalls::stalledComponents() == NUM_COMPONENTS;
}

void Chip::makeSignals() {
  int numOutputs = TOTAL_OUTPUT_PORTS;
  int numInputs  = TOTAL_INPUT_PORTS;

  oDataLocal.init(numOutputs);
  iDataLocal.init(numInputs);
  oReadyData.init(NUM_COMPONENTS);
  for (unsigned int i=0; i<oReadyData.length(); i++) {
    // Cores have a different number of input buffers/flow control signals
    // than memory banks.
    if (i % COMPONENTS_PER_TILE < CORES_PER_TILE)
      oReadyData[i].init(CORE_INPUT_CHANNELS);
    else
      oReadyData[i].init(1);
  }

  oDataGlobal.init(NUM_CORES);
  iDataGlobal.init(NUM_CORES);

  oCredit.init(NUM_CORES);
  iCredit.init(numOutputs);
  oReadyCredit.init(NUM_CORES * CORE_OUTPUT_PORTS);

  requestFromBanks.init(NUM_MEMORIES);
  requestToBanks.init(NUM_TILES);       // Broadcast within each tile
  readyRequestToBanks.init(NUM_MEMORIES);
  requestFromMHL.init(NUM_TILES);
  requestToBM.init(NUM_TILES);

  responseFromBanks.init(NUM_MEMORIES);
  responseToMHL.init(NUM_TILES);
  readyResponseToMHL.init(NUM_TILES);
  responseToBanks.init(NUM_TILES);      // Broadcast within each tile
  responseFromBM.init(NUM_TILES);

	ringStrobe.init(NUM_MEMORIES);
	ringRequest.init(NUM_MEMORIES);
	ringAcknowledge.init(NUM_MEMORIES);
}

void Chip::makeComponents() {
  for (uint tile=0; tile<NUM_TILES; tile++) {
    std::stringstream namebuilder;
    std::string name;

    // Initialise the Cores of this Tile
    for (uint core=0; core<CORES_PER_TILE; core++) {
      ComponentID coreID(tile, core);

      namebuilder << "core_" << coreID.getNameString();
      namebuilder >> name;
      namebuilder.clear();

      Core* c = new Core(name.c_str(), coreID, network.getLocalNetwork(coreID));
      
      cores.push_back(c);
    }

    // Initialise the memories of this Tile
    for (uint bank = 0; bank < MEMS_PER_TILE; bank++) {
			ComponentID memoryID(tile, CORES_PER_TILE + bank);

			namebuilder << "memory_" << memoryID.getNameString();
			namebuilder >> name;
			namebuilder.clear();

			MemoryBank* m = new MemoryBank(name.c_str(), memoryID, bank);
			m->setLocalNetwork(network.getLocalNetwork(memoryID));

			memories.push_back(m);
		}

    namebuilder << "mhl_" << tile;
    namebuilder >> name;
    namebuilder.clear();

    MissHandlingLogic* m = new MissHandlingLogic(name.c_str(), ComponentID(tile,0));
    mhl.push_back(m);
  }
}

void Chip::wireUp() {
  network.clock(clock);
  network.fastClock(fastClock);
  network.slowClock(slowClock);

	backgroundMemory.iClock(clock);

	// Global data network - connects cores to cores.
  DataNetwork* dataNet = new DataNetwork("data_net");
  dataNet->clock(clock);
  for (unsigned int i=0; i<cores.size(); i++) {
    cout << i << endl;
    cout << cores[i]->name() << endl;
    cout << iDataGlobal[i].name() << endl;
    cores[i]->iDataGlobal(iDataGlobal[i]);
    dataNet->oData[i](iDataGlobal[i]);

    cores[i]->oDataGlobal(oDataGlobal[i]);
    dataNet->iData[i](oDataGlobal[i]);

    for (unsigned int j=0; j<CORE_INPUT_CHANNELS; j++) {
      unsigned int component = cores[i]->id.getGlobalComponentNumber();
      cores[i]->oReadyData[j](oReadyData[component][j]);
      dataNet->iReady[i][j](oReadyData[component][j]);
    }
  }
  networks.push_back(dataNet);

  // Global credit network - connects cores to cores.
  CreditNetwork* creditNet = new CreditNetwork("credit_net");
  creditNet->clock(clock);
  for (unsigned int i=0; i<cores.size(); i++) {
    cores[i]->iCredit(iCredit[i]);
    creditNet->oData[i](iCredit[i]);

    cores[i]->oCredit(oCredit[i]);
    creditNet->iData[i](oCredit[i]);

    cores[i]->oReadyCredit(oReadyCredit[i]);
    creditNet->iReady[i][0](oReadyCredit[i]);
  }
  networks.push_back(creditNet);

  // Global request network - connects memories to memories.
  RequestNetwork* requestNet = new RequestNetwork("request_net");
  requestNet->clock(clock);
  requestNet->oMainMemoryRequest(requestToMainMemory);
//  backgroundMemory.iRequest(requestToMainMemory);
  requestNet->iMainMemoryReady(mainMemoryReadyRequest);
//  backgroundMemory.oReadyRequest(mainMemoryReadyRequest);
  requestNet->iMainMemoryRequest(requestFromMainMemory);
//  backgroundMemory.oRequest(requestFromMainMemory);
  for (unsigned int i=0; i<memories.size(); i++) {
    memories[i]->oRequest(requestFromBanks[i]);
    mhl[i/MEMS_PER_TILE]->iRequestFromBanks[i%MEMS_PER_TILE](requestFromBanks[i]);

    memories[i]->oReadyForRequest(readyRequestToBanks[i]);
    requestNet->iReady[i][0](readyRequestToBanks[i]);

    memories[i]->iRequest(requestToBanks[i/MEMS_PER_TILE]);
  }
  for (unsigned int i=0; i<mhl.size(); i++) {
    mhl[i]->oRequestToNetwork(requestFromMHL[i]);
    requestNet->iData[i](requestFromMHL[i]);

    mhl[i]->iRequestFromNetwork(requestToMHL[i]);
    requestNet->oData[i](requestToMHL[i]);

    mhl[i]->oRequestToBanks(requestToBanks[i]);
  }
  networks.push_back(requestNet);

  // Global response network - connects memories to memories.
  ResponseNetwork* responseNet = new ResponseNetwork("response_net");
  responseNet->clock(clock);
  responseNet->oMainMemoryResponse(responseToMainMemory);
//  backgroundMemory.iResponse(responseToMainMemory);
  responseNet->iMainMemoryReady(mainMemoryReadyResponse);
//  backgroundMemory.oReadyResponse(mainMemoryReadyResponse);
  responseNet->iMainMemoryResponse(responseFromMainMemory);
//  backgroundMemory.oResponse(responseFromMainMemory);
  for (unsigned int i=0; i<memories.size(); i++) {
    memories[i]->oResponse(responseFromBanks[i]);
    mhl[i/MEMS_PER_TILE]->iDataFromBanks[i%MEMS_PER_TILE](responseFromBanks[i]);
  }
  for (unsigned int i=0; i<mhl.size(); i++) {
    mhl[i]->iResponseFromNetwork(responseToMHL[i]);
    responseNet->oData[i](responseToMHL[i]);

    mhl[i]->oReadyForResponse(readyResponseToMHL[i]);
    responseNet->iReady[i][0](readyResponseToMHL[i]);

    mhl[i]->oResponseToNetwork(responseFromMHL[i]);
    responseNet->iData[i](responseFromMHL[i]);

    mhl[i]->oDataToBanks(responseToBanks[i]);
    for (unsigned int j=i*MEMS_PER_TILE; j<(i+1)*MEMS_PER_TILE; j++)
      memories[j]->iResponse(responseToBanks[i]);
  }
  networks.push_back(responseNet);

// Connect cores and memories to the local interconnect

	uint coreIndex = 0;
	uint memoryIndex = 0;
	uint dataInputCounter = 0;
	uint dataOutputCounter = 0;
  uint readyInputCounter = 0;

	for (uint i = 0; i < NUM_COMPONENTS; i++) {
		if ((i % COMPONENTS_PER_TILE) < CORES_PER_TILE) {
			// This is a core
      cores[coreIndex]->clock(clock);
      cores[coreIndex]->fastClock(fastClock);

			for (uint j = 0; j < CORE_INPUT_PORTS; j++) {
				uint index = dataInputCounter++;  // Position in network's array

				cores[coreIndex]->iData[j](iDataLocal[index]);
				network.oData[index](iDataLocal[index]);
			}

			for (uint j = 0; j < CORE_INPUT_CHANNELS; j++) {
			  uint index = readyInputCounter++;  // Position in network's array

			  // The ready signals are shared between all networks, and have already
			  // been connected to the cores.
//        cores[coreIndex]->oReadyData[j](readyDataFromComps[i][j]);
        network.iReady[index](oReadyData[i][j]);
			}

			for (uint j = 0; j < CORE_OUTPUT_PORTS; j++) {
				uint index = dataOutputCounter++;  // Position in network's array

				cores[coreIndex]->oData[j](oDataLocal[index]);
				network.iData[index](oDataLocal[index]);
			}

			coreIndex++;
		} else {
			uint indexInput = dataInputCounter++;  // Position in network's array
			memories[memoryIndex]->iData(iDataLocal[indexInput]);
			network.oData[indexInput](iDataLocal[indexInput]);

			uint indexReady = readyInputCounter++;  // Position in network's array
			memories[memoryIndex]->oReadyForData(oReadyData[i][0]);
	    network.iReady[indexReady](oReadyData[i][0]);

			uint indexOutput = dataOutputCounter++;  // Position in network's array
			memories[memoryIndex]->oData(oDataLocal[indexOutput]);
			network.iData[indexOutput](oDataLocal[indexOutput]);

			memoryIndex++;
		}
	}
  
  for (uint j=0; j<NUM_TILES; j++) {
    mhl[j]->clock(clock);
    mhl[j]->iDataFromBM(responseFromBM[j]);
    backgroundMemory.oData[j](responseFromBM[j]);
    mhl[j]->oRequestToBM(requestToBM[j]);
    backgroundMemory.iData[j](requestToBM[j]);

 	  for (uint i = 0; i < MEMS_PER_TILE; i++) {
			int currIndex = j * MEMS_PER_TILE + i;
			int prevIndex = j * MEMS_PER_TILE + ((i + MEMS_PER_TILE - 1) % MEMS_PER_TILE);
			int nextIndex = j * MEMS_PER_TILE + ((i + 1) % MEMS_PER_TILE);
		
			MemoryBank* m = memories[currIndex];

			m->iClock(clock);
				
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
    backgroundMemory("background_memory", NUM_COMPONENTS, NUM_TILES),
    network("network"),
    clock("clock", 1, SC_NS, 0.5),
    fastClock("fast_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.25),
    slowClock("slow_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.75) {

  makeSignals();
  makeComponents();
  wireUp();

}

Chip::~Chip() {
	for (uint i = 0; i < cores.size();    i++) delete cores[i];
	for (uint i = 0; i < memories.size(); i++) delete memories[i];
	for (uint i = 0; i < mhl.size();      i++) delete mhl[i];
	for (uint i = 0; i < networks.size(); i++) delete networks[i];
}
