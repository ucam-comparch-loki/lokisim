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

#include "../../Component.h"
#include "../../Memory/AddressedStorage.h"
#include "../../Datatype/Word.h"
#include "../../Datatype/Address.h"
#include "../../Multiplexor/Multiplexor2.h"

class IndirectRegisterFile: public Component {

//==============================//
// Ports
//==============================//

public:

  // Two addresses to read data from.
  sc_in<short> readAddr1, readAddr2;

  // Signals whether or not the second read address should use the indirect
  // registers.
  sc_in<bool>  indRead;

  // The register to write data to.
  sc_in<short> writeAddr;

  // The register containing the index of the register which should be written to.
  sc_in<short> indWriteAddr;

  // Data to be written to the register file.
  sc_in<Word>  writeData;

  // Results of the two reads.
  sc_out<Word> out1, out2;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(IndirectRegisterFile);
  IndirectRegisterFile(sc_core::sc_module_name name);
  virtual ~IndirectRegisterFile();

//==============================//
// Methods
//==============================//

private:

  void read1();
  void read2();
  void indirectWrite();
  void write();

//==============================//
// Local state
//==============================//

private:

  AddressedStorage<Word>  regs;
  AddressedStorage<short> indirect;

};

#endif /* INDIRECTREGISTERFILE_H_ */
