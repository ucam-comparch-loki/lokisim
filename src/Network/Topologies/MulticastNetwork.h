/*
 * MulticastNetwork.h
 *
 * Note: this network design is currently a big hack. It is just a crossbar
 * which accesses the instrumentation like a multicast network should.
 *
 * Behaviour should be similar, if not identical, to a proper multicast network.
 *
 *  Created on: 25 Jul 2012
 *      Author: db434
 */

#ifndef MULTICASTNETWORK_H_
#define MULTICASTNETWORK_H_

#include "Crossbar.h"

using sc_core::sc_module_name;

class MulticastNetwork: public Crossbar {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  // outputsPerComponent = number of this network's outputs which lead to the
  //                       same component.
  // buffersPerComponent = the number of buffers each destination component has.
  //                       There will be one flow control signal per buffer.
  MulticastNetwork(const sc_module_name& name,
                   int inputs,
                   int outputs,
                   int outputsPerComponent,
                   int buffersPerComponent);
  virtual ~MulticastNetwork();

//============================================================================//
// Methods
//============================================================================//

protected:

  virtual void inputChanged(const PortIndex port);
  virtual void outputChanged(const PortIndex port);

};

#endif /* MULTICASTNETWORK_H_ */
