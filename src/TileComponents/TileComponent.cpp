/*
 * TileComponent.cpp
 *
 *  Created on: 11 Jan 2010
 *      Author: db434
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "TileComponent.h"
#include "../Chip.h"
#include "../Datatype/AddressedWord.h"
#include "../Datatype/ChannelID.h"

int TileComponent::numInputs()  const {return numInputPorts;}
int TileComponent::numOutputs() const {return numOutputPorts;}

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

void TileComponent::acknowledgeCredit(PortIndex output) {
  if(clock.posedge()) {
    // Deassert the acknowledgement on the clock edge.
    ackCreditIn[output].write(false);

    // The valid signal gets deasserted on this clock edge, so will still appear
    // to be high. Wait a delta cycle.
    next_trigger(sc_core::SC_ZERO_TIME);
  }
  else if(validCreditIn[output].read()) {
    // A credit has arrived: we can safely acknowledge any credit because they
    // are immediately consumed.
    ackCreditIn[output].write(true);
    next_trigger(clock.posedge_event());
  }
  else {
    // Wait until the next credit arrives.
    next_trigger(validCreditIn[output].posedge_event());
  }
}

/* Constructors and destructors */
TileComponent::TileComponent(sc_module_name name, const ComponentID& ID,
                             int inputPorts, int outputPorts) :
    Component(name, ID),
    numInputPorts(inputPorts),
    numOutputPorts(outputPorts) {

  dataIn         = new sc_in<AddressedWord>[inputPorts];
  validDataIn    = new sc_in<bool>[inputPorts];

  dataOut        = new sc_out<AddressedWord>[outputPorts];
  validDataOut   = new sc_out<bool>[outputPorts];

  // Temporary? Only have a single credit output, used to send credits to other
  // tiles. Credits aren't used for local communication.
  creditsOut     = new sc_out<AddressedWord>[1];
  validCreditOut = new sc_out<bool>[1];
  ackCreditOut   = new sc_in<bool>[1];

  creditsIn      = new sc_in<AddressedWord>[outputPorts];
  validCreditIn  = new sc_in<bool>[outputPorts];
  ackCreditIn    = new sc_out<bool>[outputPorts];

  // Generate a method to watch each credit input port, and send an
  // acknowledgement whenever a credit arrives.
  for(unsigned int i=0; i<numOutputPorts; i++) {
    sc_core::sc_spawn_options options;
    options.spawn_method();     // Want an efficient method, not a thread
    options.dont_initialize();  // Only execute when triggered
    options.set_sensitivity(&(validCreditIn[i].pos())); // Sensitive to this port

    // Create the method.
    sc_spawn(sc_bind(&TileComponent::acknowledgeCredit, this, i), 0, &options);
  }

  idle.initialize(true);

}

TileComponent::~TileComponent() {
  delete[] dataIn;      delete[] validDataIn;
  delete[] dataOut;     delete[] validDataOut;
  delete[] creditsOut;  delete[] validCreditOut;  delete[] ackCreditOut;
  delete[] creditsIn;   delete[] validCreditIn;   delete[] ackCreditIn;
}
