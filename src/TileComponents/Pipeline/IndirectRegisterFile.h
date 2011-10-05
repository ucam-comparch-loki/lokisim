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

class Cluster;
class Word;

class IndirectRegisterFile: public Component {

//==============================//
// Constructors and destructors
//==============================//

public:

  IndirectRegisterFile(sc_module_name name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // Read from a register, redirecting to RCET if necessary.
  const int32_t read(const RegisterIndex reg, bool indirect) const;

  // Read from a register without redirecting to RCET.
  const int32_t readDebug(const RegisterIndex reg) const;

  // Write to a register, including all safety checks.
  void    write(const RegisterIndex reg, const int32_t value, bool indirect);

  // Simple methods to tell what sort of register is being dealt with.
  static bool isReserved(RegisterIndex position);
  static bool isChannelEnd(RegisterIndex position);
  static bool isAddressableReg(RegisterIndex position);
  static bool needsIndirect(RegisterIndex position);
  static bool isInvalid(RegisterIndex position);

  // Return the index of the channel-end to read from, if a read to this
  // position was requested.
  static RegisterIndex toChannelID(RegisterIndex position);

  // Return the register-addressable index corresponding to the given
  // channel ID.
  static RegisterIndex fromChannelID(RegisterIndex position);

  // Update the contents of the register reserved to hold the address of the
  // current instruction packet. It is acceptable to have this as a separate
  // method since there is only one writer, and they only write to this one
  // register.
  void updateCurrentIPK(const MemoryAddr addr);

private:

  // Store a subsection of the data into the indirect register at position
  // "address".
  void updateIndirectReg(const RegisterIndex reg, const Word data);

  // Perform the register write (no safety checks, etc.).
  void writeReg(const RegisterIndex reg, const Word value);

  Cluster* parent() const;

//==============================//
// Local state
//==============================//

private:

  AddressedStorage<Word>          regs;
  AddressedStorage<RegisterIndex> indirectRegs;

  // The register index at which the input channels begin.
  static const RegisterIndex START_OF_INPUT_CHANNELS = 2;

};

#endif /* INDIRECTREGISTERFILE_H_ */
