/*
 * DataNetwork.h
 *
 * Network which carries data between cores on different tiles. Data sent to
 * cores on the same tile uses a separate, multicast-capable network.
 *
 *  Created on: 15 Jul 2014
 *      Author: db434
 */

#ifndef DATANETWORK_H_
#define DATANETWORK_H_

#include "NetworkHierarchy2.h"

class DataNetwork: public NetworkHierarchy2 {

//==============================//
// Constructors and destructors
//==============================//

public:

  DataNetwork(const sc_module_name &name);
  virtual ~DataNetwork();

};

#endif /* DATANETWORK_H_ */
