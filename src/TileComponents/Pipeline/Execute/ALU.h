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
#include "../../../Utility/InstructionMap.h"

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
  void execute(DecodedInst& operation);

  // Tell whether an operation is currently in progress. No further operations
  // can be issued if so.
  bool busy() const;

  // Carry out a system call. All system calls are currently instant.
  void systemCall(DecodedInst& dec) const;

private:

  // Set cyclesRemaining to the appropriate value for this operation.
  cycle_count_t getFunctionLatency(function_t fn);

  void setPredicate(bool val) const;

  int32_t readReg(RegisterIndex reg) const;
  int32_t readWord(MemoryAddr addr) const;
  int32_t readByte(MemoryAddr addr) const;

  void writeReg(RegisterIndex reg, Word data) const;
  void writeWord(MemoryAddr addr, Word data) const;
  void writeByte(MemoryAddr addr, Word data) const;

  ExecuteStage* parent() const;

  uint convertTargetFlags(uint tflags) const;

//==============================//
// Methods
//==============================//

private:

  // Allow multi-cycle operations. Stall the pipeline until they are complete.
  cycle_count_t cyclesRemaining;

};

#endif /* ALU_H_ */
