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

  // Store the address to read from - used for multi-cycle instruction
  // packet reads.
  std::vector<int>       readAddresses;

  // A boolean for each input, saying whether an instruction packet read is
  // currently in progress.
  std::vector<bool>      readingIPK;

  // The value used to show that there is currently no connection to this input
  // port, so it is possible to create a new connection.
  static const int       NO_CONNECTION;

};

#endif /* MEMORYMAT_H_ */
