/*
 * CreditReturn.h
 *
 * Simple network taking credits from a tiles router and sending them to the
 * appropriate core.
 *
 *  Created on: 29 Mar 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_CREDITRETURN_H_
#define SRC_TILE_NETWORK_CREDITRETURN_H_

#include "../../Network/Network.h"

class CreditReturn: public Network<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Crossbar:
//
//  ClockInput   clock;
//
//  LokiVector<InPort>  inputs;
//  LokiVector<OutPort> outputs;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:
  CreditReturn(const sc_module_name name,
               const tile_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

protected:
  virtual PortIndex getDestination(const ChannelID address) const;

};

#endif /* SRC_TILE_NETWORK_CREDITRETURN_H_ */
