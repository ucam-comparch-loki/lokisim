/*
 * InstructionReturn.h
 *
 * Network returning instructions from memory banks to cores.
 *
 *  Created on: 13 Dec 2016
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_INSTRUCTIONRETURN_H_
#define SRC_TILE_NETWORK_INSTRUCTIONRETURN_H_

#include "../../Network/Network2.h"

// TODO: Extend from Network<Instruction> instead?
class InstructionReturn: public Network2<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput clock;
//
//  LokiVector<InPort> inputs;    // One per core
//  LokiVector<OutPort> outputs;  // One per core input buffer
//                                // Numbered core*(buffers per core) + buffer

//============================================================================//
// Constructors and destructors
//============================================================================//

public:
  InstructionReturn(const sc_module_name name,
                    const tile_parameters_t& params);
  virtual ~InstructionReturn();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual PortIndex getDestination(const ChannelID address) const;

//============================================================================//
// Local state
//============================================================================//

private:

  // The number of output ports which lead to the same core.
  const uint outputsPerCore;

  // Number of cores reachable through outputs.
  const uint outputCores;

};

#endif /* SRC_TILE_NETWORK_INSTRUCTIONRETURN_H_ */
