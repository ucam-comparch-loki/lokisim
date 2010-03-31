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
#include "ConnectionStatus.h"
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

  // Information on the channels set up with each of this memory's inputs.
  std::vector<ConnectionStatus> connections;

};

#endif /* MEMORYMAT_H_ */
