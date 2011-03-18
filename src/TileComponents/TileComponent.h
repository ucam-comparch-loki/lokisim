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
  sc_in<AddressedWord>  *in;
  sc_out<bool>          *canReceiveData;

  // All outputs of the component.
  sc_out<AddressedWord> *out;
  sc_in<bool>           *canSendData;

  // Credits to be sent across the network.
  sc_out<AddressedWord> *creditsOut;
  sc_in<bool>           *canSendCredit;

  // Credits received from the network.
  sc_in<AddressedWord>  *creditsIn;
  sc_out<bool>          *canReceiveCredit;

  // Signal that this component is not currently doing any work.
  sc_out<bool>           idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  TileComponent(sc_module_name name, ComponentID ID,
                int inputPorts, int outputPorts);
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
  virtual void writeWord(MemoryAddr addr, Word data);
  virtual void writeByte(MemoryAddr addr, Word data);

  // Return the value held in the specified register.
  virtual const int32_t readRegDebug(RegisterIndex reg) const;

  // Return the memory index of the instruction currently being decoded.
  virtual const MemoryAddr getInstIndex() const;

  // Return the value of the predicate register.
  virtual bool readPredReg() const;

  // Return a unique address for each input/output port of each core/memory.
  static ChannelID inputPortID(ComponentID component, PortIndex port);
  static ChannelID outputPortID(ComponentID component, PortIndex port);
  static ChannelID inputChannelID(ComponentID component, ChannelIndex channel);
  static ChannelID outputChannelID(ComponentID component, ChannelIndex channel);

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
