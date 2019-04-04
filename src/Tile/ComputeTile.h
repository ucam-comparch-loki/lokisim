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
#include "../Network/NetworkTypes.h"
#include "Memory/L2Logic.h"
#include "Network/BankAssociation.h"
#include "Network/BankToL2LResponses.h"
#include "Network/BankToMHLRequests.h"
#include "Network/CreditReturn.h"
#include "Network/IntertileUnit.h"
#include "Network/L2LToBankRequests.h"
#include "Network/MHLToBankResponses.h"

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
//
//  // Data network.
//  sc_port<network_source_ifc<Word>> iData;
//  sc_port<network_sink_ifc<Word>> oData;
//
//  // Credit network.
//  sc_port<network_source_ifc<Word>> iCredit;
//  sc_port<network_sink_ifc<Word>> oCredit;
//
//  // Memory request network.
//  sc_port<network_source_ifc<Word>> iRequest;
//  sc_port<network_sink_ifc<Word>> oRequest;
//
//  // Memory response network.
//  sc_port<network_source_ifc<Word>> iResponse;
//  sc_port<network_sink_ifc<Word>> oResponse;


//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  ComputeTile(const sc_module_name& name, const TileID& id,
              const tile_parameters_t& params);

//============================================================================//
// Methods
//============================================================================//

public:

  virtual uint numComponents() const;
  virtual uint numCores() const;
  virtual uint numMemories() const;

  // Get information about the given Identifiers based on the configuration of
  // the Tile.
  virtual bool isCore(ComponentID id) const;
  virtual bool isMemory(ComponentID id) const;
  virtual uint componentIndex(ComponentID id) const;
  virtual uint coreIndex(ComponentID id) const;
  virtual uint memoryIndex(ComponentID id) const;

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

  void makeComponents(const tile_parameters_t& params);
  void wireUp(const tile_parameters_t& params);

  // Allow components to find their position on the chip. Only used for debug.
  uint globalCoreIndex(ComponentID id) const;
  uint globalMemoryIndex(ComponentID id) const;

//============================================================================//
// Components
//============================================================================//

private:

  LokiVector<Core>          cores;
  LokiVector<MemoryBank>    memories;
  MissHandlingLogic         mhl;
  L2Logic                   l2l;
  IntertileUnit             icu;

  friend class Core;
  friend class MemoryBank;
  friend class MissHandlingLogic;
  friend class L2Logic;
  friend class IntertileUnit;

  // Subnetworks.
  CoreMulticast             coreToCore;
  ForwardCrossbar           coreToMemory;
  DataReturn                dataReturn;
  InstructionReturn         instructionReturn;
  CreditReturn              creditReturn;
  BankToMHLRequests         bankToMHLRequests;
  MHLToBankResponses        mhlToBankResponses;
  L2LToBankRequests         l2lToBankRequests;
  BankToL2LResponses        bankToL2LResponses;
  BankAssociation           bankAssociation;

  // Need to implement the appropriate interfaces to connect the global credit
  // network with the local one. No other networks need this because their
  // buffers are inside other units (e.g. ICU, MHL).
  NetworkFIFO<Word>         creditBuffer;

};

#endif /* SRC_TILE_COMPUTETILE_H_ */
