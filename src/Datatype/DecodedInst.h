/*
 * DecodedInst.h
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#ifndef DECODEDINST_H_
#define DECODEDINST_H_

#include "Instruction.h"

class DecodedInst {

//==============================//
// Methods
//==============================//

public:

  uint8_t getOperation() const;
  uint8_t getSource1() const;
  uint8_t getSource2() const;
  uint8_t getDestination() const;
  int32_t getImmediate() const;
  uint8_t getChannelMap() const;
  uint8_t getPredicate() const;
  bool    getSetPredicate() const;

  int32_t getOperand1() const;
  int32_t getOperand2() const;
  int32_t getResult() const;

  void    setOperation(uint8_t val);
  void    setSource1(uint8_t val);
  void    setSource2(uint8_t val);
  void    setDestination(uint8_t val);
  void    setImmediate(int32_t val);
  void    setChannelMap(uint8_t val);
  void    setPredicate(uint8_t val);
  void    setSetPredicate(bool val);

  void    setOperand1(int32_t val);
  void    setOperand2(int32_t val);
  void    setResult(int32_t val);

  friend void sc_trace(sc_core::sc_trace_file*& tf, const DecodedInst& i, const std::string& txt) {
    sc_core::sc_trace(tf, i.operation, txt + ".operation");
    sc_core::sc_trace(tf, i.destReg, txt + ".rd");
    sc_core::sc_trace(tf, i.sourceReg1, txt + ".rs");
    sc_core::sc_trace(tf, i.sourceReg2, txt + ".rt");
    sc_core::sc_trace(tf, i.immediate, txt + ".immediate");
    sc_core::sc_trace(tf, i.channelMapEntry, txt + ".channel");
    sc_core::sc_trace(tf, i.predicate, txt + ".predicate");
    sc_core::sc_trace(tf, i.setPred, txt + ".set_predicate");

    sc_core::sc_trace(tf, i.operand1, txt + ".operand1");
    sc_core::sc_trace(tf, i.operand2, txt + ".operand2");
    sc_core::sc_trace(tf, i.result, txt + ".result");
  }

  bool operator== (const DecodedInst& other) const;

  DecodedInst& operator= (const DecodedInst& other);

  friend std::ostream& operator<< (std::ostream& os, DecodedInst const& v) {
    // Do nothing? Cast to instruction and print that?
    return os;
  }

//==============================//
// Constructors and destructors
//==============================//

public:

  DecodedInst(Instruction i);
  virtual ~DecodedInst();

//==============================//
// Local state
//==============================//

private:

  uint8_t operation;
  uint8_t sourceReg1;
  uint8_t sourceReg2;
  uint8_t destReg;
  int32_t immediate;
  uint8_t channelMapEntry;
  uint8_t predicate;
  bool    setPred;

  int32_t operand1;
  int32_t operand2;
  int32_t result;


};

#endif /* DECODEDINST_H_ */
