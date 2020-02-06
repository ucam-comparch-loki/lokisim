/*
 * ForwardingNetwork.h
 *
 *  Created on: Jan 31, 2020
 *      Author: db434
 */

#ifndef SRC_TILE_CORE_FORWARDINGNETWORK_H_
#define SRC_TILE_CORE_FORWARDINGNETWORK_H_

#include "../../ISA/InstructionDecode.h"
#include "../../LokiComponent.h"

#include <list>
#include <utility>
using std::list;
using std::pair;

namespace Compute {

class ForwardingNetwork: public LokiComponent {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(ForwardingNetwork);
  ForwardingNetwork(sc_module_name name, size_t numRegisters);

//============================================================================//
// Methods
//============================================================================//

public:

  // Add an instruction which produces a result. The instruction will be
  // removed automatically when it writes to the register file.
  void addProducer(DecodedInstruction producer);

  // Return whether the given instruction needs to use the forwarding network
  // to get the specified operand.
  bool requiresForwarding(DecodedInstruction consumer, PortIndex port) const;

  // Add an instruction which receives an operand. The instruction will be
  // removed automatically when all operands have arrived.
  void addConsumer(DecodedInstruction consumer, PortIndex port);

private:

  // Method triggered whenever a producer produces a result.
  void resultProduced(RegisterIndex reg);

  // Method triggered whenever a producer writes its result to the register
  // file.
  void resultWritten(RegisterIndex reg);

  // Track the progress of a producer and remove it from the local state when
  // necessary.
  void removeProducer(RegisterIndex reg);

  // Track the progress of a consumer and remove it from the local state when
  // necessary.
  void removeConsumer(RegisterIndex reg);

//============================================================================//
// Local state
//============================================================================//

private:

  // One instruction per register, which has not yet written its result to the
  // register file.
  // Indexed using producers[register].
  vector<DecodedInstruction> producers;
  vector<sc_core::sc_event> newProducer;

  // A list of consumers waiting for each register.
  // Indexed using consumers[register].
  typedef pair<DecodedInstruction, PortIndex> consumer_t;
  vector<list<consumer_t>> consumers;

};

} /* namespace Compute */

#endif /* SRC_TILE_CORE_FORWARDINGNETWORK_H_ */
