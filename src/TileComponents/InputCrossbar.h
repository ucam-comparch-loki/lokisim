/*
 * InputCrossbar.h
 *
 * A crossbar between all input ports of a core, and all input buffers. There
 * will usually be many more buffers than ports.
 *
 *  Created on: 18 Mar 2011
 *      Author: db434
 */

#ifndef INPUTCROSSBAR_H_
#define INPUTCROSSBAR_H_

#include "../Component.h"

class AddressedWord;
class FlowControlIn;
class Word;

class InputCrossbar: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<AddressedWord>  *dataIn;
  sc_out<Word>          *dataOut;

  sc_in<int>            *flowControlIn;
  sc_out<AddressedWord> *creditsOut;

  sc_in<bool>           *readyIn;
  sc_out<bool>          *readyOut;

//==============================//
// Constructors and destructors
//==============================//

public:

  InputCrossbar(sc_module_name name, ComponentID ID, int inputs, int outputs);
  virtual ~InputCrossbar();

//==============================//
// Local state
//==============================//

private:

  ChannelID firstInput;
  int numInputs, numOutputs;

  std::vector<FlowControlIn*> flowControl;

};

#endif /* INPUTCROSSBAR_H_ */
