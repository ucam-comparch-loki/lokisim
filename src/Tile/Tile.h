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

#include <vector>
#include "../LokiComponent.h"
#include "../Network/Interface.h"
#include "../Network/NetworkTypes.h"

using sc_core::sc_module_name;
using std::vector;

class Chip;
class DataBlock;

class Tile: public LokiComponent {

//============================================================================//
// Ports
//============================================================================//

public:

  ClockInput      clock;

  // All ports are sinks of one network or another. The input ports are sinks or
  // the global network, and the outgoing ports are sinks of the local network.

  // Data network.
  sc_port<network_sink_ifc<Word>> iData;
  sc_port<network_sink_ifc<Word>> oData;

  // Credit network.
  sc_port<network_sink_ifc<Word>> iCredit;
  sc_port<network_sink_ifc<Word>> oCredit;

  // Memory request network.
  sc_port<network_sink_ifc<Word>> iRequest;
  sc_port<network_sink_ifc<Word>> oRequest;

  // Memory response network.
  sc_port<network_sink_ifc<Word>> iResponse;
  sc_port<network_sink_ifc<Word>> oResponse;

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  Tile(const sc_module_name& name, const TileID id);
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

//============================================================================//
// Local state
//============================================================================//

public:

  TileID id;

};

#endif /* SRC_TILE_TILE_H_ */
