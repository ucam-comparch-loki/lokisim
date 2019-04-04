/*
 * L2LToBankRequests.h
 *
 * Network allowing all memory banks in a tile to coordinate ownership of an
 * associative L2 memory request.
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_L2LTOBANKREQUESTS_H_
#define SRC_TILE_NETWORK_L2LTOBANKREQUESTS_H_

#include "../../LokiComponent.h"
#include "../../Network/NetworkTypes.h"

using sc_core::sc_event;
using sc_core::sc_interface;

// Interface for memory banks.
class l2_request_bank_ifc : virtual public sc_interface {

public:

  // Read the flit currently on the channel.
  virtual const NetworkRequest& peek() const = 0;
  virtual const NetworkRequest& read() = 0;

  // The bank responsible for handling this request if all banks miss.
  virtual MemoryIndex targetBank() const = 0;

  // Notify of a cache hit.
  virtual void cacheHit() = 0;

  // Notify of a cache miss.
  virtual void cacheMiss() = 0;

  // Returns whether one of the banks had a cache hit.
  virtual bool associativeHit() const = 0;

  // Check whether all banks have responded with their hit/miss status.
  // If the target bank has a cache miss, it is only allowed to begin a request
  // once all banks have responded.
  virtual bool allResponsesReceived() const = 0;
  virtual const sc_event& allResponsesReceivedEvent() const = 0;

  // Event triggered when the first flit of a new request arrives.
  virtual const sc_event& newRequestArrived() const = 0;

  // Event triggered whenever any flit arrives.
  virtual const sc_event& newFlitArrived() const = 0;

};

// Interface for L2 logic.
class l2_request_l2l_ifc : virtual public sc_interface {

public:

  // Clear any state and begin a new request.
  virtual void newRequest(NetworkRequest flit, MemoryIndex targetBank) = 0;

  // Supply a new flit for an existing request.
  virtual void newPayload(NetworkRequest flit) = 0;

  // Determine when it is safe to write a new flit.
  virtual bool canWrite() const = 0;
  virtual const sc_event& canWriteEvent() const = 0;

};

class L2LToBankRequests : public LokiComponent,
                          virtual public l2_request_bank_ifc,
                          virtual public l2_request_l2l_ifc {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  L2LToBankRequests(sc_module_name name, uint numBanks);

//============================================================================//
// Methods
//============================================================================//

public:

  // Read the flit currently on the channel.
  virtual const NetworkRequest& peek() const;
  virtual const NetworkRequest& read();

  // The bank responsible for handling this request if all banks miss.
  virtual MemoryIndex targetBank() const;

  // Notify of a cache hit.
  virtual void cacheHit();

  // Notify of a cache miss.
  virtual void cacheMiss();

  // Check whether all banks have responded with their hit/miss status.
  // If the target bank has a cache miss, it is only allowed to begin a request
  // once all banks have responded.
  virtual bool allResponsesReceived() const;
  virtual const sc_event& allResponsesReceivedEvent() const;

  // Returns whether one of the banks had a cache hit.
  virtual bool associativeHit() const;

  // Event triggered when the first flit of a new request arrives.
  virtual const sc_event& newRequestArrived() const;

  // Event triggered whenever any flit arrives.
  virtual const sc_event& newFlitArrived() const;

  // Clear any state and begin a new request.
  virtual void newRequest(NetworkRequest flit, MemoryIndex targetBank);

  // Supply a new flit for an existing request.
  virtual void newPayload(NetworkRequest flit);

  // Determine when it is safe to write a new flit.
  virtual bool canWrite() const;
  virtual const sc_event& canWriteEvent() const;

private:

  // Updates performed when any response is received (hit or miss).
  void addResponse();

//============================================================================//
// Local state
//============================================================================//

private:

  // The number of memory banks connected to this network.
  const uint numBanks;

  // The number of responses received for the current request. All banks must
  // respond to a request before a new one can begin.
  uint numResponses;

  // Has any bank had a cache hit?
  bool hit;

  // Has the flit been consumed? Each flit may only be consumed once.
  bool consumed;

  // The current flit being transmitted.
  NetworkRequest flit;

  // The target bank for the current request.
  MemoryIndex target;

  // Event triggered once all banks have responded.
  sc_event finalResponseReceived;

  // Event triggered when the first flit of a new request arrives.
  sc_event newRequestEvent;

  // Event triggered when any flit arrives.
  sc_event newFlitEvent;

  // Event triggered when all responses have been received and data has been
  // consumed.
  sc_event finishedWithFlit;

};



#endif /* SRC_TILE_NETWORK_L2LTOBANKREQUESTS_H_ */
