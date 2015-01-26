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
	cores[component.globalCoreNumber()]->storeData(instructions);
}

void Chip::storeData(const DataBlock& data) {
  if (data.component() == ComponentID(2,0,0))
    backgroundMemory.storeData(data.payload(), data.position(), data.readOnly());
  else if (data.component().isCore()) {
    assert(data.component().globalCoreNumber() < cores.size());
    cores[data.component().globalCoreNumber()]->storeData(data.payload(), data.position());
  }
  else if (data.component().isMemory()) {
    assert(data.component().globalMemoryNumber() < memories.size());
    memories[data.component().globalMemoryNumber()]->storeData(data.payload(),data.position());
  }
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
		memories[component.globalMemoryNumber()]->print(start, end);
	else
		backgroundMemory.print(start, end);
}

Word Chip::readWord(const ComponentID& component, MemoryAddr addr) {
	if (component.isMemory()) {
	  assert(component.globalMemoryNumber() < memories.size());
	  return memories[component.globalMemoryNumber()]->readWord(addr);
	}
	else
		return backgroundMemory.readWord(addr);
}

Word Chip::readByte(const ComponentID& component, MemoryAddr addr) {
	if (component.isMemory()) {
    assert(component.globalMemoryNumber() < memories.size());
	  return memories[component.globalMemoryNumber()]->readByte(addr);
	}
	else
		return backgroundMemory.readByte(addr);
}

void Chip::writeWord(const ComponentID& component, MemoryAddr addr, Word data) {
	if (component.isMemory()) {
    assert(component.globalMemoryNumber() < memories.size());
	  return memories[component.globalMemoryNumber()]->writeWord(addr, data);
	}
	else
		return backgroundMemory.writeWord(addr, data);
}

void Chip::writeByte(const ComponentID& component, MemoryAddr addr, Word data) {
	if (component.isMemory()) {
    assert(component.globalMemoryNumber() < memories.size());
	  return memories[component.globalMemoryNumber()]->writeByte(addr, data);
	}
	else
		return backgroundMemory.writeByte(addr, data);
}

int Chip::readRegister(const ComponentID& component, RegisterIndex reg) const {
  assert(component.globalCoreNumber() < cores.size());

	return cores[component.globalCoreNumber()]->readRegDebug(reg);
}

MemoryAddr Chip::getInstIndex(const ComponentID& component) const {
  assert(component.globalCoreNumber() < cores.size());

	return cores[component.globalCoreNumber()]->getInstIndex();
}

bool Chip::readPredicate(const ComponentID& component) const {
  assert(component.globalCoreNumber() < cores.size());

	return cores[component.globalCoreNumber()]->readPredReg();
}

bool Chip::isIdle() const {
  return Stalls::stalledComponents() == NUM_COMPONENTS;
}

void Chip::makeSignals() {
  int numOutputs = TOTAL_OUTPUT_PORTS;
  int numInputs  = TOTAL_INPUT_PORTS;

  // Each of the global networks (data, credit, request, response) need to
  // connect to every tile with at least one input, output and ready signal.
  // Networks within a tile only need to exist on the compute tiles.

  oDataLocal.init(numOutputs);
  iDataLocal.init(numInputs);
  oReadyData.init(NUM_TILES);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      TileID tile(col,row);
      TileIndex index = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        oReadyData[index].init(COMPONENTS_PER_TILE);

        for (uint comp=0; comp<COMPONENTS_PER_TILE; comp++) {
          // Cores have a different number of input buffers/flow control signals
          // than memory banks.
          if (comp < CORES_PER_TILE)
            oReadyData[index][comp].init(CORE_INPUT_CHANNELS);
          else
            oReadyData[index][comp].init(1);
        }
      }
      else {
        oReadyData[index].init(1,1);
      }
    }
  }

  oDataGlobal.init(NUM_TILES, CORES_PER_TILE);
  iDataGlobal.init(NUM_TILES, CORES_PER_TILE);

  oCredit.init(NUM_TILES, CORES_PER_TILE);
  iCredit.init(NUM_TILES, CORES_PER_TILE);
  oReadyCredit.init(NUM_TILES, CORES_PER_TILE);

  requestFromBanks.init(NUM_COMPUTE_TILES, MEMS_PER_TILE);
  requestToBanks.init(NUM_COMPUTE_TILES);       // Broadcast within each tile
  targetBank.init(NUM_COMPUTE_TILES);           // Broadcast within each tile
  readyRequestToMHL.init(NUM_TILES);
  requestToMHL.init(NUM_TILES);
  requestFromMHL.init(NUM_TILES);
  requestToBM.init(NUM_COMPUTE_TILES);

  responseFromBanks.init(NUM_COMPUTE_TILES, MEMS_PER_TILE);
  responseToBanks.init(NUM_COMPUTE_TILES);      // Broadcast within each tile
  responseToMHL.init(NUM_TILES);
  readyResponseToMHL.init(NUM_TILES);
  responseFromMHL.init(NUM_TILES);
  responseFromBM.init(NUM_TILES);

	ringStrobe.init(NUM_MEMORIES);
	ringRequest.init(NUM_MEMORIES);
	ringAcknowledge.init(NUM_MEMORIES);
}

void Chip::makeComponents() {

  // Iterate from 1 to MAX, rather than 0 to MAX-1, to allow a halo of I/O
  // tiles around the edge.
  for (uint col = 1; col <= COMPUTE_TILE_COLUMNS; col++) {
    for (uint row = 1; row <= COMPUTE_TILE_ROWS; row++) {
      TileID tile(col, row);
      std::stringstream namebuilder;
      std::string name;

      // Initialise the cores of this tile
      for (uint core = 0; core < CORES_PER_TILE; core++) {
        ComponentID coreID(tile, core);

        namebuilder << "core_" << coreID.getNameString();
        namebuilder >> name;
        namebuilder.clear();

        Core* c = new Core(name.c_str(), coreID, network.getLocalNetwork(coreID));

        cores.push_back(c);
      }
      
      // Initialise the memories of this tile
      for (uint mem = 0; mem < MEMS_PER_TILE; mem++) {
        ComponentID memoryID(tile, CORES_PER_TILE + mem);

        namebuilder << "memory_" << memoryID.getNameString();
        namebuilder >> name;
        namebuilder.clear();

        MemoryBank* m = new MemoryBank(name.c_str(), memoryID, mem);
        m->setLocalNetwork(network.getLocalNetwork(memoryID));

        memories.push_back(m);
      }
      
   	  namebuilder << "mhl_" << tile.getNameString();
    	namebuilder >> name;
    	namebuilder.clear();

    	MissHandlingLogic* m = new MissHandlingLogic(name.c_str(), ComponentID(tile,0));
    	mhl.push_back(m);
      
    }
  }
}

void Chip::wireUp() {
  network.clock(clock);
  network.fastClock(fastClock);
  network.slowClock(slowClock);

	backgroundMemory.iClock(clock);

	// Global data network - connects cores to cores.
  dataNet.clock(clock);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      const TileID tile(col,row);
      const TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        for (uint core=0; core<CORES_PER_TILE; core++) {
          CoreIndex coreIndex = ComponentID(tile,core).globalCoreNumber();

          cores[coreIndex]->iDataGlobal(iDataGlobal[tileIndex][core]);
          dataNet.oData[tileIndex][core](iDataGlobal[tileIndex][core]);

          cores[coreIndex]->oDataGlobal(oDataGlobal[tileIndex][core]);
          dataNet.iData[tileIndex][core](oDataGlobal[tileIndex][core]);

          for (uint ch=0; ch<CORE_INPUT_CHANNELS; ch++) {
            cores[coreIndex]->oReadyData[ch](oReadyData[tileIndex][core][ch]);
            dataNet.iReady[tileIndex][core][ch](oReadyData[tileIndex][core][ch]);
          }
        }
      }
      else {
        dataNet.oData[tileIndex][0](iDataGlobal[tileIndex][0]);
        dataNet.iData[tileIndex][0](oDataGlobal[tileIndex][0]);
        dataNet.iReady[tileIndex][0][0](oReadyData[tileIndex][0][0]);
      }
    }
  }

  // Global credit network - connects cores to cores.
  creditNet.clock(clock);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      const TileID tile(col,row);
      const TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        for (uint core=0; core<CORES_PER_TILE; core++) {
          CoreIndex coreIndex = ComponentID(tile,core).globalCoreNumber();

          cores[coreIndex]->iCredit(iCredit[tileIndex][core]);
          creditNet.oData[tileIndex][core](iCredit[tileIndex][core]);

          cores[coreIndex]->oCredit(oCredit[tileIndex][core]);
          creditNet.iData[tileIndex][core](oCredit[tileIndex][core]);

          cores[coreIndex]->oReadyCredit(oReadyCredit[tileIndex][core]);
          creditNet.iReady[tileIndex][core][0](oReadyCredit[tileIndex][core]);
        }
      }
      else {
        creditNet.oData[tileIndex][0](iCredit[tileIndex][0]);
        creditNet.iData[tileIndex][0](oCredit[tileIndex][0]);
        creditNet.iReady[tileIndex][0][0](oReadyCredit[tileIndex][0]);
      }
    }
  }

  // Global request network - connects memories to memories.
  requestNet.clock(clock);
  requestNet.oMainMemoryRequest(requestToMainMemory);
//  backgroundMemory.iRequest(requestToMainMemory);
  requestNet.iMainMemoryReady(mainMemoryReadyRequest);
//  backgroundMemory.oReadyRequest(mainMemoryReadyRequest);
  requestNet.iMainMemoryRequest(requestFromMainMemory);
//  backgroundMemory.oRequest(requestFromMainMemory);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      const TileID tile(col,row);
      const TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        const TileIndex computeTileIndex = tile.computeTileIndex();

        for (uint bank=0; bank<MEMS_PER_TILE; bank++) {
          MemoryIndex memIndex = ComponentID(tile,bank+CORES_PER_TILE).globalMemoryNumber();

          memories[memIndex]->oRequest(requestFromBanks[computeTileIndex][bank]);
          mhl[computeTileIndex]->iRequestFromBanks[bank](requestFromBanks[computeTileIndex][bank]);

          memories[memIndex]->iRequest(requestToBanks[computeTileIndex]);
          memories[memIndex]->iTargetBank(targetBank[computeTileIndex]);
        }

        mhl[computeTileIndex]->oRequestToBanks(requestToBanks[computeTileIndex]);
        mhl[computeTileIndex]->oTargetBank(targetBank[computeTileIndex]);

        mhl[computeTileIndex]->oRequestToNetwork(requestFromMHL[tileIndex]);
        requestNet.iData[tileIndex][0](requestFromMHL[tileIndex]);

        mhl[computeTileIndex]->iRequestFromNetwork(requestToMHL[tileIndex]);
        requestNet.oData[tileIndex][0](requestToMHL[tileIndex]);

        mhl[computeTileIndex]->oReadyForRequest(readyRequestToMHL[tileIndex]);
        requestNet.iReady[tileIndex][0][0](readyRequestToMHL[tileIndex]);
      }
      else {
        requestNet.oData[tileIndex][0](requestToMHL[tileIndex]);
        requestNet.iData[tileIndex][0](requestFromMHL[tileIndex]);
        requestNet.iReady[tileIndex][0][0](readyRequestToMHL[tileIndex]);
      }
    }
  }

  // Global response network - connects memories to memories.
  responseNet.clock(clock);
  responseNet.oMainMemoryResponse(responseToMainMemory);
//  backgroundMemory.iResponse(responseToMainMemory);
  responseNet.iMainMemoryReady(mainMemoryReadyResponse);
//  backgroundMemory.oReadyResponse(mainMemoryReadyResponse);
  responseNet.iMainMemoryResponse(responseFromMainMemory);
//  backgroundMemory.oResponse(responseFromMainMemory);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      const TileID tile(col,row);
      const TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        const TileIndex computeTileIndex = tile.computeTileIndex();

        for (uint bank=0; bank<MEMS_PER_TILE; bank++) {
          MemoryIndex memIndex = ComponentID(tile,bank+CORES_PER_TILE).globalMemoryNumber();

          memories[memIndex]->oResponse(responseFromBanks[computeTileIndex][bank]);
          mhl[computeTileIndex]->iDataFromBanks[bank](responseFromBanks[computeTileIndex][bank]);

          memories[memIndex]->iResponse(responseToBanks[computeTileIndex]);
        }

        mhl[computeTileIndex]->oDataToBanks(responseToBanks[computeTileIndex]);

        mhl[computeTileIndex]->oResponseToNetwork(responseFromMHL[tileIndex]);
        responseNet.iData[tileIndex][0](responseFromMHL[tileIndex]);

        mhl[computeTileIndex]->iResponseFromNetwork(responseToMHL[tileIndex]);
        responseNet.oData[tileIndex][0](responseToMHL[tileIndex]);

        mhl[computeTileIndex]->oReadyForResponse(readyResponseToMHL[tileIndex]);
        responseNet.iReady[tileIndex][0][0](readyResponseToMHL[tileIndex]);
      }
      else {
        responseNet.oData[tileIndex][0](responseToMHL[tileIndex]);
        responseNet.iData[tileIndex][0](responseFromMHL[tileIndex]);
        responseNet.iReady[tileIndex][0][0](readyResponseToMHL[tileIndex]);
      }
    }
  }

// Connect cores and memories to the local interconnect

	uint dataInputCounter = 0;
	uint dataOutputCounter = 0;
  uint readyInputCounter = 0;

  for (uint col = 0; col < TOTAL_TILE_COLUMNS; col++) {
    for (uint row = 0; row < TOTAL_TILE_ROWS; row++) {
      TileID tile(col,row);
      TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        for (uint pos = 0; pos < COMPONENTS_PER_TILE; pos++) {
          ComponentID component(tile, pos);
          if (component.isCore()) {
            uint coreIndex = component.globalCoreNumber();
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
      //        cores[coreIndex]->oReadyData[j](oReadyData[tileIndex][pos][j]);
              network.iReady[index](oReadyData[tileIndex][pos][j]);
            }

            for (uint j = 0; j < CORE_OUTPUT_PORTS; j++) {
              uint index = dataOutputCounter++;  // Position in network's array

              cores[coreIndex]->oData[j](oDataLocal[index]);
              network.iData[index](oDataLocal[index]);
            }
          } // end isCore
          else {
            uint memoryIndex = component.globalMemoryNumber();

            uint indexInput = dataInputCounter++;  // Position in network's array
            memories[memoryIndex]->iData(iDataLocal[indexInput]);
            network.oData[indexInput](iDataLocal[indexInput]);

            uint indexReady = readyInputCounter++;  // Position in network's array
            memories[memoryIndex]->oReadyForData(oReadyData[tileIndex][pos][0]);
            network.iReady[indexReady](oReadyData[tileIndex][pos][0]);

            uint indexOutput = dataOutputCounter++;  // Position in network's array
            memories[memoryIndex]->oData(oDataLocal[indexOutput]);
            network.iData[indexOutput](oDataLocal[indexOutput]);
          } // end isMemory
        } // end loop through components in tile
      } // end isComputeTile
    } // end tile row loop
  } // end tile column loop
  
  for (uint j=0; j<NUM_COMPUTE_TILES; j++) {
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
    backgroundMemory("background_memory", ComponentID(2,0,0), NUM_COMPUTE_TILES),
    dataNet("data_net"),
    creditNet("credit_net"),
    requestNet("request_net"),
    responseNet("response_net"),
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
}
