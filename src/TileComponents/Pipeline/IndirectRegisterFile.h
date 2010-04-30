/*
 * IndirectRegisterFile.h
 *
 * A class representing a register file capable of indirect reads and writes.
 * This is achieved by having two separate Storages, with one indexing into
 * the other.
 *
 * Register 0 is reserved to hold the value 0.
 * Register 1 is reserved to hold the address of the current instruction packet.
 * The next NUM_RECEIVE_CHANNELS registers address the channel-ends.
 * Up to NUM_ADDRESSABLE_REGISTERS are normal registers.
 * Beyond that, registers must be accessed through the indirect registers.
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
  sc_in<short>  readAddr1, readAddr2;

  // Signals whether or not the second read address should use the indirect
  // registers.
  sc_in<bool>   indRead;

  // The register to write data to.
  sc_in<short>  writeAddr;

  // The register containing the index of the register which should be written to.
  sc_in<short>  indWriteAddr;

  // Data to be written to the register file.
  sc_in<Word>   writeData;

  // The channel-end we want to read from when doing an indirect read.
  sc_out<short> channelID;

  // Results of the two reads.
  sc_out<Word>  out1, out2;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(IndirectRegisterFile);
  IndirectRegisterFile(sc_module_name name);
  virtual ~IndirectRegisterFile();

//==============================//
// Methods
//==============================//

public:

  // Simple methods to tell what sort of register is being dealt with.
  static bool isReserved(int position);
  static bool isChannelEnd(int position);
  static bool isAddressableReg(int position);
  static bool needsIndirect(int position);
  static bool isInvalid(int position);

  // Return the index of the channel-end to read from, if a read to this
  // position was requested.
  static int  toChannelID(int position);

  // Return the register-addressable index corresponding to the given
  // channel ID.
  static int  fromChannelID(int position);

  // Update the contents of the register reserved to hold the address of the
  // current instruction packet. It is acceptable to have this as a separate
  // method since there is only one writer, and they only write to this one
  // register.
  void updateCurrentIPK(Address addr);

private:

  // Read from the address given in readAddr1.
  void read1();

  // Read from the address given (or pointed to) by readAddr2.
  void read2();

  // Write to the address given in the register pointed to by indWriteAddr.
  void indirectWrite();

  // Write to the address given in writeAddr.
  void write();

  // Store a subsection of the data into the indirect register at position
  // "address".
  void updateIndirectReg(int address, Word data);

//==============================//
// Local state
//==============================//

private:

  AddressedStorage<Word>  regs;
  AddressedStorage<short> indirect;

};

#endif /* INDIRECTREGISTERFILE_H_ */
