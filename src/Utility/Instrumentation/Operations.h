/*
 * Operations.h
 *
 *  Created on: 17 Jun 2010
 *      Author: db434
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_

#include "InstrumentationBase.h"
#include "CounterMap.h"
#include "../../Datatype/ComponentID.h"
#include "../../Datatype/DecodedInst.h"

// Would prefer this to be private.
// Uniquely identify an operation through both its opcode and ALU function.
struct operation_key {
  opcode_t   opcode;
  function_t function;

  bool operator <(const operation_key& rhs) const {
    return (opcode < rhs.opcode) ||
           (opcode == rhs.opcode && function < rhs.function);
  }
};

namespace Instrumentation {

class Operations: public InstrumentationBase {

public:

  static void decoded(const ComponentID& core, const DecodedInst& dec);
  static void executed(const ComponentID& core, const DecodedInst& dec, bool executed);

  static unsigned long long numDecodes();
  static unsigned long long numOperations();
  static unsigned long long numOperations(opcode_t operation,
                                          function_t function = (function_t)0);

  static void printStats();

private:

  static CounterMap<operation_key> executedOps;
  static CounterMap<operation_key> unexecutedOps;

  // Is there a difference between numOps and numDecodes?
  static unsigned long long numOps_, numDecodes_;

};

}

#endif /* OPERATIONS_H_ */
