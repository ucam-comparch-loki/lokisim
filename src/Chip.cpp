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
#include "Tile/AcceleratorTile.h"
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
    return mainMemory.readWord(addr, MEMORY_SCRATCHPAD, true);
  else
    return getTile(component.tile).readWordInternal(component, addr);
}

Word Chip::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  if (MAGIC_MEMORY)
    return mainMemory.readByte(addr, MEMORY_SCRATCHPAD, true);
  else
    return getTile(component.tile).readByteInternal(component, addr);
}

void Chip::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (MAGIC_MEMORY)
    mainMemory.writeWord(addr, data.toUInt(), MEMORY_SCRATCHPAD, true);
  else
    getTile(component.tile).writeWordInternal(component, addr, data);
}

void Chip::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  if (MAGIC_MEMORY)
    mainMemory.writeByte(addr, data.toUInt(), MEMORY_SCRATCHPAD, true);
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

void Chip::makeComponents(const chip_parameters_t& params) {
  int memoryControllersMade = 0;

  tiles.init(params.allTiles().width);
  for (uint col = 0; col < params.allTiles().width; col++) {

    for (uint row = 0; row < params.allTiles().height; row++) {
      TileID tileID(col, row);
      std::stringstream name;
      name << "tile_" << col << "_" << row;

      Tile* t;
      
      if (col > 0 && col <= COMPUTE_TILE_COLUMNS &&
          row > 0 && row <= COMPUTE_TILE_ROWS) {

        if (ACCELERATORS_PER_TILE > 0)
          t = new AcceleratorTile(name.str().c_str(), tileID);
        else
          t = new ComputeTile(name.str().c_str(), tileID);

        // Some ComputeTile-specific connections.
        ((ComputeTile*)t)->fastClock(fastClock);
        ((ComputeTile*)t)->slowClock(slowClock);
      }
      else if (memoryControllerPositions.find(TileID(col,row)) != memoryControllerPositions.end()) {
        t = new MemoryControllerTile(name.str().c_str(), tileID);

        uint memoryPort = memoryControllersMade++;
        ((MemoryControllerTile*)t)->oRequestToMainMemory(mainMemory.iData[memoryPort]);
        ((MemoryControllerTile*)t)->iResponseFromMainMemory(mainMemory.oData[memoryPort]);
      }
      else {
        t = new EmptyTile(name.str().c_str(), tileID);
      }

      // Common interface for all tiles.
      t->clock(clock);
      t->oData(dataNet.inputs[col][row]);
      t->oCredit(creditNet.inputs[col][row]);
      t->oRequest(requestNet.inputs[col][row]);
      t->oResponse(responseNet.inputs[col][row]);
      dataNet.outputs[col][row](t->iData);
      creditNet.outputs[col][row](t->iCredit);
      requestNet.outputs[col][row](t->iRequest);
      responseNet.outputs[col][row](t->iResponse);

      tiles[col].push_back(t);
    }
  }
}

void Chip::wireUp() {

  // Main memory.
  mainMemory.iClock(clock);

  // Most network wiring happened when the tiles were created.
  dataNet.clock(clock);
  creditNet.clock(clock);
  requestNet.clock(clock);
  responseNet.clock(clock);

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
    clock("clock", 1, sc_core::SC_NS, 0.5) {

  makeComponents(params);
  wireUp();

}
