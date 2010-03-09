/*
 * WrappedTileComponent.h
 *
 * A TileComponent (cluster or memory mat) with flow control at every input and
 * output.
 *
 *  Created on: 17 Feb 2010
 *      Author: db434
 */

#ifndef WRAPPEDTILECOMPONENT_H_
#define WRAPPEDTILECOMPONENT_H_

#include "../Component.h"
#include "TileComponent.h"
#include "../Network/Flow Control/FlowControlIn.h"
#include "../Network/Flow Control/FlowControlOut.h"

class WrappedTileComponent: public Component {

public:
/* Ports */
  sc_in<bool>               clock;
  sc_in<Word>              *dataIn,  *requestsIn,  *responsesIn;  // arrays
  sc_out<AddressedWord>    *dataOut, *requestsOut, *responsesOut; // arrays

/* Constructors and destructors */
  WrappedTileComponent(sc_module_name name, TileComponent& tc);
  WrappedTileComponent(sc_module_name name, int ID, int type);
  virtual ~WrappedTileComponent();

/* Methods */
  virtual void storeData(std::vector<Word>& data);
  void         initialise();

private:
  void setup();

/* Components */
  TileComponent            *comp;
  FlowControlIn             fcIn;
  FlowControlOut            fcOut;

/* Signals (wires) */
  flag_signal<Word>        *dataInSig;  // array
  sc_buffer<AddressedWord> *dataOutSig, *dataOutSig2, *requestsOutSig;  // arrays
  sc_signal<bool>          *fcOutSig,   *fcInSig; // arrays

};

#endif /* WRAPPEDTILECOMPONENT_H_ */
