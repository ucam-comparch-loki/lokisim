/*
 * ComputeTile.h
 *
 * A tile containing cores and memory banks.
 *
 *  Created on: 4 Oct 2016
 *      Author: db434
 */

#ifndef SRC_TILE_COMPUTETILE_H_
#define SRC_TILE_COMPUTETILE_H_

#include "Tile.h"
#include "Core/Core.h"
#include "Memory/MemoryBank.h"
#include "Memory/MissHandlingLogic.h"
#include "Network/CoreMulticast.h"
#include "Network/DataReturn.h"
#include "Network/ForwardCrossbar.h"
#include "Network/InstructionReturn.h"
#include "../Network/Global/RouterDemultiplexer.h"
#include "../Network/NetworkTypedefs.h"
#include "../Network/WormholeMultiplexer.h"

class Core;
class MemoryBank;

class ComputeTile: public Tile {

//============================================================================//
// Ports
//============================================================================//

public:

// Inherited from Tile:
//
//  ClockInput      clock;

  // Some extra events to keep the internal networks in sync.
  ClockInput      fastClock;
  ClockInput      slowClock;

//  // Data network.
//  DataInput       iData;
//  DataOutput      oData;
//  ReadyInput      iDataReady;
//  ReadyOutput     oDataReady;
//
//  // Credit network.
//  CreditInput     iCredit;
//  CreditOutput    oCredit;
//  ReadyInput      iCreditReady;
//  ReadyOutput     oCreditReady;
//
//  // Memory request network.
//  RequestInput    iRequest;
//  RequestOutput   oRequest;
//  ReadyInput      iRequestReady;
//  ReadyOutput     oRequestReady;
//
//  // Memory response network.
//  ResponseInput   iResponse;
//  ResponseOutput  oResponse;
//  ReadyInput      iResponseReady;
//  ReadyOutput     oResponseReady;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  ComputeTile(const sc_module_name& name, const ComponentID& id);
  virtual ~ComputeTile();

//============================================================================//
// Methods
//============================================================================//

public:

  // Store the given instructions or data into the component at the given index.
  virtual void storeInstructions(vector<Word>& instructions, const ComponentID& component);
  virtual void storeData(const DataBlock& data);

  // Dump the contents of a component to stdout.
  virtual void print(const ComponentID& component, MemoryAddr start, MemoryAddr end);

  // Debug access methods which bypass all networks and have zero latency.
  virtual Word readWordInternal(const ComponentID& component, MemoryAddr addr);
  virtual Word readByteInternal(const ComponentID& component, MemoryAddr addr);
  virtual void writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data);
  virtual void writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data);
  virtual int  readRegisterInternal(const ComponentID& component, RegisterIndex reg) const;
  virtual bool readPredicateInternal(const ComponentID& component) const;
  virtual void networkSendDataInternal(const NetworkData& flit);
  virtual void networkSendCreditInternal(const NetworkCredit& flit);

  // Each tile modifies the address of outgoing memory addresses. Determine the
  // ultimate address in main memory that this address maps to.
  MemoryAddr getAddressTranslation(MemoryAddr address) const;

  // Determines whether this address is a cached copy of a value from main
  // memory. The alternative is that there is a scratchpad somewhere in the
  // memory hierarchy.
  bool    backedByMainMemory(MemoryAddr address) const;


  // Some magical network methods which prevent the need to connect thousands
  // of single-bit wires.

  // Issue a request for arbitration. This should only be called for the first
  // and last flits of each packet.
  void makeRequest(ComponentID source, ChannelID destination, bool request);

  // See if the request from source to destination has been granted.
  bool requestGranted(ComponentID source, ChannelID destination) const;

private:

  void makeSignals();
  void makeComponents();
  void wireUp();

//============================================================================//
// Components
//============================================================================//

private:

  vector<Core*>             cores;
  vector<MemoryBank*>       memories;
  MissHandlingLogic         mhl;

  friend class Core;
  friend class MemoryBank;
  friend class MissHandlingLogic;

  // Subnetworks.
  CoreMulticast             coreToCore;
  ForwardCrossbar           coreToMemory;
  DataReturn                dataReturn;
  InstructionReturn         instructionReturn;

  WormholeMultiplexer<Word> dataToRouter;
  RouterDemultiplexer<Word> dataFromRouter;
  WormholeMultiplexer<Word> creditToRouter;
  RouterDemultiplexer<Word> creditFromRouter;

//============================================================================//
// Signals (wires)
//============================================================================//

private:

  LokiVector<DataSignal>    dataToCores,              dataFromMemory,
                            instructionsToCores,      instructionsFromMemory,
                            multicastFromCores;
  LokiVector2D<DataSignal>  multicastToCores;
  LokiVector<RequestSignal> requestsToMemory,         requestsFromCores;

  LokiVector2D<ReadySignal> readyDataFromCores,       readyDataFromMemory;

  LokiVector<CreditSignal>  creditsToCores,           creditsFromCores;
  LokiVector2D<ReadySignal> readyCreditFromCores;

  // TODO remove global signals and combine with coreToMemory and memoryToCores.
  LokiVector<DataSignal>    globalDataToCores,        globalDataFromCores;

  // Signals allowing arbitration requests to be made for cores/memories/routers.
  // Currently the signals are written using a function call, but they can
  // be removed if we set up a proper SystemC channel connection.
  // Addressed using coreRequests[requester][destination]
  LokiVector2D<ArbiterRequestSignal> coreToMemRequests,
                                     dataReturnRequests, instructionReturnRequests;
  LokiVector2D<ArbiterGrantSignal>   coreToMemGrants,
                                     dataReturnGrants,   instructionReturnGrants;

  LokiVector<RequestSignal> l2RequestFromMemory;
  RequestSignal             l2RequestToMemory;
  sc_signal<MemoryIndex>    l2RequestTarget;
  LokiVector<sc_signal<bool>> l2ClaimRequest;
  sc_signal<bool>           l2RequestClaimed;

  LokiVector<ResponseSignal> l2ResponseFromMemory;
  ResponseSignal            l2ResponseToMemory;
  sc_signal<MemoryIndex>    l2ResponseTarget;

};

#endif /* SRC_TILE_COMPUTETILE_H_ */
