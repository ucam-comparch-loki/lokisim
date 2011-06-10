/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#include "TileComponent.h"
#include "../Chip.h"
#include "../Datatype/AddressedWord.h"
#include "../Datatype/ChannelID.h"

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

void TileComponent::acknowledgeCredit() {
  for(int i=0; i<numOutputPorts; i++) {
    ackCreditIn[i].write(false);

    // Send an acknowledgement straight away. Credits are always immediately
    // consumed.
    if(validCreditIn[i].read()) {
      ackCreditIn[i].write(true);
    }
  }
}

void TileComponent::receivedCredit() {
  credit.notify();
}

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, const ComponentID& ID,
                             int inputPorts, int outputPorts) :
    Component(name, ID) {

  numInputPorts  = inputPorts;
  numOutputPorts = outputPorts;

  dataIn         = new sc_in<AddressedWord>[inputPorts];
  validDataIn    = new sc_in<bool>[inputPorts];
  ackDataIn      = new sc_out<bool>[inputPorts];

  dataOut        = new sc_out<AddressedWord>[outputPorts];
  validDataOut   = new sc_out<bool>[outputPorts];
  ackDataOut     = new sc_in<bool>[outputPorts];

  creditsOut     = new sc_out<AddressedWord>[inputPorts];
  validCreditOut = new sc_out<bool>[inputPorts];
  ackCreditOut   = new sc_in<bool>[inputPorts];

  creditsIn      = new sc_in<AddressedWord>[outputPorts];
  validCreditIn  = new sc_in<bool>[outputPorts];
  ackCreditIn    = new sc_out<bool>[outputPorts];

  SC_METHOD(receivedCredit);
  for(int i=0; i<outputPorts; i++) sensitive << validCreditIn[i].pos();
  dont_initialize();

  SC_METHOD(acknowledgeCredit);
  sensitive << clock.pos();
  dont_initialize();

  idle.initialize(true);

}

TileComponent::~TileComponent() {
  delete[] dataIn;
  delete[] validDataIn;
  delete[] ackDataIn;

  delete[] dataOut;
  delete[] validDataOut;
  delete[] ackDataOut;

  delete[] creditsOut;
  delete[] validCreditOut;
  delete[] ackCreditOut;

  delete[] creditsIn;
  delete[] validCreditIn;
  delete[] ackCreditIn;
}
