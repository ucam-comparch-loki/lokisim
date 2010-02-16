/*
 * MemoryMat.h
 *
 * Current allocation of inputs/outputs:
 * in1, in2: data input (address to be read from, or data to be written)
 * in3, in4: address input (return address, or address to write to) + read/write bit
 * out1, out2: data output
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef MEMORYMAT_H_
#define MEMORYMAT_H_

#include "../TileComponent.h"
#include "AddressedStorage.h"
#include "../Datatype/Word.h"
#include "../Datatype/Address.h"
#include "../Datatype/Array.h"

class MemoryMat: public TileComponent {

public:
/* Constructors and destructors */
  SC_HAS_PROCESS(MemoryMat);
  MemoryMat(sc_core::sc_module_name name, int ID);
  virtual ~MemoryMat();

private:
/* Methods */
  // One copy of each function for each pair of input ports.
  void doOp1();
  void doOp2();
  void read1();
  void read2();
  void write1();
  void write2();

/* Local state */
  AddressedStorage<Word> data;

};

#endif /* MEMORYMAT_H_ */
