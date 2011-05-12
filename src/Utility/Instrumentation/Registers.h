/*
 * Registers.h
 *
 *  Created on: 21 Jan 2011
 *      Author: db434
 */

#ifndef REGISTERS_H_
#define REGISTERS_H_

#include "../../Datatype/ComponentID.h"
#include "InstrumentationBase.h"

namespace Instrumentation {

class Registers: public InstrumentationBase {

public:

  static void read(const ComponentID& core, RegisterIndex reg);
  static void write(const ComponentID& core, RegisterIndex reg);
  static void forward(const ComponentID& core, RegisterIndex reg);
  static void stallReg(const ComponentID& core);

  static int  numReads();
  static int  numWrites();
  static int  numForwards();
  static int  stallRegUses();

  static void printStats();

private:

  static int numReads_, numWrites_, numForwards_, stallRegs_;

};

}

#endif /* REGISTERS_H_ */
