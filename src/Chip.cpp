/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <sstream>

#include "Chip.h"
#include "Tile/Tile.h"
#include "Tile/ComputeTile.h"
#include "Tile/EmptyTile.h"
#include "Tile/MemoryControllerTile.h"
#include "Utility/Instrumentation/Stalls.h"
#include "Utility/StartUp/DataBlock.h"

using Instrumentation::Stalls;


void Chip::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
  getTile(component.tile)->storeInstructions(instructions, component);
}

void Chip::storeData(const DataBlock& data) {
  if (data.component() == ComponentID(2,0,0))
    mainMemory.storeData(data.payload(), data.position(), data.readOnly());
  else
    getTile(data.component().tile)->storeData(data);
}

void Chip::print(const ComponentID& component, MemoryAddr start, MemoryAddr end) {
  if (MAGIC_MEMORY)
    mainMemory.print(start, end);
  else
    getTile(component.tile)->print(component, start, end);
}

Word Chip::readWordInternal(const ComponentID& component, MemoryAddr addr) {
  if (MAGIC_MEMORY)
    return mainMemory.readWord(addr);
  else
    return getTile(component.tile)->readWordInternal(component, addr);
}

Word Chip::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  if (MAGIC_MEMORY)
    return mainMemory.readByte(addr);
  else
    return getTile(component.tile)->readByteInternal(component, addr);
}

void Chip::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (MAGIC_MEMORY)
    mainMemory.writeWord(addr, data.toUInt());
  else
    getTile(component.tile)->writeWordInternal(component, addr, data);
}

void Chip::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (MAGIC_MEMORY)
    mainMemory.writeByte(addr, data.toUInt());
  else
    getTile(component.tile)->writeByteInternal(component, addr, data);
}

int Chip::readRegisterInternal(const ComponentID& component, RegisterIndex reg) const {
  return getTile(component.tile)->readRegisterInternal(component, reg);
}

bool Chip::readPredicateInternal(const ComponentID& component) const {
  return getTile(component.tile)->readPredicateInternal(component);
}

void Chip::networkSendDataInternal(const NetworkData& flit) {
  getTile(flit.channelID().component.tile)->networkSendDataInternal(flit);
}

void Chip::networkSendCreditInternal(const NetworkCredit& flit) {
  getTile(flit.channelID().component.tile)->networkSendCreditInternal(flit);
}

MemoryAddr Chip::getAddressTranslation(TileID tile, MemoryAddr address) const {
  // If the given tile is a memory controller, no more transformation will be
  // performed, so return the address as-is. Otherwise, query the tile's
  // directory to find out what transformation is necessary.
  if (tile.isComputeTile()) {
    Tile* t = getTile(tile);
    return static_cast<ComputeTile*>(t)->getAddressTranslation(address);
  }
  else {
    return address;
  }
}

bool Chip::backedByMainMemory(TileID tile, MemoryAddr address) const {
  // If the given tile is a memory controller, the result is trivial.
  // Otherwise, query the tile's directory to find details of the next level of
  // memory hierarchy.
  if (memoryControllerPositions.find(tile) != memoryControllerPositions.end()) {
    return true;
  }
  else if (tile.isComputeTile()) {
    Tile* t = getTile(tile);
    return static_cast<ComputeTile*>(t)->backedByMainMemory(address);
  }
  else {
    return false;
  }
}


void Chip::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload) {
  magicMemory.operate(opcode, address, returnChannel, payload);
}

bool Chip::isIdle() const {
  return Stalls::stalledComponents() == NUM_COMPONENTS;
}

TileID Chip::nearestMemoryController(TileID tile) const {
  assert(memoryControllerPositions.size() > 0);

  TileID closest(0,0);
  uint bestDistance = UINT_MAX;

  for (std::set<TileID>::iterator it=memoryControllerPositions.begin(); it!=memoryControllerPositions.end(); it++) {
    TileID mc = *it;
    uint distance = abs(mc.x - tile.x) + abs(mc.y - tile.y);

    if (distance < bestDistance) {
      bestDistance = distance;
      closest = mc;
    }
  }

  return closest;
}

Tile* Chip::getTile(TileID tile) const {
  return tiles[tile.x][tile.y];
}

const std::set<TileID> Chip::getMemoryControllerPositions() const {
  std::set<TileID> positions;

  // Memory controllers go above and below the east-most and west-most columns
  // of compute tiles. Put them in a set in case there are duplicates.
  positions.insert(TileID(1,0));
  positions.insert(TileID(COMPUTE_TILE_COLUMNS, 0));
  positions.insert(TileID(1, COMPUTE_TILE_ROWS+1));
  positions.insert(TileID(COMPUTE_TILE_COLUMNS, COMPUTE_TILE_ROWS+1));

  if (positions.size() < MAIN_MEMORY_BANDWIDTH)
    LOKI_WARN << "Unable to use " << MAIN_MEMORY_BANDWIDTH << " words/cycle of memory bandwidth with only " << positions.size() << " memory controllers." << endl;

  return positions;
}

void Chip::makeSignals() {
  iData.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oData.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  iDataReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oDataReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);

  iCredit.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oCredit.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  iCreditReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oCreditReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);

  iRequest.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oRequest.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  iRequestReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oRequestReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);

  iResponse.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oResponse.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  iResponseReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);
  oResponseReady.init(TOTAL_TILE_COLUMNS, TOTAL_TILE_ROWS);

  requestToMainMemory.init(memoryControllerPositions.size());
  responseFromMainMemory.init(memoryControllerPositions.size());
}

void Chip::makeComponents() {
  int memoryControllersMade = 0;

  for (uint col = 0; col < TOTAL_TILE_COLUMNS; col++) {
    tiles.push_back(vector<Tile*>());

    for (uint row = 0; row < TOTAL_TILE_ROWS; row++) {
      ComponentID tileID(col, row, 0);
      std::stringstream name;
      name << "tile_" << col << "_" << row;

      Tile* t;
      
      if (col > 0 && col <= COMPUTE_TILE_COLUMNS &&
          row > 0 && row <= COMPUTE_TILE_ROWS) {
        t = new ComputeTile(name.str().c_str(), tileID);

        // Some ComputeTile-specific connections.
        ((ComputeTile*)t)->fastClock(fastClock);
        ((ComputeTile*)t)->slowClock(slowClock);
      }
      else if (memoryControllerPositions.find(TileID(col,row)) != memoryControllerPositions.end()) {
        t = new MemoryControllerTile(name.str().c_str(), tileID);

        uint memoryPort = memoryControllersMade++;
        ((MemoryControllerTile*)t)->oRequestToMainMemory(requestToMainMemory[memoryPort]);
        ((MemoryControllerTile*)t)->iResponseFromMainMemory(responseFromMainMemory[memoryPort]);
      }
      else {
        t = new EmptyTile(name.str().c_str(), tileID);
      }

      // Common interface for all tiles.
      t->clock(clock);
      t->iCredit(iCredit[col][row]);
      t->iCreditReady(iCreditReady[col][row]);
      t->iData(iData[col][row]);
      t->iDataReady(iDataReady[col][row]);
      t->iRequest(iRequest[col][row]);
      t->iRequestReady(iRequestReady[col][row]);
      t->iResponse(iResponse[col][row]);
      t->iResponseReady(iResponseReady[col][row]);
      t->oCredit(oCredit[col][row]);
      t->oCreditReady(oCreditReady[col][row]);
      t->oData(oData[col][row]);
      t->oDataReady(oDataReady[col][row]);
      t->oRequest(oRequest[col][row]);
      t->oRequestReady(oRequestReady[col][row]);
      t->oResponse(oResponse[col][row]);
      t->oResponseReady(oResponseReady[col][row]);

      tiles[col].push_back(t);
    }
  }
}

void Chip::wireUp() {

  // Main memory.
  mainMemory.iClock(clock);
  mainMemory.iData(requestToMainMemory);
  mainMemory.oData(responseFromMainMemory);

  // Global data network - connects cores to cores.
  dataNet.clock(clock);
  dataNet.oData(iData);
  dataNet.iData(oData);
  dataNet.oReady(iDataReady);
  dataNet.iReady(oDataReady);

  // Global credit network - connects cores to cores.
  creditNet.clock(clock);
  creditNet.oData(iCredit);
  creditNet.iData(oCredit);
  creditNet.oReady(iCreditReady);
  creditNet.iReady(oCreditReady);

  // Global request network - connects memories to memories.
  requestNet.clock(clock);
  requestNet.oData(iRequest);
  requestNet.iData(oRequest);
  requestNet.oReady(iRequestReady);
  requestNet.iReady(oRequestReady);

  // Global response network - connects memories to memories.
  responseNet.clock(clock);
  responseNet.oData(iResponse);
  responseNet.iData(oResponse);
  responseNet.oReady(iResponseReady);
  responseNet.iReady(oResponseReady);

}

Chip::Chip(const sc_module_name& name, const ComponentID& ID) :
    LokiComponent(name),
    memoryControllerPositions(getMemoryControllerPositions()),
    mainMemory("main_memory", ComponentID(0,0,0), memoryControllerPositions.size()),
    magicMemory("magic_memory", mainMemory),
    dataNet("data_net"),
    creditNet("credit_net"),
    requestNet("request_net"),
    responseNet("response_net"),
    clock("clock", 1, sc_core::SC_NS, 0.5),
    fastClock("fast_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.25),
    slowClock("slow_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.75) {

  makeSignals();
  makeComponents();
  wireUp();

}

Chip::~Chip() {
  for (uint i=0; i<tiles.size(); i++)
    for (uint j=0; j<tiles[i].size(); j++)
      delete tiles[i][j];
}
