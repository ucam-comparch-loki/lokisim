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
#include "../Datatype/Word.h"

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

  virtual void storeData(std::vector<Word>& data);

private:

  void doOp();
  void read(int position);
  void write(Word w, int position);
  void updateControl();

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

  // The remote channels currently connected to each of the input ports.
  std::vector<int>       connections;

  // Storing a value in memory requires two flits: one saying where to store
  // to, and one with the data. This vector holds the addresses whilst waiting
  // for data to arrive.
  std::vector<int>       storeAddresses;

  // Shows, for each input, whether a write operation is a one-off, or whether
  // the connection is set up for the cluster to stream data to this memory.
  std::vector<bool>      streamingWrites;

  // Store the address to read from - used for multi-cycle instruction
  // packet reads.
  std::vector<int>       readAddresses;

  // A boolean for each input, saying whether an instruction packet read is
  // currently in progress.
  std::vector<bool>      readingIPK;

  // The value used to show that an entry in a vector is not currently
  // storing any information related to an operation.
  static const int       UNUSED;

  // The amount to increment the address by after a read or write. For the
  // moment, it's 1, but it could be 4 if we start accessing bytes.
  static const int       STRIDE;

};

#endif /* MEMORYMAT_H_ */
