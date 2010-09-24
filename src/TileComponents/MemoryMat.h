/*
 * MemoryMat.h
 *
 * A memory bank which is accessed over the network. There will usually be
 * multiple MemoryMats in each Tile.
 *
 * Consider having designated read and write inputs?
 *   + Simplifies hardware and software
 *   + Allows full utilisation of input ports (there are usually more than outputs)
 *   - Less flexibility
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef MEMORYMAT_H_
#define MEMORYMAT_H_

#include "TileComponent.h"
#include "../Memory/AddressedStorage.h"
#include "../Memory/BufferArray.h"

class ConnectionStatus;
class Word;

class MemoryMat: public TileComponent {

//==============================//
// Ports
//==============================//

// Inherited from TileComponent:
//   clock
//   in
//   out
//   flowControlIn
//   flowControlOut
//   idle

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(MemoryMat);
  MemoryMat(sc_module_name name, int ID);
  virtual ~MemoryMat();

//==============================//
// Methods
//==============================//

public:

  // The area of this component in square micrometres.
  virtual double area()   const;

  // The energy consumed by this component in picojoules.
  virtual double energy() const;

  // Initialise the contents of this memory to the Words in the given vector.
  virtual void storeData(std::vector<Word>& data);

  // Print the contents of this memory.
  virtual void print(int start=0, int end=MEMORY_SIZE) const;

  // Return the value at the given address.
  virtual Word getMemVal(int addr) const;

private:

  // Method which is called at the beginning of each clock cycle.
  // Look through all inputs for new data. Determine whether this data is the
  // start of a new transaction or the continuation of an existing one. Then
  // carry out the first/next step of the transaction.
  void newCycle();

  // Carry out a read for the transaction at input "position".
  void read(int position);

  // Carry out a write for the transaction at input "position".
  void write(Word w, int position);

  // Update the current connections to this memory.
  void updateControl();

  // Update the output signal telling whether the memory is idle.
  void updateIdle();

//==============================//
// Local state
//==============================//

public:

  // The index of the input port which receives control commands, which allow
  // the set-up and tear-down of channels.
  static const int       CONTROL_INPUT;

private:

  // The data stored by this memory.
  AddressedStorage<Word> data;

  // Information on the channels set up with each of this memory's inputs.
  std::vector<ConnectionStatus> connections;

  // A buffer for each output channel (the size will probably be 1, but using
  // a buffer instead of a register allows more experimentation).
  BufferArray<AddressedWord> buffers;

  // The number of words we have initially put in this memory. Allows multiple
  // files to be put into the same memory.
  int wordsLoaded;

};

#endif /* MEMORYMAT_H_ */
