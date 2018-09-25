/*
 * IndirectRegisterFile.h
 *
 * A class representing a register file capable of indirect reads and writes.
 * This is achieved by having a copy of the lowest few bits of each register,
 * which itself can be interpreted as a register index.
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

#include "../../LokiComponent.h"
#include "../../Memory/AddressedStorage.h"

class Core;
class Word;

class RegisterFile: public LokiComponent {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  RegisterFile(sc_module_name name, const ComponentID& ID,
               const register_file_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

public:

  // Read from a register, redirecting to RCET if necessary.
  int32_t read(PortIndex port, RegisterIndex reg, bool indirect) const;

  // Read from a register without redirecting to RCET.
  int32_t readInternal(const RegisterIndex reg) const;

  // Write to a register, including all safety checks.
  void    write(const RegisterIndex reg, int32_t value, bool indirect);

  // Simple methods to tell what sort of register is being dealt with.
  // These perhaps belong in Core rather than here.
  bool isReserved(RegisterIndex position) const;
  bool isChannelEnd(RegisterIndex position) const;
  bool isAddressableReg(RegisterIndex position) const;
  bool needsIndirect(RegisterIndex position) const;
  bool isInvalid(RegisterIndex position) const;

  // Return the index of the channel-end to read from, if a read to this
  // position was requested.
  RegisterIndex toChannelID(RegisterIndex position) const;

  // Return the register-addressable index corresponding to the given
  // channel ID.
  RegisterIndex fromChannelID(RegisterIndex position) const;

  // Update the contents of the register reserved to hold the address of the
  // current instruction packet. It is acceptable to have this as a separate
  // method since there is only one writer, and they only write to this one
  // register.
  void updateCurrentIPK(MemoryAddr addr);

private:

  // Perform the register write (no safety checks, etc.).
  void writeInternal(RegisterIndex reg, const Word value);

  void logActivity();

  Core& parent() const;

//============================================================================//
// Local state
//============================================================================//

private:

  AddressedStorage<Word>          regs;

  // Data from previous read on each port. Used to compute Hamming distances
  // for energy models. (wr=0, rd1=1, rd2=2)
  vector<int> prevRead;
  cycle_count_t lastActivity;

  // The register index at which the input channels begin.
  static const RegisterIndex START_OF_INPUT_CHANNELS = 2;

};

#endif /* INDIRECTREGISTERFILE_H_ */
