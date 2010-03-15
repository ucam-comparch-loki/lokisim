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

#include "../Component.h"
#include "../Datatype/Word.h"
#include "../Datatype/AddressedWord.h"

class TileComponent : public Component {

public:

/* Ports */

  // Clock
  sc_in<bool>            clock;

  // All inputs to the component. There should be NUM_CLUSTER_INPUTS of them.
  sc_in<Word>           *in;

  // All outputs of the component. There should be NUM_CLUSTER_OUTPUTS of them.
  sc_out<AddressedWord> *out;

  // A flow control signal for each output (NUM_CLUSTER_OUTPUTS).
  sc_in<bool>           *flowControlIn;

  // A flow control signal for each input (NUM_CLUSTER_INPUTS).
  sc_out<bool>          *flowControlOut;  // array

/* Constructors and destructors */
  TileComponent(sc_module_name name, int ID);
  virtual ~TileComponent();

/* Methods */
  virtual void storeData(std::vector<Word>& data) = 0;

  enum Type {CLUSTER, MEMORY};

};

#endif /* TILECOMPONENT_H_ */
