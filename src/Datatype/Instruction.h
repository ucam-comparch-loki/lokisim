/*
 * Instruction.h
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

#include "Word.h"

class Instruction: public Word {

public:

  enum Predicate {P, NOT_P, ALWAYS, END_OF_PACKET};

/* Methods */
  unsigned short getOp() const;
  unsigned short getDest() const;
  unsigned short getSrc1() const;
  unsigned short getSrc2() const;
  unsigned short getRchannel() const;
  unsigned int getImmediate() const;
  unsigned short getPredicate() const;
  bool endOfPacket() const;

/* Constructors and destructors */
  Instruction();
  Instruction(const Word& other);
  Instruction(unsigned int inst);   // Instructions will probably be read in binary
  Instruction(const std::string &inst);    // In case we end up reading assembler
  virtual ~Instruction();

/* Methods */
  bool operator== (const Instruction& other) const;

  // Has to go in header
  friend std::ostream& operator<< (std::ostream& os, Instruction const& v) {
    os << v.getOp() <<" "<< v.getDest() <<" "<< v.getSrc1() <<" "<< v.getSrc2()
       <<" "<< v.getImmediate() <<" "<< v.getRchannel();
    return os;
  }

private:
  void setOp(short val);
  void setDest(short val);
  void setSrc1(short val);
  void setSrc2(short val);
  void setRchannel(short val);
  void setImmediate(unsigned int val);
  void setPredicate(short val);

/* String manipulation methods */
  std::vector<std::string>& split(const std::string &s, char delim,
                                  std::vector<std::string> &elems);
  std::vector<std::string> split(const std::string &s, char delim);
  int strToInt(const std::string &s);
  void decode(const std::string &s, int field);

};

#endif /* INSTRUCTION_H_ */
