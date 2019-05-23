/*
 * BankToMHLRequests.h
 *
 * Simple network connecting all banks of a tile with the miss handling logic
 * of the same tile.
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_BANKTOMHLREQUESTS_H_
#define SRC_TILE_NETWORK_BANKTOMHLREQUESTS_H_

#include "../../Network/Network.h"

class BankToMHLRequests: public Network<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput clock;
//
//  LokiVector<InPort> inputs;    // One per memory
//  LokiVector<OutPort> outputs;  // Only one port

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  BankToMHLRequests(const sc_module_name name, const tile_parameters_t& params);
  virtual ~BankToMHLRequests();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual PortIndex getDestination(const ChannelID address) const;
};

#endif /* SRC_TILE_NETWORK_BANKTOMHLREQUESTS_H_ */
