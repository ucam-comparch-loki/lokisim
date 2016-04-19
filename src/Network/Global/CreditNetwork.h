/*
 * CreditNetwork.h
 *
 * Network to transfer credits between components on different tiles. Currently
 * connects only cores, as we are able to provide consumption guarantees for
 * all other types of data, so credits are not required.
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#ifndef CREDITNETWORK_H_
#define CREDITNETWORK_H_

#include "NetworkHierarchy2.h"

class CreditNetwork: public NetworkHierarchy2 {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  CreditNetwork(const sc_module_name &name);
  virtual ~CreditNetwork();

};

#endif /* CREDITNETWORK_H_ */
