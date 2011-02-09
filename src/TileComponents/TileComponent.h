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

class Address;
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

  // All inputs to the component.
  sc_in<Word>           *in;

  // All outputs of the component.
  sc_out<AddressedWord> *out;

  // A flow control signal for each output.
  sc_in<bool>           *flowControlIn;

  // A flow control signal for each input. Each one tells how much space is
  // remaining in a particular input buffer.
  // Note: although this is possible, the usage now seems to be to always send
  // "1", representing a single credit.
  sc_out<int>           *flowControlOut;

  // Signal that this component is not currently doing any work.
  sc_out<bool>           idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  TileComponent(sc_module_name name, ComponentID ID);
  virtual ~TileComponent();

//==============================//
// Methods
//==============================//

public:

  // Store initial instructions or data into this cluster or memory.
  virtual void storeData(const std::vector<Word>& data, MemoryAddr location=0) = 0;

  // Print information about the component.
  virtual void print(MemoryAddr start, MemoryAddr end) const;

  // Access the memory contents of this component.
  virtual const Word readWord(MemoryAddr addr) const;
  virtual const Word readByte(MemoryAddr addr) const;
  virtual void writeWord(MemoryAddr addr, Word data) const;
  virtual void writeByte(MemoryAddr addr, Word data) const;

  // Return the value held in the specified register.
  virtual const int32_t readRegDebug(RegisterIndex reg) const;

  // Return the memory index of the instruction currently being decoded.
  virtual const Address getInstIndex() const;

  // Return the value of the predicate register.
  virtual bool readPredReg() const;

  // Return a unique address for each input/output port of each core/memory.
  static ChannelID inputPortID(ComponentID component, ChannelIndex port);
  static ChannelID outputPortID(ComponentID component, ChannelIndex port);

  // Determine which component holds the given port.
  static ComponentID component(ChannelID port);

  // Convert a unique port address into the form "(component, port)".
  static const std::string inputPortString(ChannelID port);
  static const std::string outputPortString(ChannelID port);

protected:

  // Methods used by a core or memory to access the contents of memory.
  int32_t readMemWord(MemoryAddr addr) const;
  int32_t readMemByte(MemoryAddr addr) const;
  void writeMemWord(MemoryAddr addr, Word data) const;
  void writeMemByte(MemoryAddr addr, Word data) const;

  Chip*            parent() const;

};

#endif /* TILECOMPONENT_H_ */
