/*
 * ResponseNetwork.h
 *
 * Network responsible for communicating data between L1 caches upon receipt of
 * a request from the RequestNetwork.
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#ifndef RESPONSENETWORK_H_
#define RESPONSENETWORK_H_

#include "../Topologies/Mesh.h"

class ResponseNetwork: public Mesh {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  ResponseNetwork(const sc_module_name &name, size2d_t size,
                  const router_parameters_t& routerParams);
  virtual ~ResponseNetwork();

};

#endif /* RESPONSENETWORK_H_ */
