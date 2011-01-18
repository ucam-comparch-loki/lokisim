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

class ALU: public Component {

//==============================//
// Constructors and destructors
//==============================//

public:

  ALU(sc_module_name name);
  virtual ~ALU();

//==============================//
// Methods
//==============================//

public:

  // Calculate the result of the given operation, and store it in the
  // provided decoded instruction. Returns false if there is no result.
  bool execute(DecodedInst& operation) const;

private:

  void setPred(bool val) const;

  // Determine whether the current instruction should be executed, based on its
  // predicate bits, and the contents of the predicate register.
  bool shouldExecute(short predBits) const;

  ExecuteStage* parent() const;

};

#endif /* ALU_H_ */
