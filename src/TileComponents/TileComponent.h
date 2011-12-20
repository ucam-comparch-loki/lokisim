/*
 * TileComponent.h
 *
 * A class representing any component which can be a networked part of a Tile.
 * This primarily means Clusters and memories, but may include more components
 * later if appropriate. All TileComponents have the same network interface,
 * with the same numbers of input channels and output channels.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef TILECOMPONENT_H_
#define TILECOMPONENT_H_

#include "../Component.h"
#include "../Communication/loki_ports.h"

class AddressedWord;
class Chip;
class Word;

class TileComponent : public Component {

//==============================//
// Ports
//==============================//

public:

  // Clock.
  sc_in<bool>            clock;

  // All inputs to the component. They still have addresses in case there are
  // multiple channel ends accessible through each port.
  loki_in<AddressedWord>  *dataIn;
  sc_out<bool>          *readyOut;

  // All outputs of the component.
  loki_out<AddressedWord> *dataOut;

  // Credits to be sent across the network.
  loki_out<AddressedWord> *creditsOut;

  // Credits received from the network. They still have addresses in case there
  // are multiple channel ends accessible through each port.
  loki_in<AddressedWord>  *creditsIn;

  // Signal that this component is not currently doing any work.
  sc_out<bool>           idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(TileComponent);
  TileComponent(sc_module_name name, const ComponentID& ID,
                int inputPorts, int outputPorts);
  virtual ~TileComponent();

//==============================//
// Methods
//==============================//

public:

  int numInputs() const;
  int numOutputs() const;

  // Store initial instructions or data into this cluster or memory.
  virtual void storeData(const std::vector<Word>& data, MemoryAddr location=0) = 0;

  // Print information about the component.
  virtual void print(MemoryAddr start, MemoryAddr end) const;

  // Access the memory contents of this component.
  virtual const Word readWord(MemoryAddr addr) const;
  virtual const Word readByte(MemoryAddr addr) const;
  virtual void writeWord(MemoryAddr addr, Word data);
  virtual void writeByte(MemoryAddr addr, Word data);

  // Return the value held in the specified register.
  virtual const int32_t readRegDebug(RegisterIndex reg) const;

  // Return the memory index of the instruction currently being decoded.
  virtual const MemoryAddr getInstIndex() const;

  // Return the value of the predicate register.
  virtual bool readPredReg() const;

protected:

  // Methods used by a core or memory to access the contents of memory.
  int32_t readMemWord(MemoryAddr addr);
  int32_t readMemByte(MemoryAddr addr);
  void writeMemWord(MemoryAddr addr, Word data);
  void writeMemByte(MemoryAddr addr, Word data);

  Chip* parent() const;

private:

  void acknowledgeCredit(PortIndex output);

//==============================//
// Local state
//==============================//

private:

  const unsigned int numInputPorts, numOutputPorts;

};

#endif /* TILECOMPONENT_H_ */
