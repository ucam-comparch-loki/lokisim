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

using std::vector;
using std::string;

class Instruction: public Word {

public:

  enum Predicate {P, NOT_P, ALWAYS, END_OF_PACKET};

//==============================//
// Methods
//==============================//

public:

  unsigned short getOp() const;
  unsigned short getDest() const;
  unsigned short getSrc1() const;
  unsigned short getSrc2() const;
  unsigned short getRchannel() const;
  signed   short getImmediate() const;
  unsigned short getPredicate() const;
  bool           getSetPredicate() const;
  bool           endOfPacket() const;

  void setOp(short val);
  void setDest(short val);
  void setSrc1(short val);
  void setSrc2(short val);
  void setRchannel(short val);
  void setImmediate(short val);
  void setPredicate(short val);
  void setSetPred(bool val);

  bool operator== (const Instruction& other) const;

  // Has to go in header
  friend std::ostream& operator<< (std::ostream& os, Instruction const& v) {
    os << v.getOp() <<" "<< v.getDest() <<" "<< v.getSrc1() <<" "<< v.getSrc2()
       <<" "<< v.getImmediate() <<" "<< v.getRchannel();
    return os;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  Instruction();
  Instruction(const Word& other);
  Instruction(unsigned int inst);   // For reading binary
  Instruction(const string &inst);  // For reading assembler
  virtual ~Instruction();

//==============================//
// String manipulation methods
//==============================//

private:

  vector<string>& split(const string &s, char delim, vector<string> &elems);
  vector<string>  split(const string &s, char delim);
  int             strToInt(const string &s);
  void            decode(const string &s, int field);

};

#endif /* INSTRUCTION_H_ */
