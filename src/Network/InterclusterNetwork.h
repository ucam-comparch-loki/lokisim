/*
 * InterclusterNetwork.h
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#ifndef INTERCLUSTERNETWORK_H_
#define INTERCLUSTERNETWORK_H_

#include "Interconnect.h"

class InterclusterNetwork: public Interconnect {

  // vector in/out (in1,in2,out1,out2,out3,out4)?

public:
  InterclusterNetwork(sc_core::sc_module_name name, int ID);
  virtual ~InterclusterNetwork();
};

#endif /* INTERCLUSTERNETWORK_H_ */
