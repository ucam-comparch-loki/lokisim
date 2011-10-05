/*
 * ALU.h
 *
 * Arithmetic Logic Unit.
 *
 *  Created on: 14 Jan 2010
 *      Author: db434
 */

#ifndef ALU_H_
#define ALU_H_

#include "../../../Component.h"

class DecodedInst;
class ExecuteStage;
class Word;

class ALU: public Component {

//==============================//
// Constructors and destructors
//==============================//

public:

  ALU(const sc_module_name& name, const ComponentID& ID);

//==============================//
// Methods
//==============================//

public:

  // Calculate the result of the given operation, and store it in the
  // provided decoded instruction.
  void execute(DecodedInst& operation) const;

  // Carry out a system call. All system calls are currently instant.
  void systemCall(int code) const;

private:

  void setPred(bool val) const;

  int32_t readReg(RegisterIndex reg) const;
  int32_t readWord(MemoryAddr addr) const;
  int32_t readByte(MemoryAddr addr) const;

  void writeReg(RegisterIndex reg, Word data) const;
  void writeWord(MemoryAddr addr, Word data) const;
  void writeByte(MemoryAddr addr, Word data) const;

  ExecuteStage* parent() const;

  uint convertTargetFlags(uint tflags) const;

};

#endif /* ALU_H_ */
