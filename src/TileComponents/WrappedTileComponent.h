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
#include "../Network/FlowControl/FlowControlIn.h"
#include "../Network/FlowControl/FlowControlOut.h"

class WrappedTileComponent: public Component {

//==============================//
// Ports
//==============================//

public:

  // Clock.
  sc_in<bool>               clock;

  // All inputs to the component (NUM_CLUSTER_INPUTS).
  sc_in<Word>              *dataIn;

  // All outputs from the component (NUM_CLUSTER_OUTPUTS).
  sc_out<AddressedWord>    *dataOut;

  // Requests to each input, asking whether it is possible to send data.
  // There should be NUM_CLUSTER_INPUTS of them.
  sc_in<Word>              *requestsIn;

  // Requests sent from each output (NUM_CLUSTER_OUTPUTS).
  sc_out<AddressedWord>    *requestsOut;

  // Responses received to requests sent (NUM_CLUSTER_OUTPUTS).
  sc_in<Word>              *responsesIn;

  // Responses sent from each input, saying whether they are ready for more
  // data. There should be NUM_CLUSTER_INPUTS of them.
  sc_out<AddressedWord>    *responsesOut;

  // Shows whether or not this component is doing any work currently.
  sc_out<bool>              idle;

//==============================//
// Constructors and destructors
//==============================//

public:

  WrappedTileComponent(sc_module_name name, int ID, int type);
  virtual ~WrappedTileComponent();

//==============================//
// Methods
//==============================//

public:

  virtual double area()  const;
  virtual double energy() const;

  // Store initial instructions or data into the contained cluster or memory.
  virtual void storeData(std::vector<Word>& data);

  void         print(int start, int end) const;
  int          getRegVal(int reg) const;

  // Perform any initialisation required (e.g. setting flow control values).
  void         initialise();

private:

  void setup();

//==============================//
// Components
//==============================//

private:

  TileComponent            *comp;
  FlowControlIn             fcIn;
  FlowControlOut            fcOut;

//==============================//
// Signals (wires)
//==============================//

private:

  flag_signal<Word>          *dataInSig;                // array
  flag_signal<AddressedWord> *dataOutSig, *dataOutSig2; // arrays
  sc_buffer<AddressedWord>   *requestsOutSig;           // array
  sc_signal<bool>            *fcOutSig;                 // array
  sc_signal<int>             *fcInSig;                  // array

};

#endif /* WRAPPEDTILECOMPONENT_H_ */
