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

#include "Component.h"
#include "Network/Global/CreditNetwork.h"
#include "Network/Global/ResponseNetwork.h"
#include "Network/Global/RequestNetwork.h"
#include "Network/Global/DataNetwork.h"
#include "Network/NetworkHierarchy.h"
#include "TileComponents/Core.h"
#include "TileComponents/Memory/MemoryBank.h"
#include "TileComponents/Memory/SimplifiedOnChipScratchpad.h"

using std::vector;

class DataBlock;
class MissHandlingLogic;
class TileComponent;

class Chip : public Component {

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Chip);
  Chip(const sc_module_name& name, const ComponentID& ID);
  virtual ~Chip();

//==============================//
// Methods
//==============================//

public:

  bool    isIdle() const;

  // Store the given instructions or data into the component at the given index.
  void    storeInstructions(vector<Word>& instructions, const ComponentID& component);
  void    storeData(const DataBlock& data);

  const void* getMemoryData();

  void    print(const ComponentID& component, MemoryAddr start, MemoryAddr end);
  Word    readWord(const ComponentID& component, MemoryAddr addr);
  Word    readByte(const ComponentID& component, MemoryAddr addr);
  void    writeWord(const ComponentID& component, MemoryAddr addr, Word data);
  void    writeByte(const ComponentID& component, MemoryAddr addr, Word data);
  int     readRegister(const ComponentID& component, RegisterIndex reg) const;
  MemoryAddr getInstIndex(const ComponentID& component) const;
  bool    readPredicate(const ComponentID& component) const;

private:

  // Make any necessary wires needed to connect components together.
  void    makeSignals();

  // Make all cores, memories and interconnect modules.
  void    makeComponents();

  // Wire everything together.
  void    wireUp();

//==============================//
// Components
//==============================//

private:

	vector<Core*>              cores;             // All cores of the chip
	vector<MemoryBank*>        memories;          // All memories of the chip
	vector<MissHandlingLogic*> mhl;               // One per tile
	SimplifiedOnChipScratchpad backgroundMemory;

	// Global networks - connect all tiles together.
	DataNetwork                dataNet;
	CreditNetwork              creditNet;
	RequestNetwork             requestNet;
	ResponseNetwork            responseNet;

	// All networks internal to each tile. To be folded in to another network
	// at some point.
	NetworkHierarchy           network;

//==============================//
// Signals (wires)
//==============================//

private:

	sc_clock clock;

	// Naming of signals is relative to the components: iData is a data signal
	// which is an input to a core or memory bank.
  LokiVector<DataSignal>       oDataLocal,          iDataLocal;
  LokiVector2D<DataSignal>     oDataGlobal,         iDataGlobal;
	LokiVector2D<CreditSignal>   oCredit,             iCredit;
  LokiVector2D<RequestSignal>  requestToMHL,        requestFromMHL;
  LokiVector2D<ResponseSignal> responseFromMHL,     responseToMHL;
  LokiVector<ResponseSignal>   responseFromBM,      responseToBanks;
  LokiVector2D<ResponseSignal> responseFromBanks;
  LokiVector<RequestSignal>    requestToBM,         requestToBanks;
  LokiVector2D<RequestSignal>  requestFromBanks;
  LokiVector<sc_signal<MemoryIndex> > requestTarget, responseTarget;
  LokiVector2D<sc_signal<bool> > l2RequestClaim;
  LokiVector<sc_signal<bool> > l2RequestClaimed;

  // Index ready signals using oReadyData[tile][component][buffer].
  LokiVector3D<ReadySignal>  oReadyData, oReadyCredit;
  LokiVector3D<ReadySignal>  readyRequestToMHL, readyResponseToMHL;

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
