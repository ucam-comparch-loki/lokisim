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

#include "NetworkHierarchy2.h"
#include "../../Memory/BufferStorage.h"

class ResponseNetwork: public NetworkHierarchy2 {

//==============================//
// Ports
//==============================//

public:

  // Responses to main memory.
  ResponseOutput oMainMemoryResponse;

  // Flow control from main memory.
  ReadyInput     iMainMemoryReady;

  // Responses from main memory.
  ResponseInput  iMainMemoryResponse;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(ResponseNetwork);
  ResponseNetwork(const sc_module_name &name);
  virtual ~ResponseNetwork();

//==============================//
// Methods
//==============================//

private:

  // Collect any messages which go off the edges of the mesh network, and
  // funnel them all into the single buffer, ready to go to main memory.
  // TODO: have multiple ports.
  void collectOffchipResponses(ResponseSignal* signal);

  // Send requests from the buffer to main memory.
  void responsesToMemory();

  // Send responses from main memory onto the on-chip network.
  void responsesToNetwork();

//==============================//
// Components
//==============================//

  // Use a buffer as a serialisation point before sending responses on to main
  // memory. In practice, the network would be shaped such that no
  // serialisation is required, so this buffer wouldn't be needed.
  BufferStorage<NetworkResponse> toMainMemory;

};

#endif /* RESPONSENETWORK_H_ */
