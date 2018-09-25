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

#include "../../Network/Topologies/Crossbar.h"

class InstructionReturn: public Crossbar {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Crossbar:
//
//  ClockInput   clock;
//
//  LokiVector<DataInput>  iData;
//  LokiVector<DataOutput> oData;
//
//  // A request/grant signal for each input to reserve each output.
//  // Indexed as: iRequest[input][output]
//  LokiVector2D<ArbiterRequestInput> iRequest;
//  LokiVector2D<ArbiterGrantOutput>  oGrant;
//
//  // A signal from each buffer of each component, telling whether it is ready
//  // to receive data. Addressed using iReady[component][buffer].
//  LokiVector2D<ReadyInput>   iReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:
  InstructionReturn(const sc_module_name name, ComponentID tile,
                    const tile_parameters_t& params);
  virtual ~InstructionReturn();
};

#endif /* SRC_TILE_NETWORK_INSTRUCTIONRETURN_H_ */
