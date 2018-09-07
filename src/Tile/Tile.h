/*
 * Tile.h
 *
 * Base class for all types of tile on the Loki chip.
 *
 * Exposes an interface to each of the global networks on chip. There are no
 * routers in the Tile component, however; this is left to the global networks.
 *
 * All tiles must have the same interface, but particular networks may be unused
 * on particular tiles.
 *
 *  Created on: 4 Oct 2016
 *      Author: db434
 */

#ifndef SRC_TILE_TILE_H_
#define SRC_TILE_TILE_H_

#include "../LokiComponent.h"
#include "../Network/NetworkTypes.h"

class Chip;
class DataBlock;

class Tile: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput      clock;

  // Data network.
  DataInput       iData;
  DataOutput      oData;
  ReadyInput      iDataReady;
  ReadyOutput     oDataReady;

  // Credit network.
  CreditInput     iCredit;
  CreditOutput    oCredit;
  ReadyInput      iCreditReady;
  ReadyOutput     oCreditReady;

  // Memory request network.
  RequestInput    iRequest;
  RequestOutput   oRequest;
  ReadyInput      iRequestReady;
  ReadyOutput     oRequestReady;

  // Memory response network.
  ResponseInput   iResponse;
  ResponseOutput  oResponse;
  ReadyInput      iResponseReady;
  ReadyOutput     oResponseReady;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Tile(const sc_module_name& name, const ComponentID& id);
  virtual ~Tile();

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

  bool isComputeTile(TileID id) const;

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

  virtual void magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload = 0);


protected:

  // Return a pointer to the chip to which this tile belongs.
  Chip& chip() const;

};

#endif /* SRC_TILE_TILE_H_ */
