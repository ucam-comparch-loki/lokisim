/*
 * IntertileNetwork.h
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#ifndef INTERTILENETWORK_H_
#define INTERTILENETWORK_H_

#include "Interconnect.h"

class IntertileNetwork: public Interconnect {

  // vector in/out north/east/south/west

public:
  IntertileNetwork(sc_core::sc_module_name name, int ID);
  virtual ~IntertileNetwork();
};

#endif /* INTERTILENETWORK_H_ */
