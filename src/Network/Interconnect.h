/*
 * Interconnect.h
 *
 * Base class for all types of interconnection network.
 *
 *  Created on: 15 Jan 2010
 *      Author: db434
 */

#ifndef INTERCONNECT_H_
#define INTERCONNECT_H_

#include "../Component.h"

class Interconnect: public Component {

protected:
  // Flow control?      Or are these controlled completely
  // Topology?          by a parameter?
  // virtual void route();? Or RoutingProtocol r;?

public:
  Interconnect(sc_core::sc_module_name name, int ID);
  virtual ~Interconnect();

};

#endif /* INTERCONNECT_H_ */
