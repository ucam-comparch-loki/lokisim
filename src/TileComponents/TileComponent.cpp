/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "TileComponent.h"
#include "../Chip.h"
#include "../Datatype/ChannelID.h"

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
	return parent()->readWord(id, addr).toInt();
}

int32_t TileComponent::readMemByte(MemoryAddr addr) {
	// For now, this always reads from the background memory
	return parent()->readByte(id, addr).toInt();
}

void TileComponent::writeMemWord(MemoryAddr addr, Word data) {
	// For now, this always writes to the background memory
	parent()->writeWord(id, addr, data);
}

void TileComponent::writeMemByte(MemoryAddr addr, Word data) {
	// For now, this always writes to the background memory
	parent()->writeByte(id, addr, data);
}

Chip* TileComponent::parent() const {
  return static_cast<Chip*>(this->get_parent());
}

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, const ComponentID& ID,
                             int inputPorts, int outputPorts) :
    Component(name, ID) {

  iData.init(inputPorts);
  oData.init(outputPorts);

}
