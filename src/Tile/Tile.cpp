/*
 * Tile.cpp
 *
 *  Created on: 4 Oct 2016
 *      Author: db434
 */

#include "Tile.h"
#include "../Chip.h"
#include "../Exceptions/UnsupportedFeatureException.h"

Tile::Tile(const sc_module_name& name, const ComponentID& id) :
    LokiComponent(name, id),
    iClock("clock"),
    iData("iData"),
    oData("oData"),
    iDataReady("iDataReady"),
    oDataReady("oDataReady"),
    iCredit("iCredit"),
    oCredit("oCredit"),
    iCreditReady("iCreditReady"),
    oCreditReady("oCreditReady"),
    iRequest("iRequest"),
    oRequest("oRequest"),
    iRequestReady("iRequestReady"),
    oRequestReady("oRequestReady"),
    iResponse("iResponse"),
    oResponse("oResponse"),
    iResponseReady("iResponseReady"),
    oResponseReady("oResponseReady") {
  // Nothing

}

Tile::~Tile() {
  // Nothing
}

uint Tile::numComponents() const {return 0;}
uint Tile::numCores() const {return 0;}
uint Tile::numMemories() const {return 0;}

bool Tile::isCore(ComponentID id) const {
  LOKI_ERROR << "Calling Tile::isCore on Tile with no cores " << id << endl;
  assert(false);
  return false;
}

bool Tile::isMemory(ComponentID id) const {
  LOKI_ERROR << "Calling Tile::isMemory on Tile with no memories " << id << endl;
  assert(false);
  return false;
}

uint Tile::componentIndex(ComponentID id) const {
  LOKI_ERROR << "Calling Tile::localComponentNumber on Tile with no components "
      << id << endl;
  assert(false);
  return 0;
}

uint Tile::coreIndex(ComponentID id) const {
  LOKI_ERROR << "Calling Tile::localCoreNumber on Tile with no cores "
      << id << endl;
  assert(false);
  return 0;
}

uint Tile::memoryIndex(ComponentID id) const {
  LOKI_ERROR << "Calling Tile::localMemoryNumber on Tile with no memories "
      << id << endl;
  assert(false);
  return 0;
}

bool Tile::isComputeTile(TileID id) const {
  return chip().isComputeTile(id);
}

void Tile::storeInstructions(vector<Word>& instructions, const ComponentID& component) {
  throw UnsupportedFeatureException("Tile::storeInstructions");
}

void Tile::storeData(const DataBlock& data) {
  throw UnsupportedFeatureException("Tile::storeData");
}

void Tile::print(const ComponentID& component, MemoryAddr start, MemoryAddr end) {
  throw UnsupportedFeatureException("Tile::print");
}

Word Tile::readWordInternal(const ComponentID& component, MemoryAddr addr) {
  throw UnsupportedFeatureException("Tile::readWordInternal");
  return Word();
}

Word Tile::readByteInternal(const ComponentID& component, MemoryAddr addr) {
  throw UnsupportedFeatureException("Tile::readByteInternal");
  return Word();
}

void Tile::writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  throw UnsupportedFeatureException("Tile::writeWordInternal");
}

void Tile::writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data) {
  throw UnsupportedFeatureException("Tile::writeByteInternal");
}

int  Tile::readRegisterInternal(const ComponentID& component, RegisterIndex reg) const {
  throw UnsupportedFeatureException("Tile::readRegisterInternal");
  return 0;
}

bool Tile::readPredicateInternal(const ComponentID& component) const {
  throw UnsupportedFeatureException("Tile::readPredicateInternal");
  return false;
}

void Tile::networkSendDataInternal(const NetworkData& flit) {
  throw UnsupportedFeatureException("Tile::networkSendDataInternal");
}

void Tile::networkSendCreditInternal(const NetworkCredit& flit) {
  throw UnsupportedFeatureException("Tile::networkSendCreditInternal");
}

void Tile::magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload) {
  chip().magicMemoryAccess(opcode, address, returnChannel, payload);
}

Chip& Tile::chip() const {
  return static_cast<Chip&>(*(this->get_parent_object()));
}

