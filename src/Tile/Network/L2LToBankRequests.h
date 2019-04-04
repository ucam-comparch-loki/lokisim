/*
 * L2LToBankRequests.h
 *
 * Memory requests broadcast from the L2 logic to a tile's memory banks.
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_L2LTOBANKREQUESTS_H_
#define SRC_TILE_NETWORK_L2LTOBANKREQUESTS_H_

#include "../../Network/Network.h"

using std::set;

class L2LToBankRequests: public Network<Word> {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Network:
//
//  ClockInput clock;
//
//  LokiVector<InPort> inputs;    // Only one port
//  LokiVector<OutPort> outputs;  // One per memory

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  L2LToBankRequests(const sc_module_name name, const tile_parameters_t& params);
  virtual ~L2LToBankRequests();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual PortIndex getDestination(const ChannelID address) const;
  virtual set<PortIndex> getDestinations(const ChannelID address) const;

  // The memory banks have tight synchronisation, so this network only accepts
  // new data when all banks are able to receive it.
  virtual void newData(PortIndex input);

//============================================================================//
// Local state
//============================================================================//

private:

  // The network destinations are always the same when broadcasting, so store a
  // copy.
  set<PortIndex> allOutputs;

};

#endif /* SRC_TILE_NETWORK_L2LTOBANKREQUESTS_H_ */
