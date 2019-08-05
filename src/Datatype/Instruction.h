/*
 * Instruction.h
 *
 * Class representing an instruction as a long value. There is no optimisation,
 * for simplicity: every component value is stored at a different location, even
 * if some of the values are not used by the particular instruction.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

#include "../Types.h"
#include "Word.h"
#include "../ISA/ISA.h"

using std::string;

// Options for the predicate value.
enum Predicate {
  EXECUTE_ALWAYS,   // Always execute
  EXECUTE_IF_P,     // Execute only if the predicate register holds 1
  EXECUTE_IF_NOT_P, // Execute only if the predicate register holds 0
  END_OF_PACKET     // Always execute + this is the packet's last instruction
};
typedef Predicate predicate_t;

// A remote channel value signifying that no channel was specified.
const ChannelIndex NO_CHANNEL = 15;

class Instruction: public Word {

//============================================================================//
// Methods
//============================================================================//

public:

  // Accessors
  opcode_t  opcode() const;
  function_t function() const;
  RegisterIndex reg1() const;
  RegisterIndex reg2() const;
  RegisterIndex reg3() const;
  ChannelIndex  remoteChannel() const;
  int32_t  immediate() const;
  predicate_t predicate() const;
  bool     endOfPacket() const;

  // Mutators
  void     opcode(const opcode_t val);
  void     function(const function_t val);
  void     reg1(const RegisterIndex val);
  void     reg2(const RegisterIndex val);
  void     reg3(const RegisterIndex val);
  void     remoteChannel(const ChannelIndex val);
  void     immediate(const int32_t val);
  void     predicate(const predicate_t val);

  uint64_t toLong() const;
  bool     operator== (const Instruction& other) const;

private:

  void    decodeOpcode(const string& opcode);
  RegisterIndex decodeField(const string& s);
  int32_t decodeImmediate(const string& channel);

  // Set the appropriate register fields in this instruction. Some operations
  // may not have a destination register, for example, so it should not be used.
  void    setFields(const RegisterIndex reg1, const RegisterIndex reg2,
                    const RegisterIndex reg3);

  // Contains the implementation of the << operator so it doesn't have to go in
  // the header.
  virtual std::ostream& print(std::ostream& os) const;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Instruction();
  Instruction(const Word& other);
  Instruction(const uint64_t inst);  // For reading binary
  Instruction(const string& inst);   // For reading assembly

  virtual ~Instruction() {}

};

#endif /* INSTRUCTION_H_ */
