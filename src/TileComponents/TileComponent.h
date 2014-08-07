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
#include "../Network/NetworkTypedefs.h"

class AddressedWord;
class Chip;
class Word;

class TileComponent : public Component {

//==============================//
// Ports
//==============================//

public:

  // Clock.
  ClockInput     clock;

  // All inputs to the component. They still have addresses in case there are
  // multiple channel ends accessible through each port.
  LokiVector<DataInput>    iData;

  // All outputs of the component.
  LokiVector<DataOutput>   oData;

  // Signal that this component is not currently doing any work.
  IdleOutput     oIdle;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(TileComponent);
  TileComponent(sc_module_name name, const ComponentID& ID,
                int inputPorts, int outputPorts);

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

};

#endif /* TILECOMPONENT_H_ */
