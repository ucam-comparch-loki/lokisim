/*
 * TileComponent.h
 *
 * A class representing any component which can be a networked part of a Tile.
 * This primarily means Clusters and memories, but may include more components
 * later if appropriate. All TileComponents have the same network interface,
 * with the same numbers of input channels and output channels.
 *
 *  Created on: 5 Jan 2010
 *      Author: db434
 */

#ifndef TILECOMPONENT_H_
#define TILECOMPONENT_H_

#include "Component.h"

#include "Datatype/Word.h"
#include "Datatype/AddressedWord.h"
#include "Datatype/Array.h"

class TileComponent : public Component {

public:
/* Ports */
  sc_in<bool>            clock;
  sc_in<Word>           *in;              // array
  sc_out<AddressedWord> *out;             // array
  sc_in<bool>           *flowControlIn;   // array
  sc_out<bool>          *flowControlOut;  // array

/* Constructors and destructors */
  TileComponent(sc_core::sc_module_name name, int ID);
  ~TileComponent();

  // Copy constructor and assignment operator
  TileComponent(const TileComponent& other);
  TileComponent* operator= (const TileComponent& other);

};

#endif /* TILECOMPONENT_H_ */
