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
#include "../Network/Topologies/NormalCrossbar.h"

class AddressedWord;
class FlowControlIn;
class Word;

class InputCrossbar: public Component {

//==============================//
// Ports
//==============================//

public:

  sc_in<bool>   clock;

  DataInput    *dataIn;
  sc_out<Word> *dataOut;

  sc_in<int>   *creditsIn;
  CreditOutput *creditsOut;

  ReadyInput   *canSendCredits;
  ReadyOutput  *canReceiveData;

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
  NormalCrossbar dataXbar;
  NormalCrossbar creditXbar;

  sc_buffer<DataType>   *dataToBuffer;
  sc_buffer<CreditType> *creditsToNetwork;
  sc_signal<ReadyType>  *readyForData, *readyForCredit;

};

#endif /* INPUTCROSSBAR_H_ */
