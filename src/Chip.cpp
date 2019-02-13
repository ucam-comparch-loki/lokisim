/*
 * Chip.cpp
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <sstream>

#include "Chip.h"
#include "Tile/ComputeTile.h"
#include "Tile/EmptyTile.h"
#include "Tile/MemoryControllerTile.h"
#include "Utility/Instrumentation/Stalls.h"
#include "Utility/StartUp/DataBlock.h"

using Instrumentation::Stalls;

bool Chip::isComputeTile(TileID id) const {
  // All tiles except the borders are compute tiles.
  return (id.x > 0) && (id.x < tiles.size()-1)
      && (id.y > 0) && (id.y < tiles[0].size()-1);
}

uint Chip::overallTileIndex(TileID id) const {
  return (id.y * tiles.size()) + id.x;
}

uint Chip::computeTileIndex(TileID id) const {
  return ((id.y - 1) * (tiles.size() - 2)) + (id.x - 1);
}

bool Chip::isCore(ComponentID id) const {
  return getTile(id.tile).isCore(id);
}

bool Chip::isMemory(ComponentID id) const {
  return getTile(id.tile).isMemory(id);
}

uint Chip::globalComponentIndex(ComponentID id) const {
  // Assumes all compute tiles are the same.
  Tile& tile = getTile(id.tile);
  return computeTileIndex(id.tile) * tile.numComponents() +
         tile.componentIndex(id);
}

uint Chip::globalCoreIndex(ComponentID id) const {
  // Assumes all compute tiles are the same.
  Tile& tile = getTile(id.tile);
  return computeTileIndex(id.tile) * tile.numCores() +
         tile.coreIndex(id);
}

uint Chip::globalMemoryIndex(ComponentID id) const {
  // Assumes all compute tiles are the same.
  Tile& tile = getTile(id.tile);
  return computeTileIndex(id.tile) * tile.numMemories() +
         tile.memoryIndex(id);
}

bool Chip::isCore(ChannelID id) const {
  return id.multicast || isCore(id.component);
}

bool Chip::isMemory(ChannelID id) const {
  return !isCore(id);
}

void Chip::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
  getTile(component.tile).storeInstructions(instructions, component);
}

void Chip::storeData(const DataBlock& data) {
  if (data.component() == ComponentID(2,0,0))
    mainMemory.storeData(data.payload(), data.position(), data.readOnly());
  else
    getTile(data.component().tile).storeData(data);
}

void Chip::print(const ComponentID& component, MemoryAddr start, MemoryAddr end) {
  if (MAGIC_MEMORY)
    mainMemory.print(start, end);
  else
    getTile(component.tile).print(component, start, end);
}

Word Chip::readWordInternal(const ComponentID& component, MemoryAddr addr) {
  if (MAGIC_MEMORY)
    return mainMemory.readWord(addr, MEMORY_SCRATCHPAD);
  else
    return getTile(component.tile).readWordInternal(component, addr);
}

Word Chip::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  if (MAGIC_MEMORY)
    return mainMemory.readByte(addr, MEMORY_SCRATCHPAD);
  else
    return getTile(component.tile).readByteInternal(component, addr);
}

void Chip::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (MAGIC_MEMORY)
    mainMemory.writeWord(addr, data.toUInt(), MEMORY_SCRATCHPAD);
  else
    getTile(component.tile).writeWordInternal(component, addr, data);
}

void Chip::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (MAGIC_MEMORY)
    mainMemory.writeByte(addr, data.toUInt(), MEMORY_SCRATCHPAD);
  else
    getTile(component.tile).writeByteInternal(component, addr, data);
}

int Chip::readRegisterInternal(const ComponentID& component, RegisterIndex reg) const {
  return getTile(component.tile).readRegisterInternal(component, reg);
}

bool Chip::readPredicateInternal(const ComponentID& component) const {
  return getTile(component.tile).readPredicateInternal(component);
}

void Chip::networkSendDataInternal(const NetworkData& flit) {
  getTile(flit.channelID().component.tile).networkSendDataInternal(flit);
}

void Chip::networkSendCreditInternal(const NetworkCredit& flit) {
  getTile(flit.channelID().component.tile).networkSendCreditInternal(flit);
}

MemoryAddr Chip::getAddressTranslation(TileID tile, MemoryAddr address) const {
  // If the given tile is a memory controller, no more transformation will be
  // performed, so return the address as-is. Otherwise, query the tile's
  // directory to find out what transformation is necessary.
  if (isComputeTile(tile)) {
    Tile& t = getTile(tile);
    return static_cast<ComputeTile&>(t).getAddressTranslation(address);
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
  else if (isComputeTile(tile)) {
    Tile& t = getTile(tile);
    return static_cast<ComputeTile&>(t).backedByMainMemory(address);
  }
  else {
    return false;
  }
}


void Chip::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload) {
  magicMemory.operate(opcode, address, returnChannel, payload);
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

Tile& Chip::getTile(TileID tile) const {
  return tiles[tile.x][tile.y];
}

const std::set<TileID> Chip::getMemoryControllerPositions(const chip_parameters_t& params) const {
  std::set<TileID> positions;

  // Memory controllers go above and below the east-most and west-most columns
  // of compute tiles. Put them in a set in case there are duplicates.
  positions.insert(TileID(1,0));
  positions.insert(TileID(params.numComputeTiles.width, 0));
  positions.insert(TileID(1, params.numComputeTiles.height+1));
  positions.insert(TileID(params.numComputeTiles.width, params.numComputeTiles.height+1));

  loki_assert_with_message(positions.size() >= params.memory.bandwidth,
      "Unable to use %d words/cycle of memory bandwidth with only %d memory controllers",
      params.memory.bandwidth, positions.size());

  return positions;
}

void Chip::makeSignals(size2d_t allTiles) {
  iData.init("iData", allTiles.width, allTiles.height);
  oData.init("oData", allTiles.width, allTiles.height);
  iDataReady.init("iDataReady", allTiles.width, allTiles.height);
  oDataReady.init("oDataReady", allTiles.width, allTiles.height);

  iCredit.init("iCredit", allTiles.width, allTiles.height);
  oCredit.init("oCredit", allTiles.width, allTiles.height);
  iCreditReady.init("iCreditReady", allTiles.width, allTiles.height);
  oCreditReady.init("oCreditReady", allTiles.width, allTiles.height);

  iRequest.init("iRequest", allTiles.width, allTiles.height);
  oRequest.init("oRequest", allTiles.width, allTiles.height);
  iRequestReady.init("iRequestReady", allTiles.width, allTiles.height);
  oRequestReady.init("oRequestReady", allTiles.width, allTiles.height);

  iResponse.init("iResponse", allTiles.width, allTiles.height);
  oResponse.init("oResponse", allTiles.width, allTiles.height);
  iResponseReady.init("iResponseReady", allTiles.width, allTiles.height);
  oResponseReady.init("oResponseReady", allTiles.width, allTiles.height);

  requestToMainMemory.init("requestToMainMemory", memoryControllerPositions.size());
  responseFromMainMemory.init("responseFromMainMemory", memoryControllerPositions.size());
}

void Chip::makeComponents(const chip_parameters_t& params) {
  int memoryControllersMade = 0;

  tiles.init(params.allTiles().width);
  for (uint col = 0; col < params.allTiles().width; col++) {

    for (uint row = 0; row < params.allTiles().height; row++) {
      TileID tileID(col, row);
      std::stringstream name;
      name << "tile_" << col << "_" << row;

      Tile* t;
      
      if (col > 0 && col <= params.numComputeTiles.width &&
          row > 0 && row <= params.numComputeTiles.height) {
        t = new ComputeTile(name.str().c_str(), tileID, params.tile);

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

Chip::Chip(const sc_module_name& name, const chip_parameters_t& params) :
    LokiComponent(name),
    memoryControllerPositions(getMemoryControllerPositions(params)),
    mainMemory("main_memory", memoryControllerPositions.size(), params.memory),
    magicMemory("magic_memory", mainMemory),
    dataNet("data_net", params.allTiles(), params.router),
    creditNet("credit_net", params.allTiles(), params.router),
    requestNet("request_net", params.allTiles(), params.router),
    responseNet("response_net", params.allTiles(), params.router),
    clock("clock", 1, sc_core::SC_NS, 0.5),
    fastClock("fast_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.25),
    slowClock("slow_clock", sc_core::sc_time(1.0, sc_core::SC_NS), 0.75) {

  makeSignals(params.allTiles());
  makeComponents(params);
  wireUp();

}
