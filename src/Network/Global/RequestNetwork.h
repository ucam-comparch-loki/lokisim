/*
 * RequestNetwork.h
 *
 * Network which allows L1 caches in different tiles to coordinate and share
 * information. This network transfers requests for information, and the
 * ResponseNetwork transfers the data.
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#ifndef REQUESTNETWORK_H_
#define REQUESTNETWORK_H_

#include "NetworkHierarchy2.h"

class RequestNetwork: public NetworkHierarchy2 {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  RequestNetwork(const sc_module_name &name);
  virtual ~RequestNetwork();

};

#endif /* REQUESTNETWORK_H_ */
