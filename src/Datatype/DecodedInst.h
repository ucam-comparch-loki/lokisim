/*
 * DecodedInst.h
 *
 *  Created on: 11 Sep 2010
 *      Author: daniel
 */

#ifndef DECODEDINST_H_
#define DECODEDINST_H_

#include <inttypes.h>
#include "../Typedefs.h"
#include "systemc.h"
#include "Address.h"

class Instruction;

class DecodedInst {

//==============================//
// Methods
//==============================//

public:

  uint8_t operation() const;
  RegisterIndex sourceReg1() const;
  RegisterIndex sourceReg2() const;
  RegisterIndex destination() const;
  int32_t immediate() const;
  ChannelIndex channelMapEntry() const;
  uint8_t predicate() const;
  bool    setsPredicate() const;
  uint8_t memoryOp() const;

  int32_t operand1() const;
  int32_t operand2() const;
  int64_t result() const;
  Address location() const;

  bool    usesPredicate() const;
  bool    hasOperand1() const;
  bool    hasResult() const;

  void    operation(const uint8_t val);
  void    sourceReg1(const RegisterIndex val);
  void    sourceReg2(const RegisterIndex val);
  void    destination(const RegisterIndex val);
  void    immediate(const int32_t val);
  void    channelMapEntry(const ChannelIndex val);
  void    predicate(const uint8_t val);
  void    setsPredicate(const bool val);
  void    memoryOp(const uint8_t val);

  void    operand1(const int32_t val);
  void    operand2(const int32_t val);
  void    result(const int64_t val);
  void    location(const Address val);

  Instruction toInstruction() const;

  friend void sc_trace(sc_core::sc_trace_file*& tf, const DecodedInst& i, const std::string& txt) {
    sc_core::sc_trace(tf, i.operation_,       txt + ".operation");
    sc_core::sc_trace(tf, i.destReg_,         txt + ".rd");
    sc_core::sc_trace(tf, i.sourceReg1_,      txt + ".rs");
    sc_core::sc_trace(tf, i.sourceReg2_,      txt + ".rt");
    sc_core::sc_trace(tf, i.immediate_,       txt + ".immediate");
    sc_core::sc_trace(tf, i.channelMapEntry_, txt + ".channel");
    sc_core::sc_trace(tf, i.predicate_,       txt + ".predicate");
    sc_core::sc_trace(tf, i.setsPred_,        txt + ".set_predicate");

    sc_core::sc_trace(tf, i.operand1_,        txt + ".operand1");
    sc_core::sc_trace(tf, i.operand2_,        txt + ".operand2");
    sc_core::sc_trace(tf, i.result_,          txt + ".result");
  }

  bool operator== (const DecodedInst& other) const;

  DecodedInst& operator= (const DecodedInst& other);

  friend std::ostream& operator<< (std::ostream& os, DecodedInst const& v) {
    return v.print(os);
  }

private:

  // Holds implementation of the << operator, so it doesn't have to go in the
  // header.
  std::ostream& print(std::ostream& os) const;

//==============================//
// Constructors and destructors
//==============================//

public:

  DecodedInst();
  DecodedInst(Instruction i);
  virtual ~DecodedInst();

//==============================//
// Local state
//==============================//

private:

  uint8_t operation_;
  RegisterIndex sourceReg1_;
  RegisterIndex sourceReg2_;
  RegisterIndex destReg_;
  int32_t immediate_;
  ChannelIndex channelMapEntry_;
  uint8_t predicate_;
  bool    setsPred_;
  uint8_t memoryOp_;

  int32_t operand1_;
  int32_t operand2_;
  int64_t result_;    // May be an instruction

  Address location_;  // The position in memory that this instruction comes from.

  // Use to determine whether fields have already been set.
  // Can't just use != 0 because they may have been set to 0.
  bool    hasOperand1_;
  bool    hasResult_;

};

#endif /* DECODEDINST_H_ */
