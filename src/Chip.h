/*
 * Chip.h
 *
 * A class encapsulating a local group of Clusters and memories (collectively
 * known as TileComponents) to reduce the amount of global communication. Each
 * Tile is connected to its neighbours by a group of input and output ports at
 * each edge. It is up to the Tile to decide how routing is implemented.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef CHIP_H_
#define CHIP_H_

#include <set>
#include <vector>

#include "LokiComponent.h"
#include "Network/Global/CreditNetwork.h"
#include "Network/Global/DataNetwork.h"
#include "Network/Global/RequestNetwork.h"
#include "Network/Global/ResponseNetwork.h"
#include "Network/NetworkTypes.h"
#include "OffChip/MagicMemory.h"
#include "OffChip/MainMemory.h"
#include "Types.h"
#include "Utility/LokiVector2D.h"

using std::vector;

class DataBlock;
class Tile;

class Chip : public LokiComponent {

//============================================================================//
// Constructors and destructors
//============================================================================//

public:

  SC_HAS_PROCESS(Chip);
  Chip(const sc_module_name& name, const ComponentID& ID);
  virtual ~Chip();

//============================================================================//
// Methods
//============================================================================//

public:

  bool    isIdle() const;

  // During initialisation, determine the closest memory controller to the
  // given tile. This must happen after makeComponents().
  TileID  nearestMemoryController(TileID tile) const;

  // Store the given instructions or data into the component at the given index.
  void    storeInstructions(vector<Word>& instructions, const ComponentID& component);
  void    storeData(const DataBlock& data);

  void    print(const ComponentID& component, MemoryAddr start, MemoryAddr end);
  Word    readWordInternal(const ComponentID& component, MemoryAddr addr);
  Word    readByteInternal(const ComponentID& component, MemoryAddr addr);
  void    writeWordInternal(const ComponentID& component, MemoryAddr addr, Word data);
  void    writeByteInternal(const ComponentID& component, MemoryAddr addr, Word data);
  int     readRegisterInternal(const ComponentID& component, RegisterIndex reg) const;
  bool    readPredicateInternal(const ComponentID& component) const;
  void    networkSendDataInternal(const NetworkData& flit);
  void    networkSendCreditInternal(const NetworkCredit& flit);

  // Each tile modifies the address of outgoing memory addresses. Determine the
  // ultimate address in main memory that this address maps to.
  MemoryAddr getAddressTranslation(TileID tile, MemoryAddr address) const;

  // Determines whether this address is a cached copy of a value from main
  // memory. The alternative is that there is a scratchpad somewhere in the
  // memory hierarchy.
  bool    backedByMainMemory(TileID tile, MemoryAddr address) const;

  void    magicMemoryAccess(MemoryOpcode opcode, MemoryAddr address, ChannelID returnChannel, Word payload = 0);

private:

  Tile*   getTile(TileID tile) const;

  // For initialisation only. Allows other components to be initialised based
  // on the properties of the memoryControllerPositions set.
  const std::set<TileID> getMemoryControllerPositions() const;

  // Make any necessary wires needed to connect components together.
  void    makeSignals();

  // Make all cores, memories and interconnect modules.
  void    makeComponents();

  // Wire everything together.
  void    wireUp();

//============================================================================//
// Local state
//============================================================================//

private:

  const std::set<TileID> memoryControllerPositions;

//============================================================================//
// Components
//============================================================================//

private:

  // Access using tiles[column][row].
  vector<vector<Tile*>>      tiles;
  MainMemory                 mainMemory;

  MagicMemory                magicMemory;       // Wrapper for mainMemory

  // Global networks - connect all tiles together.
  DataNetwork                dataNet;
  CreditNetwork              creditNet;
  RequestNetwork             requestNet;
  ResponseNetwork            responseNet;

  friend class Tile;
  friend class ComputeTile;

//============================================================================//
// Signals (wires)
//============================================================================//

private:

  sc_clock clock;

  // Naming of signals is relative to the tiles: iData is a data signal
  // which is an input to a tile.

  // Addressed using array[tileX][tileY]
  LokiVector2D<DataSignal>     iData,          oData;
  LokiVector2D<ReadySignal>    iDataReady,     oDataReady;
  LokiVector2D<CreditSignal>   iCredit,        oCredit;
  LokiVector2D<ReadySignal>    iCreditReady,   oCreditReady;
  LokiVector2D<RequestSignal>  iRequest,       oRequest;
  LokiVector2D<ReadySignal>    iRequestReady,  oRequestReady;
  LokiVector2D<ResponseSignal> iResponse,      oResponse;
  LokiVector2D<ReadySignal>    iResponseReady, oResponseReady;

  // Some extra connections for each memory port.
  LokiVector<RequestSignal>  requestToMainMemory;
  LokiVector<ResponseSignal> responseFromMainMemory;

  // Delays in SystemC slow simulation right down, so instead, make separate
  // clocks. The fast clock has its negative edge 1/4 of a cycle early, and the
  // slow clock has its negative edge 1/4 of a cycle late. The positive edges
  // are at the same time as the normal clock.
  // These are used for the small input crossbar in each core, to ensure that
  // credits get to the local tile network in time to be sent, and to allow
  // time for data to arrive from the tile network.
  sc_clock fastClock, slowClock;

};

#endif /* CHIP_H_ */
