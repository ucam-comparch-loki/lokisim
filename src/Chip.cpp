/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "Chip.h"
#include "TileComponents/Memory/MissHandlingLogic.h"
#include "Utility/Instrumentation/Stalls.h"
#include "Utility/StartUp/DataBlock.h"

using Instrumentation::Stalls;


void Chip::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
  cores[component.globalCoreNumber()]->storeData(instructions);
}

void Chip::storeData(const DataBlock& data) {
  if (data.component() == ComponentID(2,0,0))
    mainMemory.storeData(data.payload(), data.position(), data.readOnly());
  else if (data.component().isCore()) {
    assert(data.component().globalCoreNumber() < cores.size());
    cores[data.component().globalCoreNumber()]->storeData(data.payload(), data.position());
  }
  else if (data.component().isMemory()) {
    LOKI_ERROR << "storing directly to memory banks is disabled since it cannot be known "
        "which bank the core will access." << endl;
//    assert(data.component().globalMemoryNumber() < memories.size());
//    memories[data.component().globalMemoryNumber()]->storeData(data.payload(),data.position());
  }
}

void Chip::print(const ComponentID& component, MemoryAddr start, MemoryAddr end) {
  if (component.isMemory() && !MAGIC_MEMORY)
    memories[component.globalMemoryNumber()]->print(start, end);
  else
    mainMemory.print(start, end);
}

Word Chip::readWordInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.isMemory() && !MAGIC_MEMORY) {
    assert(component.globalMemoryNumber() < memories.size());
    return memories[component.globalMemoryNumber()]->readWordDebug(addr);
  }
  else
    return mainMemory.readWord(addr);
}

Word Chip::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  if (component.isMemory() && !MAGIC_MEMORY) {
    assert(component.globalMemoryNumber() < memories.size());
    return memories[component.globalMemoryNumber()]->readByteDebug(addr);
  }
  else
    return mainMemory.readByte(addr);
}

void Chip::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.isMemory() && !MAGIC_MEMORY) {
    assert(component.globalMemoryNumber() < memories.size());
    memories[component.globalMemoryNumber()]->writeWordDebug(addr, data);
  }
  else
    mainMemory.writeWord(addr, data.toUInt());
}

void Chip::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (component.isMemory() && !MAGIC_MEMORY) {
    assert(component.globalMemoryNumber() < memories.size());
    memories[component.globalMemoryNumber()]->writeByteDebug(addr, data);
  }
  else
    mainMemory.writeByte(addr, data.toUInt());
}

int Chip::readRegisterInternal(const ComponentID& component, RegisterIndex reg) const {
  assert(component.globalCoreNumber() < cores.size());

  return cores[component.globalCoreNumber()]->readRegDebug(reg);
}

bool Chip::readPredicateInternal(const ComponentID& component) const {
  assert(component.globalCoreNumber() < cores.size());

  return cores[component.globalCoreNumber()]->readPredReg();
}

void Chip::networkSendDataInternal(const NetworkData& flit) {
  assert(flit.channelID().isCore());
  cores[flit.channelID().component.globalCoreNumber()]->deliverDataInternal(flit);
}

void Chip::networkSendCreditInternal(const NetworkCredit& flit) {
  assert(flit.channelID().isCore());
  cores[flit.channelID().component.globalCoreNumber()]->deliverCreditInternal(flit);
}


void Chip::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload) {
  magicMemory.operate(opcode, address, returnChannel, payload);
}

bool Chip::isIdle() const {
  return Stalls::stalledComponents() == NUM_COMPONENTS;
}

void Chip::makeSignals() {
  // Each of the global networks (data, credit, request, response) need to
  // connect to every tile with at least one input, output and ready signal.
  // Networks within a tile only need to exist on the compute tiles.

  oDataLocal.init(TOTAL_OUTPUT_PORTS);
  iDataLocal.init(TOTAL_INPUT_PORTS);

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

  // Would prefer this (it's much easier to read), but the global data network
  // doesn't include ready signals for memories, so the two don't quite match.
//  oReadyData.init(dataNet.iReady);

  oDataGlobal.init(dataNet.iData);
  iDataGlobal.init(dataNet.oData);

  oCredit.init(creditNet.iData);
  iCredit.init(creditNet.oData);
  oReadyCredit.init(creditNet.iReady);

  requestFromBanks.init(NUM_COMPUTE_TILES, MEMS_PER_TILE);
  requestToBanks.init(NUM_COMPUTE_TILES);       // Broadcast within each tile
  requestTarget.init(NUM_COMPUTE_TILES);        // Broadcast within each tile
  requestToBM.init(NUM_COMPUTE_TILES);
  requestToMHL.init(requestNet.iData);
  readyRequestToMHL.init(requestNet.iReady);
  requestFromMHL.init(requestNet.oData);

  responseFromBanks.init(NUM_COMPUTE_TILES, MEMS_PER_TILE);
  responseToBanks.init(NUM_COMPUTE_TILES);      // Broadcast within each tile
  responseTarget.init(NUM_COMPUTE_TILES);       // Broadcast within each tile
  responseFromBM.init(NUM_COMPUTE_TILES);
  responseToMHL.init(responseNet.iData);
  readyResponseToMHL.init(responseNet.iReady);
  responseFromMHL.init(responseNet.oData);

  l2RequestClaim.init(NUM_COMPUTE_TILES, MEMS_PER_TILE);
  l2RequestClaimed.init(NUM_COMPUTE_TILES);
}

void Chip::makeComponents() {
  cores.assign(NUM_CORES, NULL);
  memories.assign(NUM_MEMORIES, NULL);
  mhl.assign(NUM_COMPUTE_TILES, NULL);

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

        cores[coreID.globalCoreNumber()] = c;
      }
      
      // Initialise the memories of this tile
      for (uint mem = 0; mem < MEMS_PER_TILE; mem++) {
        ComponentID memoryID(tile, CORES_PER_TILE + mem);

        namebuilder << "memory_" << memoryID.getNameString();
        namebuilder >> name;
        namebuilder.clear();

        MemoryBank* m = new MemoryBank(name.c_str(), memoryID, mem);
        m->setLocalNetwork(network.getLocalNetwork(memoryID));
        m->setBackgroundMemory(&mainMemory);

        memories[memoryID.globalMemoryNumber()] = m;
      }
      
       namebuilder << "mhl_" << tile.getNameString();
      namebuilder >> name;
      namebuilder.clear();

      MissHandlingLogic* m = new MissHandlingLogic(name.c_str(), ComponentID(tile,0));
      mhl[tile.computeTileIndex()] = m;
      
    }
  }
}

void Chip::wireUp() {
  network.clock(clock);
  network.fastClock(fastClock);
  network.slowClock(slowClock);

  mainMemory.iClock(clock);

  // Global data network - connects cores to cores.
  dataNet.clock(clock);
  dataNet.oData(iDataGlobal);
  dataNet.iData(oDataGlobal);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      const TileID tile(col,row);
      const TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        for (uint core=0; core<CORES_PER_TILE; core++) {
          CoreIndex coreIndex = ComponentID(tile,core).globalCoreNumber();

          cores[coreIndex]->iDataGlobal(iDataGlobal[tileIndex][core]);
          cores[coreIndex]->oDataGlobal(oDataGlobal[tileIndex][core]);

          for (uint ch=0; ch<CORE_INPUT_CHANNELS; ch++) {
            cores[coreIndex]->oReadyData[ch](oReadyData[tileIndex][core][ch]);
            dataNet.iReady[tileIndex][core][ch](oReadyData[tileIndex][core][ch]);
          }
        }
      }
      else {
        dataNet.iReady[tileIndex][0][0](oReadyData[tileIndex][0][0]);
      }
    }
  }

  // Global credit network - connects cores to cores.
  creditNet.clock(clock);
  creditNet.oData(iCredit);
  creditNet.iData(oCredit);
  creditNet.iReady(oReadyCredit);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      const TileID tile(col,row);
      const TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        for (uint core=0; core<CORES_PER_TILE; core++) {
          CoreIndex coreIndex = ComponentID(tile,core).globalCoreNumber();

          cores[coreIndex]->iCredit(iCredit[tileIndex][core]);
          cores[coreIndex]->oCredit(oCredit[tileIndex][core]);
          cores[coreIndex]->oReadyCredit(oReadyCredit[tileIndex][core][0]);
        }
      }
    }
  }

  // Global request network - connects memories to memories.
  requestNet.clock(clock);
  requestNet.oData(requestToMHL);
  requestNet.iData(requestFromMHL);
  requestNet.iReady(readyRequestToMHL);
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
          memories[memIndex]->iRequestTarget(requestTarget[computeTileIndex]);

          memories[memIndex]->iRequestClaimed(l2RequestClaimed[computeTileIndex]);
          memories[memIndex]->oClaimRequest(l2RequestClaim[computeTileIndex][bank]);
          mhl[computeTileIndex]->iClaimRequest[bank](l2RequestClaim[computeTileIndex][bank]);
        }

        mhl[computeTileIndex]->oRequestToBanks(requestToBanks[computeTileIndex]);
        mhl[computeTileIndex]->oRequestTarget(requestTarget[computeTileIndex]);
        mhl[computeTileIndex]->oRequestClaimed(l2RequestClaimed[computeTileIndex]);

        mhl[computeTileIndex]->oRequestToNetwork(requestFromMHL[tileIndex][0]);
        mhl[computeTileIndex]->iRequestFromNetwork(requestToMHL[tileIndex][0]);
        mhl[computeTileIndex]->oReadyForRequest(readyRequestToMHL[tileIndex][0][0]);
      }
    }
  }

  // Global response network - connects memories to memories.
  responseNet.clock(clock);
  responseNet.oData(responseToMHL);
  responseNet.iData(responseFromMHL);
  responseNet.iReady(readyResponseToMHL);
  for (uint col=0; col<TOTAL_TILE_COLUMNS; col++) {
    for (uint row=0; row<TOTAL_TILE_ROWS; row++) {
      const TileID tile(col,row);
      const TileIndex tileIndex = tile.overallTileIndex();

      if (tile.isComputeTile()) {
        const TileIndex computeTileIndex = tile.computeTileIndex();

        for (uint bank=0; bank<MEMS_PER_TILE; bank++) {
          MemoryIndex memIndex = ComponentID(tile,bank+CORES_PER_TILE).globalMemoryNumber();

          memories[memIndex]->oResponse(responseFromBanks[computeTileIndex][bank]);
          mhl[computeTileIndex]->iResponseFromBanks[bank](responseFromBanks[computeTileIndex][bank]);

          memories[memIndex]->iResponse(responseToBanks[computeTileIndex]);
          memories[memIndex]->iResponseTarget(responseTarget[computeTileIndex]);
        }

        mhl[computeTileIndex]->oResponseToBanks(responseToBanks[computeTileIndex]);
        mhl[computeTileIndex]->oResponseTarget(responseTarget[computeTileIndex]);

        mhl[computeTileIndex]->oResponseToNetwork(responseFromMHL[tileIndex][0]);
        mhl[computeTileIndex]->iResponseFromNetwork(responseToMHL[tileIndex][0]);
        mhl[computeTileIndex]->oReadyForResponse(readyResponseToMHL[tileIndex][0][0]);
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

            memories[memoryIndex]->iClock(clock);

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
  
  // Miss handling logic.
  for (uint j=0; j<NUM_COMPUTE_TILES; j++) {
    mhl[j]->clock(clock);
    mhl[j]->iResponseFromBM(responseFromBM[j]);
    mainMemory.oData[j](responseFromBM[j]);
    mhl[j]->oRequestToBM(requestToBM[j]);
    mainMemory.iData[j](requestToBM[j]);
  }
}

Chip::Chip(const sc_module_name& name, const ComponentID& ID) :
    Component(name),
    mainMemory("main_memory", ComponentID(2,0,0), NUM_COMPUTE_TILES),
    magicMemory("magic_memory", mainMemory),
    dataNet("data_net"),
    creditNet("credit_net"),
    requestNet("request_net"),
    responseNet("response_net"),
    network("network"),
    clock("clock", 1, sc_core::SC_NS, 0.5),
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
