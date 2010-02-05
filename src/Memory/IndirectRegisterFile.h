/*
 * IndirectRegisterFile.h
 *
 * A class representing a register file capable of indirect reads and writes.
 * This is achieved by having two separate Storages, with one indexing into
 * the other.
 *
 *  Created on: 6 Jan 2010
 *      Author: db434
 */

#ifndef INDIRECTREGISTERFILE_H_
#define INDIRECTREGISTERFILE_H_

#include "../Component.h"
#include "AddressedStorage.h"
#include "../Datatype/Word.h"
#include "../Datatype/Address.h"
#include "../Multiplexor/Multiplexor2.h"

class IndirectRegisterFile: public Component {

/* Local state */
  AddressedStorage<Word> regs;
  AddressedStorage<short> indirect;

/* Methods */
  void read1();
  void read2();
  void indirectWrite();
  void write();

public:
/* Ports */
  sc_in<short> indWriteAddr, readAddr1, readAddr2, writeAddr;
  sc_in<bool> indRead;
  sc_in<Word> writeData;
  sc_out<Word> out1, out2;

/* Constructors and destructors */
  SC_HAS_PROCESS(IndirectRegisterFile);
  IndirectRegisterFile(sc_core::sc_module_name name, int ID);
  virtual ~IndirectRegisterFile();
};

#endif /* INDIRECTREGISTERFILE_H_ */
