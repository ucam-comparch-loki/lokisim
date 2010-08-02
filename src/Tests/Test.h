/*
 * Test.h
 *
 * Useful definitions/constructs for use throughout all tests.
 *
 *  Created on: 20 Jan 2010
 *      Author: db434
 */

#ifndef TEST_H_
#define TEST_H_

#include <systemc.h>

#include "../Utility/Parameters.h"
#include "../Utility/TraceFile.h"
#include "../Datatype/Word.h"
#include "../Datatype/Address.h"
#include "../Datatype/Data.h"
#include "../Datatype/Instruction.h"
#include "../Datatype/AddressedWord.h"

// A small unit of simulation time, allowing signals to propagate
#define TIMESTEP {\
  static int cycleNumber = 0;\
  if(DEBUG) cout << "\n======= Cycle " << cycleNumber++ << " =======" << endl;\
  sc_start(1, SC_NS);\
}

#endif /* TEST_H_ */
