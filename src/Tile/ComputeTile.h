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
#include "../Network/Topologies/LocalNetwork.h"
#include "../Network/Global/RouterDemultiplexer.h"
#include "../Network/NetworkTypedefs.h"
#include "../Network/WormholeMultiplexer.h"
#include "../Utility/LokiVector.h"
#include "../Utility/LokiVector2D.h"

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

  // Local network encompasses all communications between cores and memory
  // banks on this tile.
  LocalNetwork              localNetwork;

  WormholeMultiplexer<Word> dataToRouter;
  RouterDemultiplexer<Word> dataFromRouter;
  WormholeMultiplexer<Word> creditToRouter;
  RouterDemultiplexer<Word> creditFromRouter;

//============================================================================//
// Signals (wires)
//============================================================================//

private:

  LokiVector2D<DataSignal>  dataToComponents,         dataFromComponents;
  LokiVector2D<ReadySignal> readyDataFromComponents;

  LokiVector<CreditSignal>  creditsToCores,           creditsFromCores;
  LokiVector2D<ReadySignal> readyCreditFromCores;

  LokiVector<DataSignal>    globalDataToCores,        globalDataFromCores;

  LokiVector<RequestSignal> requestFromMemory;
  RequestSignal             requestToMemory;
  sc_signal<MemoryIndex>    requestTarget;
  LokiVector<sc_signal<bool>> claimRequest;
  sc_signal<bool>           requestClaimed;

  LokiVector<ResponseSignal> responseFromMemory;
  ResponseSignal            responseToMemory;
  sc_signal<MemoryIndex>    responseTarget;

};

#endif /* SRC_TILE_COMPUTETILE_H_ */
