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
#include "../../../Datatype/Data.h"
#include "../../../Datatype/DecodedInst.h"

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
  bool execute(DecodedInst& operation);

private:

  void setPred(bool val);

  // Determine whether the current instruction should be executed, based on its
  // predicate bits, and the contents of the predicate register.
  bool shouldExecute(short predBits);

  ExecuteStage* parent() const;

//==============================//
// Local state
//==============================//

private:

  int64_t lastResult;
  uint8_t lastDestination;

};

#endif /* ALU_H_ */
