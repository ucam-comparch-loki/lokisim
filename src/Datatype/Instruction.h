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
#include "../Utility/InstructionMap.h"

using std::string;

class Instruction: public Word {

//==============================//
// Methods
//==============================//

public:

  uint16_t getOp() const;
  uint8_t  getDest() const;
  uint8_t  getSrc1() const;
  uint8_t  getSrc2() const;
  uint8_t  getRchannel() const;
  int32_t  getImmediate() const;
  uint8_t  getPredicate() const;
  bool     getSetPredicate() const;
  bool     endOfPacket() const;

  void setOp(uint16_t val);
  void setDest(uint8_t val);
  void setSrc1(uint8_t val);
  void setSrc2(uint8_t val);
  void setRchannel(uint8_t val);
  void setImmediate(int32_t val);
  void setPredicate(uint8_t val);
  void setSetPred(bool val);

  bool operator== (const Instruction& other) const;

  // Has to go in header
  friend std::ostream& operator<< (std::ostream& os, const Instruction& v) {
    if(v.getPredicate() == P) os << "p?";
    else if(v.getPredicate() == NOT_P) os << "!p?";

    os << InstructionMap::name(InstructionMap::operation(v.getOp())) <<
       (v.getSetPredicate()?".p":"") <<
       (v.endOfPacket()?".eop":"") << " r" << v.getDest() << " r" << v.getSrc1()
       << " r" << v.getSrc2() << " " << v.getImmediate();
    if(v.getRchannel() != NO_CHANNEL) os << " -> " << v.getRchannel();
    return os;
  }

private:

  void decodeOpcode(const string& opcode);
  void decodeField(const string& s, int field);
  void decodeRChannel(const string& channel);

//==============================//
// Constructors and destructors
//==============================//

public:

  Instruction();
  Instruction(const Word& other);
  Instruction(uint64_t inst);  // For reading binary
  Instruction(const string& inst);  // For reading assembler
  virtual ~Instruction();

//==============================//
// Local state
//==============================//

public:

  // The options for the predicate value.
  //   ALWAYS        = always execute
  //   P             = execute if the predicate register holds 1
  //   NOT_P         = execute if the predicate register holds 0
  //   END_OF_PACKET = this instruction is the last in the current packet
  enum Predicate {ALWAYS, P, NOT_P, END_OF_PACKET};

  // A remote channel value signifying that no channel was specified.
  static const uint8_t NO_CHANNEL = 255;

};

#endif /* INSTRUCTION_H_ */
