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
    LokiComponent(name, id) {
  // Nothing

}

Tile::~Tile() {
  // Nothing
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
  chip()->magicMemoryAccess(opcode, address, returnChannel, payload);
}

Chip* Tile::chip() const {
  return static_cast<Chip*>(this->get_parent_object());
}

