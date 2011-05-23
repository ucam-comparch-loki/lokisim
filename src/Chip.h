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
#include "flag_signal.h"
#include "TileComponents/Cluster.h"
#include "TileComponents/Memory/MemoryBank.h"
#include "TileComponents/Memory/SimplifiedOnChipScratchpad.h"
#include "Network/NetworkHierarchy.h"

using std::vector;

class TileComponent;

class Chip : public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>  clock;

  sc_out<bool> idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  SC_HAS_PROCESS(Chip);
  Chip(sc_module_name name, const ComponentID& ID);
  virtual ~Chip();

//==============================//
// Methods
//==============================//

public:

  virtual double area()   const;
  virtual double energy() const;

  bool    isIdle() const;

  // Store the given instructions or data into the component at the given index.
  void    storeInstructions(vector<Word>& instructions, const ComponentID& component);
  void    storeData(vector<Word>& data, const ComponentID& component, MemoryAddr location=0);

  void    print(const ComponentID& component, MemoryAddr start, MemoryAddr end);
  Word    readWord(const ComponentID& component, MemoryAddr addr);
  Word    readByte(const ComponentID& component, MemoryAddr addr);
  void    writeWord(const ComponentID& component, MemoryAddr addr, Word data);
  void    writeByte(const ComponentID& component, MemoryAddr addr, Word data);
  int     readRegister(const ComponentID& component, RegisterIndex reg) const;
  MemoryAddr getInstIndex(const ComponentID& component) const;
  bool    readPredicate(const ComponentID& component) const;

private:

  void    updateIdle();

//==============================//
// Components
//==============================//

private:

	vector<Cluster*> clusters;  // All clusters of the chip
	vector<MemoryBank*> memories;  // All memories of the chip
	SimplifiedOnChipScratchpad backgroundMemory;
	NetworkHierarchy network;

//==============================//
// Signals (wires)
//==============================//

private:

	sc_signal<bool> 							*idleSig;

	sc_buffer<DataType> 						*dataFromComponents;
	flag_signal<DataType> 						*dataToComponents;
	sc_buffer<CreditType> 						*creditsFromComponents;
	sc_buffer<CreditType> 						*creditsToComponents;

	sc_signal<ReadyType> 						*ackDataFromComps;
	sc_signal<ReadyType> 						*ackCreditToComps;
	sc_signal<ReadyType> 						*ackDataToComps;
	sc_signal<ReadyType> 						*ackCreditFromComps;

	sc_signal<ReadyType> 						*validDataFromComps;
	sc_signal<ReadyType> 						*validDataToComps;
	sc_signal<ReadyType> 						*validCreditFromComps;
	sc_signal<ReadyType> 						*validCreditToComps;

	sc_signal<bool>								*strobeToBackgroundMemory;
	sc_signal<MemoryRequest>					*dataToBackgroundMemory;
	sc_signal<bool>								*strobeFromBackgroundMemory;
	sc_signal<Word>								*dataFromBackgroundMemory;

	sc_signal<bool>								*ringStrobe;
	sc_signal<MemoryBank::RingNetworkRequest>	*ringRequest;
	sc_signal<bool>								*ringAcknowledge;

//==============================//
// Local state
//==============================//

private:

  bool idleVal;

};

#endif /* CHIP_H_ */
