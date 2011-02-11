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

#include "Word.h"
#include "../Typedefs.h"

using std::string;

class Instruction: public Word {

//==============================//
// Methods
//==============================//

public:

  // Accessors
  uint8_t  opcode() const;
  RegisterIndex destination() const;
  RegisterIndex sourceReg1() const;
  RegisterIndex sourceReg2() const;
  ChannelIndex  remoteChannel() const;
  int32_t  immediate() const;
  uint8_t  predicate() const;
  bool     setsPredicate() const;
  bool     endOfPacket() const;

  // Mutators
  void     opcode(const uint8_t val);
  void     destination(const RegisterIndex val);
  void     sourceReg1(const RegisterIndex val);
  void     sourceReg2(const RegisterIndex val);
  void     remoteChannel(const ChannelIndex val);
  void     immediate(const int32_t val);
  void     predicate(const uint8_t val);
  void     setsPredicate(const bool val);

  uint64_t toLong() const;
  bool     operator== (const Instruction& other) const;

  // If the size of an instruction is greater than the size of the word, we
  // would like to be able to split it into word-size chunks for storage.
  Word     firstWord() const;
  Word     secondWord() const;

  // Has to go in header
  friend std::ostream& operator<< (std::ostream& os, const Instruction& v) {
    return v.print(os);
  }

private:

  void    decodeOpcode(const string& opcode);
  RegisterIndex decodeField(const string& s);
  int32_t decodeRChannel(const string& channel);

  // Set the appropriate register fields in this instruction. Some operations
  // may not have a destination register, for example, so it should not be used.
  void    setFields(const RegisterIndex reg1, const RegisterIndex reg2,
                    const RegisterIndex reg3);

  // Contains the implementation of the << operator so it doesn't have to go in
  // the header.
  std::ostream& print(std::ostream& os) const;

//==============================//
// Constructors and destructors
//==============================//

public:

  Instruction();
  Instruction(const Word& other);
  Instruction(const uint64_t inst);  // For reading binary
  Instruction(const string& inst);   // For reading assembler
  virtual ~Instruction();

//==============================//
// Local state
//==============================//

public:

  // The options for the predicate value.
  //   ALWAYS        = always execute
  //   P             = execute only if the predicate register holds 1
  //   NOT_P         = execute only if the predicate register holds 0
  //   END_OF_PACKET = this instruction is the last in the current packet
  enum Predicate {ALWAYS, P, NOT_P, END_OF_PACKET};

  // A remote channel value signifying that no channel was specified.
  static const ChannelIndex NO_CHANNEL = 15;

};

#endif /* INSTRUCTION_H_ */
