/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "TileComponent.h"
#include "../Chip.h"
#include "../Utility/Assert.h"

int TileComponent::numInputs()  const {return iData.length();}
int TileComponent::numOutputs() const {return oData.length();}

void TileComponent::print(MemoryAddr start, MemoryAddr end) const {
  // Do nothing if print isn't defined
}

const Word TileComponent::readWord(MemoryAddr addr) const {return Word(-1);}
const Word TileComponent::readByte(MemoryAddr addr) const {return Word(-1);}
void TileComponent::writeWord(MemoryAddr addr, Word data) {}
void TileComponent::writeByte(MemoryAddr addr, Word data) {}

const int32_t TileComponent::readRegDebug(RegisterIndex reg) const {
  return -1;
}

const MemoryAddr TileComponent::getInstIndex() const {
  return 0xFFFFFFFF;
}

bool TileComponent::readPredReg() const {
  return false;
}

int32_t TileComponent::readMemWord(MemoryAddr addr) {
  // For now, this always reads from the background memory
  return parent()->readWordInternal(id, addr).toInt();
}

int32_t TileComponent::readMemByte(MemoryAddr addr) {
  // For now, this always reads from the background memory
  return parent()->readByteInternal(id, addr).toInt();
}

void TileComponent::writeMemWord(MemoryAddr addr, Word data) {
  // For now, this always writes to the background memory
  parent()->writeWordInternal(id, addr, data);
}

void TileComponent::writeMemByte(MemoryAddr addr, Word data) {
  // For now, this always writes to the background memory
  parent()->writeByteInternal(id, addr, data);
}

Chip* TileComponent::parent() const {
  return static_cast<Chip*>(this->get_parent_object());
}

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, const ComponentID& ID,
                             int inputPorts, int outputPorts) :
    Component(name, ID) {

  loki_assert(inputPorts > 0);
  loki_assert(outputPorts > 0);

  iData.init(inputPorts);
  oData.init(outputPorts);

}
