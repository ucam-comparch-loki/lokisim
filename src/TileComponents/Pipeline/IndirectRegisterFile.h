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

class IndirectRegisterFile: public Component {

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

  int32_t read(uint8_t reg, bool indirect) const;

  void    write(uint8_t reg, int32_t value, bool indirect);

private:

  // Store a subsection of the data into the indirect register at position
  // "address".
  void updateIndirectReg(int address, Word data);

//==============================//
// Local state
//==============================//

private:

  AddressedStorage<Word>    regs;
  AddressedStorage<uint8_t> indirectRegs;

  // The register index at which the input channels begin.
  static const int START_OF_INPUT_CHANNELS = 16;

};

#endif /* INDIRECTREGISTERFILE_H_ */
