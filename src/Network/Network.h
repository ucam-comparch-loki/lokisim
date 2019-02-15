/*
 * Network.h
 *
 * Base class of all networks.
 *
 *  Created on: 2 Nov 2010
 *      Author: db434
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "../LokiComponent.h"
#include "NetworkTypes.h"

class Network : public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  // No clock is provided by default. It is the responsibility of any network
  // inputs to limit themselves to sending data at the appropriate times and
  // rates.
  // ClockInput   clock;

  // No data ports are provided by default. This allows any of Network's
  // subclasses to structure ports in the most convenient way (e.g. multi-
  // dimensional array).
  // LokiVector<DataInput>  iData;
  // LokiVector<DataOutput> oData;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Network(const sc_module_name& name);

//============================================================================//
// Methods
//============================================================================//

protected:

  // Compute which output of this network will be used by the given address.
  PortIndex getDestination(ChannelID address) const;

};

#endif /* NETWORK_H_ */
