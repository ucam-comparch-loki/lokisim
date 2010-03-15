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
#include "../Datatype/Address.h"
#include "../Datatype/MemoryRequest.h"

class MemoryMat: public TileComponent {

public:

/*
 * Ports inherited from TileComponent:
 *   clock
 *   in
 *   out
 *   flowControlIn
 *   flowControlOut
 */

/* Constructors and destructors */
  SC_HAS_PROCESS(MemoryMat);
  MemoryMat(sc_module_name name, int ID);
  virtual ~MemoryMat();

/* Methods */
  virtual void storeData(std::vector<Word>& data);

private:
  void doOp();
  void read(int position);
  void write(Word w, int position);

/* Local state */
  AddressedStorage<Word> data;
  MemoryRequest         *transactions;  // array

};

#endif /* MEMORYMAT_H_ */
