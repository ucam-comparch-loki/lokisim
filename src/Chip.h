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
#include "Network/NetworkHierarchy.h"
#include "Utility/BatchMode/BatchModeEventRecorder.h"

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
  Chip(sc_module_name name, ComponentID ID, BatchModeEventRecorder *eventRecorder);
  virtual ~Chip();

//==============================//
// Methods
//==============================//

public:

  virtual double area()   const;
  virtual double energy() const;

  bool    isIdle() const;

  // Store the given instructions or data into the component at the given index.
  void    storeData(vector<Word>& data, ComponentID component, MemoryAddr location=0);

  void    print(ComponentID component, MemoryAddr start, MemoryAddr end) const;
  Word    readWord(ComponentID component, MemoryAddr addr) const;
  Word    readByte(ComponentID component, MemoryAddr addr) const;
  void    writeWord(ComponentID component, MemoryAddr addr, Word data) const;
  void    writeByte(ComponentID component, MemoryAddr addr, Word data) const;
  int     readRegister(ComponentID component, RegisterIndex reg) const;
  MemoryAddr getInstIndex(ComponentID component) const;
  bool    readPredicate(ComponentID component) const;

private:

  void    updateIdle();

//==============================//
// Components
//==============================//

private:

  vector<TileComponent*> contents;  // Clusters and memories of this tile
  NetworkHierarchy network;

//==============================//
// Signals (wires)
//==============================//

private:

  flag_signal<AddressedWord> *dataFromComponents;
  flag_signal<Word>          *dataToComponents;
  flag_signal<int>           *creditsFromComponents;
  flag_signal<bool>          *networkReadyForData;
  sc_signal<bool>            *idleSig;

  sc_buffer<AddressedWord>   *creditsToComponents;
  sc_signal<bool>            *compsReadyForCredits;

//==============================//
// Local state
//==============================//

private:

  bool idleVal;

};

#endif /* CHIP_H_ */
