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
#include "../../Memory/BufferStorage.h"

class RequestNetwork: public NetworkHierarchy2 {

//==============================//
// Ports
//==============================//

public:

  // Requests to main memory.
  RequestOutput oMainMemoryRequest;

  // Flow control from main memory.
  ReadyInput    iMainMemoryReady;

  // Responses from main memory.
  RequestInput  iMainMemoryRequest;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(RequestNetwork);
  RequestNetwork(const sc_module_name &name);
  virtual ~RequestNetwork();

//==============================//
// Methods
//==============================//

private:

  // Collect any messages which go off the edges of the mesh network, and
  // funnel them all into the single buffer, ready to go to main memory.
  // TODO: have multiple ports.
  void collectOffchipRequests(RequestSignal* signal);

  // Send requests from the buffer to main memory.
  void requestsToMemory();

  // Send requests from main memory onto the on-chip network.
  void requestsToNetwork();

//==============================//
// Components
//==============================//

  // Use a buffer as a serialisation point before sending requests on to main
  // memory. In practice, the network would be shaped such that no
  // serialisation is required, so this buffer wouldn't be needed.
  BufferStorage<NetworkRequest> toMainMemory;

};

#endif /* REQUESTNETWORK_H_ */
