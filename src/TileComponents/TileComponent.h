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
class Word;

class TileComponent : public Component {

//==============================//
// Ports
//==============================//

public:

  // Clock
  sc_in<bool>            clock;

  // All inputs to the component. There should be NUM_CLUSTER_INPUTS of them.
  sc_in<Word>           *in;

  // All outputs of the component. There should be NUM_CLUSTER_OUTPUTS of them.
  sc_out<AddressedWord> *out;

  // A flow control signal for each output (NUM_CLUSTER_OUTPUTS).
  sc_in<bool>           *flowControlIn;

  // A flow control signal for each input (NUM_CLUSTER_INPUTS). Each one tells
  // how much space is remaining in a particular input buffer.
  sc_out<int>           *flowControlOut;

  // Signal that this component is not currently doing any work.
  sc_out<bool>           idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  TileComponent(sc_module_name name, uint16_t ID);
  virtual ~TileComponent();

//==============================//
// Methods
//==============================//

public:

  // Store initial instructions or data into this cluster or memory.
  virtual void storeData(std::vector<Word>& data, int location=0) = 0;

  // Print information about the component.
  virtual void print(int start, int end) const;

  // Return the data held at the given memory address.
  virtual Word getMemVal(uint32_t addr) const;

  // Return the value held in the specified register.
  virtual int32_t readRegDebug(uint8_t reg) const;

  // Return the memory index of the instruction currently being decoded.
  virtual Address getInstIndex() const;

  // Return the value of the predicate register.
  virtual bool readPredReg() const;

  enum Type {CLUSTER, MEMORY};

};

#endif /* TILECOMPONENT_H_ */
