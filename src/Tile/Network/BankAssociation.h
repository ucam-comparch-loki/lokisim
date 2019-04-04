/*
 * BankAssociation.h
 *
 * Network allowing all memory banks in a tile to coordinate ownership of an
 * associative L2 memory request.
 *
 *  Created on: 3 Apr 2019
 *      Author: db434
 */

#ifndef SRC_TILE_NETWORK_BANKASSOCIATION_H_
#define SRC_TILE_NETWORK_BANKASSOCIATION_H_

#include "../../LokiComponent.h"

using sc_core::sc_event;
using sc_core::sc_interface;

// Interface for memory banks.
class association_bank_ifc : virtual public sc_interface {

public:

  // The bank responsible for handling this request if all banks miss.
  virtual MemoryIndex targetBank() const = 0;

  // Notify of a cache hit.
  virtual void cacheHit() = 0;

  // Notify of a cache miss.
  virtual void cacheMiss() = 0;

  // Check whether all banks have responded with their hit/miss status.
  // If the target bank has a cache miss, it is only allowed to begin a request
  // once all banks have responded.
  virtual bool allResponsesReceived() const = 0;
  virtual const sc_event& allResponsesReceivedEvent() const = 0;

  // Returns whether one of the banks had a cache hit.
  virtual bool associativeHit() const = 0;

};

// Interface for L2 logic.
class association_l2l_ifc : virtual public sc_interface {

public:

  // Clear any state and begin a new request.
  virtual void newRequest(MemoryIndex targetBank) = 0;

};

class BankAssociation : public LokiComponent,
                        virtual public association_bank_ifc,
                        virtual public association_l2l_ifc {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  BankAssociation(sc_module_name name, uint numBanks);

//============================================================================//
// Methods
//============================================================================//

public:

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

  // Begin a new request.
  virtual void newRequest(MemoryIndex targetBank);

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

  // The target bank for the current request.
  MemoryIndex target;

  // Event triggered once all banks have responded.
  sc_event finalResponseReceived;

};



#endif /* SRC_TILE_NETWORK_BANKASSOCIATION_H_ */
